/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2017 Stefano Crocco <posta@stefanocrocco.it>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "webenginepartdownloadmanager.h"

#include "webenginepage.h"
#include <webenginepart_debug.h>

#include <QWebEngineView>
#include <QWebEngineProfile>
#include <QFileDialog>
#include <QStandardPaths>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <QTimer>

#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KParts/BrowserOpenOrSaveQuestion>
#include <KJobTrackerInterface>
#include <KIO/JobTracker>
#include <KIO/OpenUrlJob>
#include <KFileUtils>
#include <KIO/JobUiDelegate>

WebEnginePartDownloadManager::WebEnginePartDownloadManager()
    : QObject(), m_tempDownloadDir(QDir(QDir::tempPath()).filePath("WebEnginePartDownloadManager"))
{
    connect(QWebEngineProfile::defaultProfile(), &QWebEngineProfile::downloadRequested, this, &WebEnginePartDownloadManager::performDownload);
}

WebEnginePartDownloadManager::~WebEnginePartDownloadManager()
{
}

WebEnginePartDownloadManager * WebEnginePartDownloadManager::instance()
{
    static WebEnginePartDownloadManager inst;
    return &inst;
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
    WebEnginePage *page = qobject_cast<WebEnginePage*>(it->page());
    bool forceNew = false;
    //According to the documentation, QWebEngineDownloadItem::page() can return nullptr "if the download was not triggered by content in a page"
    if (!page && !m_pages.isEmpty()) {
        qCDebug(WEBENGINEPART_LOG) << "downloading" << it->url() << "in new window or tab";
        page = m_pages.first();
        forceNew = true;
    } else if (!page) {
        qCDebug(WEBENGINEPART_LOG) << "Couldn't find a part wanting to download" << it->url();
        return;
    }
    if (it->url().scheme() != "blob") {
        page->download(it->url(), it->mimeType(), forceNew);
    } else {
        downloadBlob(it);
    }
}

void WebEnginePartDownloadManager::downloadBlob(QWebEngineDownloadItem* it)
{
    WebEnginePage *p = qobject_cast<WebEnginePage*>(it->page());
    QWidget *w = p ? p->view() : nullptr;
    KParts::BrowserOpenOrSaveQuestion askDlg(w, it->url(), it->mimeType());
    KParts::BrowserOpenOrSaveQuestion::Result ans = askDlg.askEmbedOrSave(KParts::BrowserOpenOrSaveQuestion::AttachmentDisposition);
    switch (ans) { 
        case KParts::BrowserOpenOrSaveQuestion::Cancel:
            it->cancel();
            return;
        case KParts::BrowserOpenOrSaveQuestion::Save:
            saveBlob(it);
            break;
        case KParts::BrowserOpenOrSaveQuestion::Embed:
        case KParts::BrowserOpenOrSaveQuestion::Open:
            openBlob(it, p);
            break;
    }
}

void WebEnginePartDownloadManager::saveBlob(QWebEngineDownloadItem* it)
{
    QWidget *w = it->page() ? it->page()->view() : nullptr;
    QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForName(it->mimeType());
    QFileDialog dlg(w, QString(), downloadDir);
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setMimeTypeFilters(QStringList{type.name(), "application/octet-stream"});
    dlg.setSupportedSchemes(QStringList{"file"});
    QDialog::DialogCode exitCode = static_cast<QDialog::DialogCode>(dlg.exec());
    if (exitCode == QDialog::Rejected) {
        it->cancel();
        return;
    }
    QString file = dlg.selectedFiles().at(0);
    QFileInfo info(file);
#ifdef WEBENGINEDOWNLOADITEM_USE_PATH
    it->setPath(info.filePath());
#else
    it->setDownloadDirectory(info.path());
    it->setDownloadFileName(info.fileName());
#endif
    it->accept();
    it->pause();
    WebEngineBlobDownloadJob *j = new WebEngineBlobDownloadJob(it, this);
    KJobTrackerInterface *t = KIO::getJobTracker();
    if (t) {
        t->registerJob(j);
    }
    j->start();
}

void WebEnginePartDownloadManager::openBlob(QWebEngineDownloadItem* it, WebEnginePage *page)
{
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForName(it->mimeType());
#ifdef WEBENGINEDOWNLOADITEM_USE_PATH
    QString suggestedName = it->path();
#else
    QString suggestedName = it->suggestedFileName();
#endif
    QString fileName = generateBlobTempFileName(suggestedName, type.preferredSuffix());
#ifdef WEBENGINEDOWNLOADITEM_USE_PATH
    it->setPath(m_tempDownloadDir.filePath(fileName));
#else
    it->setDownloadDirectory(m_tempDownloadDir.path());
    it->setDownloadFileName(fileName);
#endif
    connect(it, &QWebEngineDownloadItem::finished, this, [this, it, page](){blobDownloadedToFile(it, page);});
    it->accept();
}

QString WebEnginePartDownloadManager::generateBlobTempFileName(const QString& suggestedName, const QString& ext) const
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


void WebEnginePartDownloadManager::blobDownloadedToFile(QWebEngineDownloadItem *it, WebEnginePage *page)
{
#ifdef WEBENGINEDOWNLOADITEM_USE_PATH
    QString file = it->path();
#else
    QString file = QDir(it->downloadDirectory()).filePath(it->downloadFileName());
#endif
    if (page) {
        page->requestOpenFileAsTemporary(QUrl::fromLocalFile(file), it->mimeType());
    } else {
        KIO::OpenUrlJob *j = new KIO::OpenUrlJob(QUrl::fromLocalFile(file), it->mimeType(), this);
        QWidget *w = page->view();
        j->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, w ? w->window() : nullptr));
        j->start();
    }
}

WebEngineBlobDownloadJob::WebEngineBlobDownloadJob(QWebEngineDownloadItem* it, QObject* parent) : KJob(parent), m_downloadItem(it)
{
    setCapabilities(KJob::Killable|KJob::Suspendable);
    setTotalAmount(KJob::Bytes, m_downloadItem->totalBytes());
    connect(m_downloadItem, &QWebEngineDownloadItem::downloadProgress, this, &WebEngineBlobDownloadJob::downloadProgressed);
    connect(m_downloadItem, &QWebEngineDownloadItem::finished, this, &WebEngineBlobDownloadJob::downloadFinished);
    connect(m_downloadItem, &QWebEngineDownloadItem::stateChanged, this, &WebEngineBlobDownloadJob::stateChanged);
}

void WebEngineBlobDownloadJob::start()
{
    QTimer::singleShot(0, this, &WebEngineBlobDownloadJob::startDownloading);
}

bool WebEngineBlobDownloadJob::doKill()
{
    delete m_downloadItem;
    m_downloadItem = nullptr;
    return true;
}

bool WebEngineBlobDownloadJob::doResume()
{
    if (m_downloadItem) {
        m_downloadItem->resume();
    }
    return true;
}

bool WebEngineBlobDownloadJob::doSuspend()
{
    if (m_downloadItem) {
        m_downloadItem->pause();
    }
    return true;
}

void WebEngineBlobDownloadJob::downloadProgressed(quint64 received, quint64 total)
{
    setPercent(received*100.0/total);
}

void WebEngineBlobDownloadJob::stateChanged(QWebEngineDownloadItem::DownloadState state)
{
    if (state != QWebEngineDownloadItem::DownloadInterrupted) {
        return;
    }
    setError(m_downloadItem->interruptReason() + UserDefinedError);
    setErrorText(m_downloadItem->interruptReasonString());
}

QString WebEngineBlobDownloadJob::errorString() const
{
    return i18n("An error occurred while saving the file: %1", errorText());
}

void WebEngineBlobDownloadJob::startDownloading()
{
    if (m_downloadItem) {
        m_startTime = QDateTime::currentDateTime();
#ifdef WEBENGINEDOWNLOADITEM_USE_PATH
        QString name = QFileInfo(m_downloadItem->path()).filePath();
#else
        QString name = m_downloadItem->downloadFileName();
#endif
        emit description(this, i18nc("Notification about downloading a file", "Downloading"),
                        QPair<QString, QString>(i18nc("Source of a file being downloaded", "Source"), m_downloadItem->url().toString()),
                        QPair<QString, QString>(i18nc("Destination of a file download", "Destination"), name));
        m_downloadItem->resume();
    }
}

void WebEngineBlobDownloadJob::downloadFinished()
{
    emitResult();
    QDateTime now = QDateTime::currentDateTime();
    if (m_startTime.msecsTo(now) < 500) {
        if (m_downloadItem && m_downloadItem->page()) {
            WebEnginePage *page = qobject_cast<WebEnginePage*>(m_downloadItem->page());
#ifdef WEBENGINEDOWNLOADITEM_USE_PATH
            QString filePath = m_downloadItem->path();
#else
            QString filePath = QDir(m_downloadItem->downloadDirectory()).filePath(m_downloadItem->downloadFileName());
#endif
            emit page->setStatusBarText(i18nc("Finished saving BLOB URL", "Finished saving %1 as %2", m_downloadItem->url().toString(), filePath));
        }
    }
    delete m_downloadItem;
    m_downloadItem = nullptr;
}
