/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2017 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "webenginepartdownloadmanager.h"

#include "webenginepage.h"
#include <webenginepart_debug.h>
#include "webenginepartcontrols.h"
#include "navigationrecorder.h"
#include "choosepagesaveformatdlg.h"

#include <QWebEngineView>
#include <QWebEngineProfile>
#include <QFileDialog>
#include <QStandardPaths>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <QTimer>
#include <QDialog>

#include <KLocalizedString>
#include <KJobTrackerInterface>
#include <KIO/JobTracker>
#include <KIO/OpenUrlJob>
#include <KFileUtils>
#include <KIO/JobUiDelegate>
#include <KIO/JobUiDelegateFactory>

using namespace KonqInterfaces;

QTemporaryDir& WebEnginePartDownloadManager::tempDownloadDir()
{
    static QTemporaryDir s_tempDownloadDir(QDir(QDir::tempPath()).filePath(QStringLiteral("WebEnginePartDownloadManager")));
    return s_tempDownloadDir;
}

WebEnginePartDownloadManager::WebEnginePartDownloadManager(QWebEngineProfile *profile, QObject *parent)
    : QObject(parent)
{
    connect(profile, &QWebEngineProfile::downloadRequested, this, &WebEnginePartDownloadManager::performDownload);
}

WebEnginePartDownloadManager::~WebEnginePartDownloadManager()
{
}

void WebEnginePartDownloadManager::setForceDownload(const QUrl& url, WebEnginePage *page)
{
    m_forcedDownloads.insert(url, page);
}

bool WebEnginePartDownloadManager::checkForceDownload(QWebEngineDownloadRequest *req, WebEnginePage *page)
{
    if (!page) {
        return false;
    }
    bool force = m_forcedDownloads.remove(req->url(), page) != 0;

    // Bookkeeping: remove any entry with an invalid page. Note that in most cases, m_forcedDownloads
    // will be empty, since entries are usually added just before this is called indirectly from
    // WebEnginePage::download. The line before this removes one entry from m_forcedDownloads, which
    // usually will be the one just added, meaning m_forcedDownloads will be empty.
    for (const QUrl &k : m_forcedDownloads.uniqueKeys()) {
        m_forcedDownloads.remove(k, nullptr);
    }
    if (force) {
        return true;
    }

    //Check whether the mimetypes is one known to be displayed by QtWebEngine itself. If so, it means
    //that the file should be saved (otherwise, QtWebEngine would have displayed it and not requested
    //a download.
    //TODO: find out all the mimetypes supported by QtWebEngine
    static const QStringList s_supportedMimetypes = {
        QStringLiteral("audio/mp4"),
        QStringLiteral("audio/mpeg"),
        QStringLiteral("audio/ogg"),
        QStringLiteral("image/bmp"),
        QStringLiteral("image/gif"),
        QStringLiteral("image/jpeg"),
        QStringLiteral("image/png"),
        QStringLiteral("image/svg+xml"),
        QStringLiteral("video/mp4"),
        QStringLiteral("video/ogg"),
        QStringLiteral("application/xml"),
        QStringLiteral("text/plain"),
        QStringLiteral("text/html"),
        QStringLiteral("text/xml"),
        QStringLiteral("text/markdown"),
    };
    return s_supportedMimetypes.contains(req->mimeType());
}

void WebEnginePartDownloadManager::addPage(WebEnginePage* page)
{
    if (!m_pages.contains(page)) {
        m_pages.append(page);
    }
    connect(page, &QObject::destroyed, this, &WebEnginePartDownloadManager::removePage);
}

void WebEnginePartDownloadManager::removePage(QObject* page)
{
    m_pages.removeOne(static_cast<WebEnginePage*>(page));
}

void WebEnginePartDownloadManager::performDownload(QWebEngineDownloadRequest* it)
{
    QUrl url = it->url();
    WebEnginePage *page = qobject_cast<WebEnginePage*>(it->page());
    if (it->isSavePageDownload()) {
        saveHtmlPage(it, page);
        return;
    }
    bool forceNew = false;
    //According to the documentation, QWebEngineDownloadRequest::page() can return nullptr "if the download was not triggered by content in a page"
    if (!page && !m_pages.isEmpty()) {
        qCDebug(WEBENGINEPART_LOG) << "downloading" << url << "in new window or tab";
        page = m_pages.first();
        forceNew = true;
    } else if (!page) {
        qCDebug(WEBENGINEPART_LOG) << "Couldn't find a part wanting to download" << url;
        it->cancel();
        return;
    }

    if (WebEnginePartControls::self()->navigationRecorder()->isPostRequest(it->url(), page)) {
        WebEnginePartControls::self()->navigationRecorder()->recordNavigationFinished(page, url);
        //When downloading the reply to a POST request, it's better to use a new tab, as navigating back to POST requests
        //can be problematic
        forceNew = true;
    }
    bool forceDownload = checkForceDownload(it, page);

    it->setDownloadDirectory(tempDownloadDir().path());
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForName(it->mimeType());
    QString suggestedName = it->suggestedFileName();
    QString fileName = generateDownloadTempFileName(suggestedName, type.preferredSuffix());
    it->setDownloadFileName(fileName);

    page->requestDownload(it, forceNew, forceDownload);
}

QString WebEnginePartDownloadManager::generateDownloadTempFileName(const QString& suggestedName, const QString& ext)
{
    QString baseName(suggestedName);
    if (baseName.isEmpty()) {
        baseName = QString::number(QTime::currentTime().msecsSinceStartOfDay());
    }
    if (QFileInfo(baseName).completeSuffix().isEmpty() && !ext.isEmpty()) {
        baseName.append("."+ext);
    }
    QString completeName = QDir(tempDownloadDir().path()).filePath(baseName);
    if (QFileInfo::exists(completeName)) {
        completeName = KFileUtils::suggestName(QUrl::fromLocalFile(tempDownloadDir().path()), baseName);
    }
    return completeName;
}

void WebEnginePartDownloadManager::saveHtmlPage(QWebEngineDownloadRequest* it, WebEnginePage *page)
{
    QWidget *parent = page ? page->view() : nullptr;

    ChoosePageSaveFormatDlg *formatDlg = new ChoosePageSaveFormatDlg(parent);
    if (formatDlg->exec() == QDialog::Rejected) {
        return;
    }
    QWebEngineDownloadRequest::SavePageFormat format = formatDlg->choosenFormat();
    it->setSavePageFormat(format);

    QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QFileDialog dlg(parent, QString(), downloadDir);
    dlg.setAcceptMode(QFileDialog::AcceptSave);

    QString mimeType = format == QWebEngineDownloadRequest::MimeHtmlSaveFormat ? QStringLiteral("application/x-mimearchive") : QStringLiteral("text/html");

    dlg.setMimeTypeFilters(QStringList{mimeType, "application/octet-stream"});
    dlg.selectFile(it->suggestedFileName());

    QDialog::DialogCode exitCode = static_cast<QDialog::DialogCode>(dlg.exec());
    if (exitCode == QDialog::Rejected) {
        it->cancel();
        return;
    }

    QString file = dlg.selectedFiles().at(0);
    QFileInfo info(file);
    QString relativePath = info.fileName();
    QString dir = info.dir().path();

    it->setDownloadDirectory(dir);
    it->setDownloadFileName(relativePath);
    WebEngineDownloadJob *job = new WebEngineDownloadJob(it, this);
    KJobTrackerInterface *t = KIO::getJobTracker();
    if (t) {
        t->registerJob(job);
    }
    job->start();
}

WebEngineDownloadJob::WebEngineDownloadJob(QWebEngineDownloadRequest* it, QObject* parent) : DownloaderJob(parent), m_downloadItem(it)
{
    setCapabilities(KJob::Killable|KJob::Suspendable);
    connect(m_downloadItem, &QWebEngineDownloadRequest::stateChanged, this, &WebEngineDownloadJob::stateChanged);
    setTotalAmount(KJob::Bytes, m_downloadItem->totalBytes());
    setFinishedNotificationHidden(true);
    setAutoDelete(false);
}

WebEngineDownloadJob::~WebEngineDownloadJob() noexcept
{
    if (m_downloadItem) {
        m_downloadItem->deleteLater();
        m_downloadItem = nullptr;
    }
}

void WebEngineDownloadJob::start()
{
    if (m_downloadItem && m_downloadItem->state() == QWebEngineDownloadItem::DownloadRequested) {
        m_downloadItem->accept();
    }
    QTimer::singleShot(0, this, &WebEngineDownloadJob::startDownloading);
}

bool WebEngineDownloadJob::finished() const
{
    return !m_downloadItem || (m_started && m_downloadItem->isFinished());
}

bool WebEngineDownloadJob::doKill()
{
    m_downloadItem->cancel();
    return true;
}

bool WebEngineDownloadJob::doResume()
{
    if (m_downloadItem) {
        m_downloadItem->resume();
    }
    return true;
}

bool WebEngineDownloadJob::doSuspend()
{
    if (m_downloadItem) {
        m_downloadItem->pause();
    }
    return true;
}

#if QT_VERSION_MAJOR < 6
void WebEngineDownloadJob::downloadProgressed(quint64 received, quint64 total)
{
    setPercent(total != 0 ? received*100.0/total : 0);
}
#else
void WebEngineDownloadJob::downloadProgressed()
{
    setPercent(m_downloadItem->totalBytes() != 0 ? m_downloadItem->receivedBytes()*100/m_downloadItem->totalBytes() : 0);
}
#endif

void WebEngineDownloadJob::stateChanged(QWebEngineDownloadRequest::DownloadState state)
{
    switch (state) {
        case QWebEngineDownloadRequest::DownloadInterrupted:
        case QWebEngineDownloadRequest::DownloadCancelled:
            setError(m_downloadItem->interruptReason() + UserDefinedError);
            setErrorText(m_downloadItem->interruptReasonString());
            break;
        default: return;
    }
}

QString WebEngineDownloadJob::errorString() const
{
    return i18n("An error occurred while saving the file: %1", errorText());
}

void WebEngineDownloadJob::startDownloading()
{
    m_started = true;
    if (!m_downloadItem) {
        return;
    }
    m_startTime = QDateTime::currentDateTime();
    QString name = m_downloadItem->downloadFileName();
    emit description(this, i18nc("Notification about downloading a file", "Downloading"),
                    QPair<QString, QString>(i18nc("Source of a file being downloaded", "Source"), m_downloadItem->url().toString()),
                    QPair<QString, QString>(i18nc("Destination of a file download", "Destination"), name));
    //Between calls to QWebEngineDownloadRequest::accept and QWebEngineDownloadRequest::pause, QtWebEngine already starts downloading the file
    //This means that, for small files, it's possible that when WebEngineDownloadJob::start is called, the download will already have been
    //completed. In that case, set the download progress to 100% and emit the result() signal
    if (!m_downloadItem->isFinished()) {
#if QT_VERSION_MAJOR < 6
        connect(m_downloadItem, &QWebEngineDownloadRequest::downloadProgress, this, &WebEngineDownloadJob::downloadProgressed);
        connect(m_downloadItem, &QWebEngineDownloadRequest::finished, this, &WebEngineDownloadJob::downloadFinished);
#else
        connect(m_downloadItem, &QWebEngineDownloadRequest::receivedBytesChanged, this, &WebEngineDownloadJob::downloadProgressed);
        connect(m_downloadItem, &QWebEngineDownloadRequest::isFinishedChanged, this, &WebEngineDownloadJob::downloadFinished);
#endif
        m_downloadItem->resume();
    } else {
#if QT_VERSION_MAJOR < 6
        downloadProgressed(m_downloadItem->receivedBytes(), m_downloadItem->totalBytes());
#else
        downloadProgressed();
#endif
        emitResult();
    }
}

void WebEngineDownloadJob::downloadFinished()
{
    QPointer<WebEnginePage> page = m_downloadItem ? qobject_cast<WebEnginePage*>(m_downloadItem->page()) : nullptr;
    emitResult();
    QDateTime now = QDateTime::currentDateTime();
    if (m_startTime.msecsTo(now) < 500) {
        if (page) {
            QString filePath = QDir(m_downloadItem->downloadDirectory()).filePath(m_downloadItem->downloadFileName());
            emit page->setStatusBarText(i18nc("Finished saving URL", "Saved %1 as %2", m_downloadItem->url().toString(), filePath));
        }
    }
}

QString WebEngineDownloadJob::downloadPath() const
{
    if (!m_downloadItem) {
        return QString();
    }
    return QDir(m_downloadItem->downloadDirectory()).filePath(m_downloadItem->downloadFileName());
}

QWebEngineDownloadRequest * WebEngineDownloadJob::item() const
{
    return m_downloadItem;
}

bool WebEngineDownloadJob::setDownloadPath(const QString& path)
{
    if (!canChangeDownloadPath()) {
        return false;
    }
    QFileInfo info(path);
    m_downloadItem->setDownloadFileName(info.fileName());
    m_downloadItem->setDownloadDirectory(info.path());
    return true;
}

bool WebEngineDownloadJob::canChangeDownloadPath() const
{
    return m_downloadItem && m_downloadItem->state() == QWebEngineDownloadItem::DownloadRequested;
}
