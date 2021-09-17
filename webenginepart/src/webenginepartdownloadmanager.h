/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2017 Stefano Crocco <posta@stefanocrocco.it>

    SPDX-License-Identifier: LGPL-2.1-or-later

*/

#ifndef WEBENGINEPARTDOWNLOADMANAGER_H
#define WEBENGINEPARTDOWNLOADMANAGER_H

#include <QObject>
#include <QVector>
#include <QWebEngineDownloadItem>
#include <QTemporaryDir>
#include <QDateTime>

#include <KJob>

class WebEnginePage;
class QWebEngineProfile;

class WebEnginePartDownloadManager : public QObject
{
    Q_OBJECT

public:
    ~WebEnginePartDownloadManager() override;

    WebEnginePartDownloadManager(QWebEngineProfile *profile, QObject *parent = nullptr);

private:
    void downloadBlob(QWebEngineDownloadItem *it);
    QString generateBlobTempFileName(QString const &suggestedName, const QString &ext) const;

public Q_SLOTS:
    void addPage(WebEnginePage *page);
    void removePage(QObject *page);

private Q_SLOTS:
    void performDownload(QWebEngineDownloadItem *it);
    void saveBlob(QWebEngineDownloadItem *it);
    void openBlob(QWebEngineDownloadItem *it, WebEnginePage *page);
    void blobDownloadedToFile(QWebEngineDownloadItem *it, WebEnginePage *page);

private:
    QVector<WebEnginePage*> m_pages;
    QTemporaryDir m_tempDownloadDir;
};

class WebEngineBlobDownloadJob : public KJob
{
    Q_OBJECT

public:
    WebEngineBlobDownloadJob(QWebEngineDownloadItem *it, QObject *parent = nullptr);
    ~WebEngineBlobDownloadJob() override{}

    void start() override;

    QString errorString() const override;

protected:
    bool doKill() override;
    bool doResume() override;
    bool doSuspend() override;

private slots:
    void downloadProgressed(quint64 received, quint64 total);
    void stateChanged(QWebEngineDownloadItem::DownloadState state);
    void startDownloading();
    void downloadFinished();

private:
    QWebEngineDownloadItem *m_downloadItem;
    QDateTime m_startTime;
};

#endif // WEBENGINEPARTDOWNLOADMANAGER_H
