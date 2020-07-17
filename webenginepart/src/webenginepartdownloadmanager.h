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
#include <QWebEngineDownloadItem>

#include <KJob>

class WebEnginePage;
class QFile;

class WebEnginePartDownloadManager : public QObject
{
    Q_OBJECT

public:
    static WebEnginePartDownloadManager* instance();

    ~WebEnginePartDownloadManager() override;

private:
    WebEnginePartDownloadManager();
    void downloadBlob(QWebEngineDownloadItem *it);

public Q_SLOTS:
    void addPage(WebEnginePage *page);
    void removePage(QObject *page);

private Q_SLOTS:
    void performDownload(QWebEngineDownloadItem *it);
    void saveBlob(QWebEngineDownloadItem *it);
    void embedBlob(QWebEngineDownloadItem *it);
    void openBlob(QWebEngineDownloadItem *it);
    void blobDownloadedToFile(QWebEngineDownloadItem *it);

#ifndef DOWNLOADITEM_KNOWS_PAGE
private:
    WebEnginePage* pageForDownload(QWebEngineDownloadItem *it);

private Q_SLOTS:
    void recordNavigationRequest(WebEnginePage* page, const QUrl& url);
#endif

private:
    QVector<WebEnginePage*> m_pages;
#ifndef DOWNLOADITEM_KNOWS_PAGE
    QHash<QUrl, WebEnginePage*> m_requests;
#endif
};

class WebEngineBlobDownloadJob : public KJob
{
    Q_OBJECT

public:
    WebEngineBlobDownloadJob(QWebEngineDownloadItem *it, QObject *parent = nullptr);
    ~WebEngineBlobDownloadJob(){}

    void start() override;

    QString errorString() const override;

protected:
    bool doKill() override;
    bool doResume() override;
    bool doSuspend() override;

private slots:
    void downloadProgressed(quint64 received, quint64 total);
    void stateChanged(QWebEngineDownloadItem::DownloadState state);

private:

    QWebEngineDownloadItem *m_downloadItem;

};

#endif // WEBENGINEPARTDOWNLOADMANAGER_H
