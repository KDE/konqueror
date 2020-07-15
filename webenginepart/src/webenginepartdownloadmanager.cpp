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

#include <QWebEngineDownloadItem>
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

WebEnginePartDownloadManager::WebEnginePartDownloadManager()
    : QObject()
{
    connect(QWebEngineProfile::defaultProfile(), &QWebEngineProfile::downloadRequested, this, &WebEnginePartDownloadManager::performDownload);
}

WebEnginePartDownloadManager::~WebEnginePartDownloadManager()
{
#ifndef DOWNLOADITEM_KNOWS_PAGE
    m_requests.clear();
#endif
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
#ifndef DOWNLOADITEM_KNOWS_PAGE
    connect(page, &WebEnginePage::navigationRequested, this, &WebEnginePartDownloadManager::recordNavigationRequest);
#endif
    connect(page, &QObject::destroyed, this, &WebEnginePartDownloadManager::removePage);
}

void WebEnginePartDownloadManager::removePage(QObject* page)
{
#ifndef DOWNLOADITEM_KNOWS_PAGE
    const QUrl url = m_requests.key(static_cast<WebEnginePage *>(page));
    m_requests.remove(url);
#endif
    m_pages.removeOne(static_cast<WebEnginePage*>(page));
}

void WebEnginePartDownloadManager::performDownload(QWebEngineDownloadItem* it)
{
#ifdef DOWNLOADITEM_KNOWS_PAGE
    WebEnginePage *page = qobject_cast<WebEnginePage*>(it->page());
#else
    WebEnginePage *page = m_requests.take(it->url());
#endif
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
        page->download(it->url(), forceNew);
    } else {
        downloadBlob(it);
    }
}

void WebEnginePartDownloadManager::downloadBlob(QWebEngineDownloadItem* it)
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
    it->setDownloadFileName(info.fileName());
    it->setDownloadDirectory(info.path());
    it->accept();
    it->pause();
    WebEngineBlobDownloadJob *j = new WebEngineBlobDownloadJob(it, this);
    KNotificationJobUiDelegate *d = new KNotificationJobUiDelegate;
    d->setAutoErrorHandlingEnabled(true);
    d->setAutoWarningHandlingEnabled(true);
    j->setUiDelegate(d);
    emit j->description(j, i18nc("Notification about downloading a file", "Downloading"),
                        QPair<QString, QString>(i18nc("Source of a file being downloaded", "Source"), it->url().toString()),
                        QPair<QString, QString>(i18nc("Destination of a file download", "Destination"), it->downloadFileName()));
    j->start();
}

#ifndef DOWNLOADITEM_KNOWS_PAGE

void WebEnginePartDownloadManager::recordNavigationRequest(WebEnginePage *page, const QUrl& url)
{
//     qCDebug(WEBENGINEPART_LOG) << url;
    m_requests.insert(url, page);
}

WebEnginePage* WebEnginePartDownloadManager::pageForDownload(QWebEngineDownloadItem* it)
{
    WebEnginePage *page = m_requests.value(it->url());
    if (!page && !m_pages.isEmpty()) {
        page = m_pages.first();
    }
    return page;
}
#endif

WebEngineBlobDownloadJob::WebEngineBlobDownloadJob(QWebEngineDownloadItem* it, QObject* parent) : KJob(parent), m_downloadItem(it)
{
    setCapabilities(KJob::Killable|KJob::Suspendable);
    setTotalAmount(KJob::Bytes, m_downloadItem->totalBytes());
    connect(m_downloadItem, &QWebEngineDownloadItem::downloadProgress, this, &WebEngineBlobDownloadJob::downloadProgressed);
    connect(m_downloadItem, &QWebEngineDownloadItem::finished, this, [this](){emitResult();});
    connect(m_downloadItem, &QWebEngineDownloadItem::stateChanged, this, &WebEngineBlobDownloadJob::stateChanged);
}

void WebEngineBlobDownloadJob::start()
{
    QTimer::singleShot(0, m_downloadItem, &QWebEngineDownloadItem::resume);
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
    emit infoMessage(this, i18nc("Download progress", "Downloading: %1", received));
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
