/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2017 Stefano Crocco <stefano.crocco@alice.it>

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
#include <KParts/BrowserOpenOrSaveQuestion>

class WebEnginePage;
class QWebEngineProfile;

class WebEnginePartDownloadManager : public QObject
{
    Q_OBJECT

public:
    ~WebEnginePartDownloadManager() override;

    WebEnginePartDownloadManager(QWebEngineProfile *profile, QObject *parent = nullptr);

private:

    /**
     * Makes WebEnginePart itself download a file, rather than letting the application do it
     *
     * This is needed in two situations:
     * - when downloading an URL with the `blob` scheme
     * - when the file is the response to a POST request
     *
     * In both cases, this is needed because the application can't repeat the request (in particular,
     * in the case of POST requests, `QtWebEngine` doesn't provide access to the POST data itself).
     *
     * This function uses `KParts::BrowserOpenOrSaveQuestion` to ask the user what to do, then
     * calls saveFile() or openFile() to perform the chosen action.
     * @param it the download item representing the download request
     * @param disposition the argument to pass to `KParts::BrowserOpenOrSaveQuestion::askEmebedOrSave`
     */
    void downloadFile(QWebEngineDownloadItem *it, KParts::BrowserOpenOrSaveQuestion::AskEmbedOrSaveFlags disposition, bool forceNewTab = false);
    QString generateDownloadTempFileName(QString const &suggestedName, const QString &ext) const;

public Q_SLOTS:
    void addPage(WebEnginePage *page);
    void removePage(QObject *page);

private Q_SLOTS:
    void performDownload(QWebEngineDownloadItem *it);
    void saveFile(QWebEngineDownloadItem *it);
    void openFile(QWebEngineDownloadItem *it, WebEnginePage *page, bool forceNewTab = false);
    void downloadToFileCompleted(QWebEngineDownloadItem *it, WebEnginePage *page, bool forceNewTab = false);

private:
    QVector<WebEnginePage*> m_pages;
    QTemporaryDir m_tempDownloadDir;
};

class WebEngineDownloadJob : public KJob
{
    Q_OBJECT

public:
    WebEngineDownloadJob(QWebEngineDownloadItem *it, QObject *parent = nullptr);
    ~WebEngineDownloadJob() override{}

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
