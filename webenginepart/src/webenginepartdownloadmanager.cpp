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
#include <KNotificationJobUiDelegate>
#include <KJobTrackerInterface>
#include <KIO/JobTracker>
#include <KIO/OpenUrlJob>
#include <KFileUtils>
#include <KIO/JobUiDelegate>

WebEnginePartDownloadManager::WebEnginePartDownloadManager(QWebEngineProfile *profile, QObject *parent)
    : QObject(parent), m_tempDownloadDir(QDir(QDir::tempPath()).filePath("WebEnginePartDownloadManager"))
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

bool WebEnginePartDownloadManager::checkForceDownload(const QUrl& url, WebEnginePage *page)
{
    if (!page) {
        return false;
    }
    bool force = m_forcedDownloads.remove(url, page) != 0;

    // Bookkeeping: remove any entry with an invalid page. Note that in most cases, m_forcedDownloads
    // will be empty, since entries are usually added just before this is called indirectly from
    // WebEnginePage::download. The line before this removes one entry from m_forcedDownloads, which
    // usually will be the one just added, meaning m_forcedDownloads will be empty.
    for (const QUrl &k : m_forcedDownloads.uniqueKeys()) {
        m_forcedDownloads.remove(k, nullptr);
    }

    return force;
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

void WebEnginePartDownloadManager::performDownload(QWebEngineDownloadItem* it)
{
    QUrl url = it->url();
    WebEnginePage *page = qobject_cast<WebEnginePage*>(it->page());
    if (it->isSavePageDownload()) {
        saveHtmlPage(it, page);
        return;
    }
    bool forceNew = false;
    //According to the documentation, QWebEngineDownloadItem::page() can return nullptr "if the download was not triggered by content in a page"
    if (!page && !m_pages.isEmpty()) {
        qCDebug(WEBENGINEPART_LOG) << "downloading" << url << "in new window or tab";
        page = m_pages.first();
        forceNew = true;
    } else if (!page) {
        qCDebug(WEBENGINEPART_LOG) << "Couldn't find a part wanting to download" << url;
        return;
    }

    if (WebEnginePartControls::self()->navigationRecorder()->isPostRequest(it->url(), page)) {
        WebEnginePartControls::self()->navigationRecorder()->recordNavigationFinished(page, url);
        downloadFile(it, KParts::BrowserOpenOrSaveQuestion::InlineDisposition, true);
        return;
    }

    bool forceDownload = checkForceDownload(url, page);
    if (!forceDownload && it->url().scheme() != "blob") {
        page->downloadItem(it, forceNew);
    } else {
        downloadFile(it, KParts::BrowserOpenOrSaveQuestion::AttachmentDisposition);
    }
}

void WebEnginePartDownloadManager::downloadFile(QWebEngineDownloadItem* it, KParts::BrowserOpenOrSaveQuestion::AskEmbedOrSaveFlags disposition, bool forceNewTab)
{
    WebEnginePage *p = qobject_cast<WebEnginePage*>(it->page());
    QWidget *w = p ? p->view() : nullptr;
    KParts::BrowserOpenOrSaveQuestion askDlg(w, it->url(), it->mimeType());
    KParts::BrowserOpenOrSaveQuestion::Result ans = askDlg.askEmbedOrSave(disposition);
    switch (ans) { 
        case KParts::BrowserOpenOrSaveQuestion::Cancel:
            it->cancel();
            return;
        case KParts::BrowserOpenOrSaveQuestion::Save:
            saveFile(it);
            break;
        case KParts::BrowserOpenOrSaveQuestion::Embed:
        case KParts::BrowserOpenOrSaveQuestion::Open:
            openFile(it, p, forceNewTab);
            break;
    }
}

void WebEnginePartDownloadManager::saveFile(QWebEngineDownloadItem* it)
{
    QWidget *w = it->page() ? it->page()->view() : nullptr;
    QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForName(it->mimeType());
    QFileDialog dlg(w, QString(), downloadDir);
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setMimeTypeFilters(QStringList{type.name(), "application/octet-stream"});
    QDialog::DialogCode exitCode = static_cast<QDialog::DialogCode>(dlg.exec());
    if (exitCode == QDialog::Rejected) {
        it->cancel();
        return;
    }
    QString file = dlg.selectedFiles().at(0);
    startDownloadJob(file, it);
}

void WebEnginePartDownloadManager::openFile(QWebEngineDownloadItem* it, WebEnginePage *page, bool forceNewTab)
{
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForName(it->mimeType());
    QString suggestedName = it->suggestedFileName();
    QString fileName = generateDownloadTempFileName(suggestedName, type.preferredSuffix());
    it->setDownloadDirectory(m_tempDownloadDir.path());
    it->setDownloadFileName(fileName);
    connect(it, &QWebEngineDownloadItem::finished, this, [this, it, page, forceNewTab](){
        downloadToFileCompleted(it, page, forceNewTab);});
    it->accept();
}

QString WebEnginePartDownloadManager::generateDownloadTempFileName(const QString& suggestedName, const QString& ext) const
{
    QString baseName(suggestedName);
    if (baseName.isEmpty()) {
        baseName = QString::number(QTime::currentTime().msecsSinceStartOfDay());
    }
    if (QFileInfo(baseName).completeSuffix().isEmpty() && !ext.isEmpty()) {
        baseName.append("."+ext);
    }
    QString completeName = QDir(m_tempDownloadDir.path()).filePath(baseName);
    if (QFileInfo::exists(completeName)) {
        completeName = KFileUtils::suggestName(QUrl::fromLocalFile(m_tempDownloadDir.path()), baseName);
    }
    return completeName;
}


void WebEnginePartDownloadManager::downloadToFileCompleted(QWebEngineDownloadItem *it, WebEnginePage *page, bool forceNewTab)
{
    QString file = QDir(it->downloadDirectory()).filePath(it->downloadFileName());
    if (page) {
        page->requestOpenFileAsTemporary(QUrl::fromLocalFile(file), it->mimeType(), false, true);
    } else {
        KIO::OpenUrlJob *j = new KIO::OpenUrlJob(QUrl::fromLocalFile(file), it->mimeType(), this);
        j->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));
        j->start();
    }
}

void WebEnginePartDownloadManager::saveHtmlPage(QWebEngineDownloadItem* it, WebEnginePage *page)
{
    QWidget *parent = page ? page->view() : nullptr;

    ChoosePageSaveFormatDlg *formatDlg = new ChoosePageSaveFormatDlg(parent);
    if (formatDlg->exec() == QDialog::Rejected) {
        return;
    }
    QWebEngineDownloadItem::SavePageFormat format = formatDlg->choosenFormat();
    it->setSavePageFormat(format);

    QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QFileDialog dlg(parent, QString(), downloadDir);
    dlg.setAcceptMode(QFileDialog::AcceptSave);

    QString mimeType = format == QWebEngineDownloadItem::MimeHtmlSaveFormat ? QStringLiteral("application/x-mimearchive") : QStringLiteral("text/html");

    dlg.setMimeTypeFilters(QStringList{mimeType, "application/octet-stream"});
    dlg.selectFile(it->suggestedFileName());

    QDialog::DialogCode exitCode = static_cast<QDialog::DialogCode>(dlg.exec());
    if (exitCode == QDialog::Rejected) {
        it->cancel();
        return;
    }

    startDownloadJob(dlg.selectedFiles().at(0), it);
}

void WebEnginePartDownloadManager::startDownloadJob(const QString &file, QWebEngineDownloadItem *it)
{
    QFileInfo info(file);
    QString relativePath = info.fileName();
    QString dir = info.dir().path();

    it->setDownloadDirectory(dir);
    it->setDownloadFileName(relativePath);

    it->accept();
    it->pause();
    WebEngineDownloadJob *j = new WebEngineDownloadJob(it, this);
    KJobTrackerInterface *t = KIO::getJobTracker();
    if (t) {
        t->registerJob(j);
    }
    j->start();
}

WebEngineDownloadJob::WebEngineDownloadJob(QWebEngineDownloadItem* it, QObject* parent) : KJob(parent), m_downloadItem(it)
{
    setCapabilities(KJob::Killable|KJob::Suspendable);
    setTotalAmount(KJob::Bytes, m_downloadItem->totalBytes());
    connect(m_downloadItem, &QWebEngineDownloadItem::downloadProgress, this, &WebEngineDownloadJob::downloadProgressed);
    connect(m_downloadItem, &QWebEngineDownloadItem::finished, this, &WebEngineDownloadJob::downloadFinished);
    connect(m_downloadItem, &QWebEngineDownloadItem::stateChanged, this, &WebEngineDownloadJob::stateChanged);
}

void WebEngineDownloadJob::start()
{
    QTimer::singleShot(0, this, &WebEngineDownloadJob::startDownloading);
}

bool WebEngineDownloadJob::doKill()
{
    m_downloadItem->deleteLater();
    m_downloadItem = nullptr;
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

void WebEngineDownloadJob::downloadProgressed(quint64 received, quint64 total)
{
    setPercent(received*100.0/total);
}

void WebEngineDownloadJob::stateChanged(QWebEngineDownloadItem::DownloadState state)
{
    if (state != QWebEngineDownloadItem::DownloadInterrupted) {
        return;
    }
    setError(m_downloadItem->interruptReason() + UserDefinedError);
    setErrorText(m_downloadItem->interruptReasonString());
}

QString WebEngineDownloadJob::errorString() const
{
    return i18n("An error occurred while saving the file: %1", errorText());
}

void WebEngineDownloadJob::startDownloading()
{
    if (m_downloadItem) {
        m_startTime = QDateTime::currentDateTime();
        QString name = m_downloadItem->downloadFileName();
        emit description(this, i18nc("Notification about downloading a file", "Downloading"),
                        QPair<QString, QString>(i18nc("Source of a file being downloaded", "Source"), m_downloadItem->url().toString()),
                        QPair<QString, QString>(i18nc("Destination of a file download", "Destination"), name));
        m_downloadItem->resume();
    }
}

void WebEngineDownloadJob::downloadFinished()
{
    emitResult();
    QDateTime now = QDateTime::currentDateTime();
    if (m_startTime.msecsTo(now) < 500) {
        if (m_downloadItem && m_downloadItem->page()) {
            WebEnginePage *page = qobject_cast<WebEnginePage*>(m_downloadItem->page());
            QString filePath = QDir(m_downloadItem->downloadDirectory()).filePath(m_downloadItem->downloadFileName());
            emit page->setStatusBarText(i18nc("Finished saving BLOB URL", "Finished saving %1 as %2", m_downloadItem->url().toString(), filePath));
        }
    }
    delete m_downloadItem;
    m_downloadItem = nullptr;
}
