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
#include <QWebEngineDownloadRequest>
#include <KJob>
#include <QPointer>

#include "interfaces/downloadjob.h"

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
     * @brief Enum which describes what the user wants to do with a URL once it has been downloaded
     *
     * Currently there are three possibilities:
     * - the user wants to open the URL in the application embedding the part (for example, when
     *  clicking on a link). This is the default download objective
     * - the user wants to save locally the file represented by the URL (for example, when using
     *  the "save link as..." context menu entry
     * - the user wants to save the currently displayed page locally (for example, when using the "Save as"
     *  menu entry
     */
    enum class DownloadObjective {
        OpenInApplication, //!< A URL is to be downloaded to be opened in the application
        SaveAs, //!< The current page is to saved locally
        SaveOnly //!< A URL is to be saved locally
    };

    /**
     * @brief Tells the download manager that the next download of the given URL for the given page
     * has a special objective
     *
     * This is needed to ensure that if the user choses, for example "Save As..." the page or document will actually be saved and not
     * opened with an external application or embedded.
     *
     * By default, downloads are considered having the \link DownloadObjective::OpenInApplication OpenInApplication\endlink objective;
     * this function is only needed to specify a different objective.
     *
     * @param url the url this setting applies to
     * @param page the page this setting applies to. Requests to download @p url made by pages other than @p page will be processed as
     * usual
     * @param objective the special objective for the download
     */
    void specifyDownloadObjective(const QUrl &url, WebEnginePage *page, DownloadObjective objective);

private:

    static QString generateDownloadTempFileName(QString const &suggestedName, const QString &ext);

    /**
     * @brief Struct encapsulating a download objective with the page it refers to
     */
    struct DownloadObjectiveWithPage {
        QPointer<WebEnginePage> page = nullptr; //!< The page requesting the download
        DownloadObjective downloadObjective = DownloadObjective::OpenInApplication; //!< The objective of the download
        bool operator==(const DownloadObjectiveWithPage &other) const {return page == other.page && downloadObjective == other.downloadObjective;} //!< Equality operator
    };

    /**
     * @brief Retrieves the special objective for a download, if any
     *
     * This function is used to retrieve data set using specifyDownloadObjective().
     *
     * @param req the url to download
     * @param page the page requesting the download
     * @return the specified objective for the URL in @p url and page @p page, if any, and
     *  \link DownloadObjective::OpenInApplication OpenInApplication\endlink otherwise
     */
    DownloadObjective fetchDownloadObjective(const QUrl &url, WebEnginePage *page);

    /**
     * @brief Saves a full HTML page, after asking the user which formats he wants to use
     *
     * The available formats are described in `QWebEngineDownloadRequest::SavePageFormat`
     * @param it the item to use for the download
     * @param page the page which requested the download
     */
    void saveHtmlPage(QWebEngineDownloadRequest *it, WebEnginePage *page);

    static QTemporaryDir& tempDownloadDir();

    /**
     * @brief Asks Konqueror to download the given URL using KonqInterfaces::Browser::openUrl()
     *
     * This should only be called when there's no part associated with a download request
     * @param url the URL to download
     * @param mimeType the mimetype of the URL
     */
    bool downloadWithoutPart(const QUrl &url, const QString &mimeType);

public Q_SLOTS:
    void addPage(WebEnginePage *page);
    void removePage(QObject *page);

private Q_SLOTS:
    void performDownload(QWebEngineDownloadRequest *it);


private:
    QVector<WebEnginePage*> m_pages;

    /**
     * @brief Information about download objectives
     *
     * @see specifyDownloadObjective
     * @see fetchDownloadObjective
     * @todo See wether it would be possible to use both url and page as key
     */
    QMultiHash<QUrl, DownloadObjectiveWithPage> m_downloadObjectives;
};

class WebEngineDownloadJob : public KonqInterfaces::DownloadJob
{
    Q_OBJECT

public:
    WebEngineDownloadJob(QWebEngineDownloadRequest *it, QObject *parent = nullptr);
    ~WebEngineDownloadJob() override;

    QUrl url() const override;

    void start() override;

    QString errorString() const override;

    bool setDownloadPath(const QString & path) override;
    QString downloadPath() const override;
    bool canChangeDownloadPath() const override;
    bool finished() const override;

    QWebEngineDownloadRequest* item() const;

    Intent intent() const override;
    void setIntent(Intent intent) override;

    /**
     * @brief Whether this job has been created while handling the "Save As..." menu entry
     *
     * This is used by WebEnginePart::displayActOnDownloadedFileBar. When the user chooses
     * the "Save As..." menu entry, the newly saved file is always displayed in the current
     * part, if possible, which means in this case we don't want to also display the action bar
     * @return `true` if the job has been created in response to the "Save As..." menu entry and
     * `false` otherwise.
     */
    bool calledForSaveAs() const;

    /**
     * @brief Sets whether this job has been created while handling the "Save As..." menu entry
     * @param saveAs `true` if the job has been created while handling the "Save As..." menu entry
     * and `false` (the default) otherwise
     */
    void setCalledForSaveAs(bool saveAs);

protected:
    bool doKill() override;
    bool doResume() override;
    bool doSuspend() override;

private slots:
    void downloadProgressed();
    void stateChanged(QWebEngineDownloadRequest::DownloadState state);
    void startDownloading();
    void downloadFinished();
    void emitDownloadResult(KJob *job);

private:
    bool m_started = false; //!< Whether the job has been started or not
    Intent m_intent = Unknown; //!< The intent of the download
    bool m_calledForSaveAs = false; //!< Whether this job has been created while handling the "Save As..." menu entry
    QPointer<QWebEngineDownloadRequest> m_downloadItem;
    QDateTime m_startTime;
};

#endif // WEBENGINEPARTDOWNLOADMANAGER_H
