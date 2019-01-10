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

WebEnginePartDownloadManager::WebEnginePartDownloadManager()
    : QObject()
{
    connect(QWebEngineProfile::defaultProfile(), &QWebEngineProfile::downloadRequested, this, &WebEnginePartDownloadManager::performDownload);
}

WebEnginePartDownloadManager::~WebEnginePartDownloadManager()
{
    m_requests.clear();
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
    connect(page, &WebEnginePage::navigationRequested, this, &WebEnginePartDownloadManager::recordNavigationRequest);
    connect(page, &QObject::destroyed, this, &WebEnginePartDownloadManager::removePage);
}

void WebEnginePartDownloadManager::removePage(QObject* page)
{
    const QUrl url = m_requests.key(static_cast<WebEnginePage *>(page));
    m_requests.remove(url);
    m_pages.removeOne(static_cast<WebEnginePage*>(page));
}

void WebEnginePartDownloadManager::performDownload(QWebEngineDownloadItem* it)
{
    WebEnginePage *page = m_requests.take(it->url());
    bool forceNew = false;
    if (!page && !m_pages.isEmpty()) {
        qCDebug(WEBENGINEPART_LOG) << "downloading" << it->url() << "in new window or tab";
        page = m_pages.first();
        forceNew = true;
    } else if (!page) {
        qCDebug(WEBENGINEPART_LOG) << "Couldn't find a part wanting to download" << it->url();
        return;
    }
    page->download(it->url(), forceNew);
}

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
