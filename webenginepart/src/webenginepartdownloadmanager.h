/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2017 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef WEBENGINEPARTDOWNLOADMANAGER_H
#define WEBENGINEPARTDOWNLOADMANAGER_H

#include <QObject>
#include <QVector>
#include <QTemporaryDir>
#include <QDateTime>
#include <QSet>
#include <KJob>
#include <QPointer>

#include "qtwebengine6compat.h"

#include "browseropenorsavequestion.h"
#include "interfaces/downloaderextension.h"

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

private:

    static QString generateDownloadTempFileName(QString const &suggestedName, const QString &ext);

    /**
     * @brief Checks whether a download request for the given URL by the given page should be treated as a forced download or not
     *
     * @warning This function @b should only be called @b once for each download, because it removes information about the download from the internal
     * state (this is for performance reasons).
     * @param req the download request
     * @param page the page which requested the download
     * @return `true` if the page should be downloaded by the download manager and `false` if it should be processed as usual
     */
    bool checkForceDownload(QWebEngineDownloadRequest *req, WebEnginePage *page);

    /**
     * @brief Saves a full HTML page, after asking the user which formats he wants to use
     *
     * The available formats are described in `QWebEngineDownloadRequest::SavePageFormat`
     * @param it the item to use for the download
     * @param page the page which requested the download
     */
    void saveHtmlPage(QWebEngineDownloadRequest *it, WebEnginePage *page);

    static QTemporaryDir& tempDownloadDir();

public Q_SLOTS:
    void addPage(WebEnginePage *page);
    void removePage(QObject *page);

private Q_SLOTS:
    void performDownload(QWebEngineDownloadRequest *it);


private:
    QVector<WebEnginePage*> m_pages;

    /**
     * @brief Urls which the download manager should download by itself rather than passing them to the application, when the download is requested by the appropriate page
     */
    QMultiHash<QUrl, QPointer<WebEnginePage>> m_forcedDownloads;
};

class WebEngineDownloadJob : public KonqInterfaces::DownloaderJob
{
    Q_OBJECT

public:
    WebEngineDownloadJob(QWebEngineDownloadRequest *it, QObject *parent = nullptr);
    ~WebEngineDownloadJob() override;

    void start() override;

    QString errorString() const override;

    bool setDownloadPath(const QString & path) override;
    QString downloadPath() const override;
    bool canChangeDownloadPath() const override;
    bool finished() const override;

    QWebEngineDownloadRequest* item() const;

protected:
    bool doKill() override;
    bool doResume() override;
    bool doSuspend() override;

private slots:
#if QT_VERSION_MAJOR < 6
    void downloadProgressed(qint64 received, qint64 total);
#else
    void downloadProgressed();
#endif

    void stateChanged(QWebEngineDownloadRequest::DownloadState state);
    void startDownloading();
    void downloadFinished();

private:
    bool m_started = false; ///<! @brief Whether the job has been started or not
    QPointer<QWebEngineDownloadRequest> m_downloadItem;
    QDateTime m_startTime;
};

#endif // WEBENGINEPARTDOWNLOADMANAGER_H
