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

#ifndef WEBENGINEPARTDOWNLOADMANAGER_H
#define WEBENGINEPARTDOWNLOADMANAGER_H

#include <QObject>
#include <QHash>
#include <QVector>

class WebEnginePage;
class QWebEngineDownloadItem;

class WebEnginePartDownloadManager : public QObject
{
    Q_OBJECT

public:
    static WebEnginePartDownloadManager* instance();

    ~WebEnginePartDownloadManager() override;

public Q_SLOTS:
    void addPage(WebEnginePage *page);
    void removePage(QObject *page);

private:
    WebEnginePartDownloadManager();
    WebEnginePage* pageForDownload(QWebEngineDownloadItem *it);

private Q_SLOTS:
    void performDownload(QWebEngineDownloadItem *it);
    void recordNavigationRequest(WebEnginePage* page, const QUrl& url);

private:
    QVector<WebEnginePage*> m_pages;
    QHash<QUrl, WebEnginePage*> m_requests;
};

#endif // WEBENGINEPARTDOWNLOADMANAGER_H
