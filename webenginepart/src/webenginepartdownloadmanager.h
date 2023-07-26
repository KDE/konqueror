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

#include "browseropenorsavequestion.h"
#include <downloaderinterface.h>

class WebEnginePage;
class QWebEngineProfile;
class WebEngineDownloadJob;

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

    /**
     * @brief Sets up a new download job
     * @overload
     * @param file the path of the downloaded file
     * @param it the object which will perform the actual download
     */
    // void startDownloadJob(const QString &file, QWebEngineDownloadItem *it);

    static WebEngineDownloadJob* createDownloadJob(QWebEngineDownloadItem *it, QObject *parent = nullptr);

private:

    static QString generateDownloadTempFileName(QString const &suggestedName, const QString &ext);

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

    static QTemporaryDir& tempDownloadDir();

public Q_SLOTS:
    void addPage(WebEnginePage *page);
    void removePage(QObject *page);

private Q_SLOTS:
    void performDownload(QWebEngineDownloadItem *it);


private:
    QVector<WebEnginePage*> m_pages;

    /**
     * @brief Urls which the download manager should download by itself rather than passing them to the application, when the download is requested by the appropriate page
     */
    QMultiHash<QUrl, QPointer<WebEnginePage>> m_forcedDownloads;
};

class WebEngineDownloadJob : public DownloaderJob
{
    Q_OBJECT

public:
    WebEngineDownloadJob(QWebEngineDownloadItem *it, QObject *parent = nullptr);
    ~WebEngineDownloadJob() override;

    void start() override;

    QString errorString() const override;

    QString downloadPath() const override;
    bool finished() const override;

    QWebEngineDownloadItem* item() const;

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
    bool m_started = false; ///<! @brief Whether the job has been started or not
    QPointer<QWebEngineDownloadItem> m_downloadItem;
};

#endif // WEBENGINEPARTDOWNLOADMANAGER_H
