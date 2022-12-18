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
#include <QSet>

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

    /**
     * @brief Tells the download manager that the next time the given page requests to download a specific URL, it should perform the
     * download by itself and not delegate it to the main application
     *
     * This is needed to ensure that if the user choses, for example "Save As..." the page or document will actually be saved and not
     * opened with an external application or embedded.
     *
     * @param url the url this setting applies to
     * @param page the page this setting applies to. Requests to download @p url made by pages other than @p page will be processed as
     * usual
     */
    void setForceDownload(const QUrl &url, WebEnginePage *page);

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

    /**
     * @brief Checks whether a download request for the given URL by the given page should be treated as a forced download or not
     *
     * @warning This function @b should only be called @b once for each download, because it removes information about the download from the internal
     * state (this is for performance reasons).
     * @param url the url to download
     * @param page the page which requested the download
     * @return `true` if the page should be downloaded by the download manager and `false` if it should be processed as usual
     */
    bool checkForceDownload(const QUrl &url, WebEnginePage *page);

    /**
     * @brief Saves a full HTML page, after asking the user which formats he wants to use
     *
     * The available formats are described in `QWebEngineDownloadItem::SavePageFormat`
     * @param it the item to use for the download
     * @param page the page which requested the download
     */
    void saveHtmlPage(QWebEngineDownloadItem *it, WebEnginePage *page);

public Q_SLOTS:
    void addPage(WebEnginePage *page);
    void removePage(QObject *page);

private Q_SLOTS:
    void performDownload(QWebEngineDownloadItem *it);
    void saveFile(QWebEngineDownloadItem *it);
    void openFile(QWebEngineDownloadItem *it, WebEnginePage *page, bool forceNewTab = false);
    void downloadToFileCompleted(QWebEngineDownloadItem *it, WebEnginePage *page, bool forceNewTab = false);
    void startDownloadJob(const QString &file, QWebEngineDownloadItem *it);

private:
    QVector<WebEnginePage*> m_pages;
    QTemporaryDir m_tempDownloadDir;

    /**
     * @brief Urls which the download manager should download by itself rather than passing them to the application, when the download is requested by the appropriate page
     */
    QMultiHash<QUrl, QPointer<WebEnginePage>> m_forcedDownloads;
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
