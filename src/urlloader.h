/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef URLLLOADER_H
#define URLLLOADER_H

#include "konqopenurlrequest.h"
#include "downloadactionquestion.h"

#include <QObject>
#include <QUrl>
#include <QPointer>

#include <KService>
#include <KJob>

namespace KParts {
    class ReadOnlyPart;
};

class KJob;
namespace KIO {
    class OpenUrlJob;
    class ApplicationLauncherJob;
    class MimeTypeFinderJob;
}
namespace KonqInterfaces {
    class DownloadJob;
}
class KonqMainWindow;
class KonqView;


/**
 * @brief Class which takes care of finding out what to do with an URL and carries out the chosen action
 *
 * Depending on whether the mimetype of the URL is already known and on whether the URL is a local or remote
 * file, this class can work in a synchronous or asynchronous way. This should be mostly transparent to the user.
 *
 * This class is meant to be used in the following way:
 * - create an instance, passing it the known information about the URL to load
 * - connect to the finished() signal to be notified when the URL has been loaded
 * - call start(): this will attempt to determine the synchronously determine mimetype and, if successful, will
 * decide what to do with it
 * - call viewToUse() to find out where the URL should be opened. If needed, create a new view and call setView()
 * passing the new view
 * - call goOn(): this will asynchronously determine the mimetype and the action to carry out, if not already done,
 * and perform the action itself.
 *
 * @internal
 * For remote files, what happens after goOn depends on whether the BrowserArguments associated with the request
 * provides a DownloadJob:
 * - if it doesn't, this class will determine the mimetype of the URL (if needed), then embed, open or save it
 *   depending on the mimetype and the user's previous choices
 * - if it does, the job will be used to download the URL, then #m_url will be updated so that it contains the path
 *   of the downloaded file, then it proceeds as above
 */
class UrlLoader : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor
     *
     * @param mainWindow the KonqMainWindow which asked to load the URL
     * @param view the view which asked to open the URL. It can be `nullptr`
     * @param url the URL to load
     * @param mimeType the mimetype of the URL or an empty string if not known
     * @param req the object containing information about the URL loading request
     * @param trustedSource whether the source of the URL is trusted
     */
    UrlLoader(KonqMainWindow* mainWindow, KonqView* view, const QUrl& url, const QString& mimeType, const KonqOpenURLRequest& req, bool trustedSource);
    ~UrlLoader();


    using OpenUrlAction = Konq::UrlAction;

    /** @brief Enum describing the view to use to embed an URL*/
    enum class ViewToUse{
        View, /**< Use the view passed as argument to the constructor */
        CurrentView, /**< Use the current view */
        NewTab /**< Create a new tab and use its view */
    };

    /**
     * @brief Determines what to do with the URL if its mimetype can be determined without using an `OpenUrlJob`.
     *
     * When the mimetype can be determined without using an `OpenUrlJob`, this function calls decideAction() to
     * determine what should be done with the URL. In this case, subsequent calls to isReady() will return `true`.
     * If the mimetype can't be determined without using an `OpenUrlJob`, calls to isReady() will return `false`,
     * because `OpenUrlJob` works asynchronously.
     *
     * The mimetype can be determined without using an `OpenUrlJob` in the following situations:
     * - a mimetype different from `application/octet-stream` is passed to the constructor
     * - the URL is a local file
     * - the URL scheme is `http` and the URL hasn't yet been processed by the default HTML engine (in this case,
     * a fake `text/html` mimetype will be used and the HTML engine will take care of determining the mimetype)
     *
     * @note This function *doesn't* create or start the `OpenUrlJob`, even if it will be needed.
     */
    void start();

    /**
     * @brief Performs the required action on the URL, using an `OpenUrlJob` to determine its mimetype if needed.
     *
     * If start() had been able to determine the action to carry out, this function simply calls performAction()
     * to perform the chosen action. In all other cases (that is, if the mimetype is still unknown), it launches
     * an `OpenUrlJob` to determine the mimetype. When the job has determined the mimetype, this function will
     * call decideAction() to decide what to do with the URL and then call performAction() to carry out the chosen
     * action.
     */
    void goOn();

    /**
     * @brief Carries out the requested action
     */
    void performAction();

    void abort();

    QString mimeType() const;
    bool isReady() const {return m_ready;}
    ViewToUse viewToUse() const;
    QUrl url() const {return m_url;}
    KonqOpenURLRequest request() const {return m_request;}
    KonqView* view() const {return m_view;}
    void setView(KonqView *view);
    bool isAsync() const {return m_isAsync;}
    void setOldLocationBarUrl(const QString &old);
    bool hasError() const {return m_jobErrorCode;}
    void setNewTab(bool newTab);

    QString suggestedFileName() const {return m_request.suggestedFileName;}

    /**
     * @brief The @c ID of the part to use to open a local file
     *
     * @param path the file path
     * @return the plugin id for the preferred part for @p file (as returned by @c KPluginMetaData::pluginId() or
     * an empty string if no part could be found
     */
    static QString partForLocalFile(const QString &path);

signals:
    void finished(UrlLoader *self);

private slots:

    void mimetypeDeterminedByJob();
    void jobFinished(KJob* job);
    void done(KJob *job=nullptr);

    /**
     * @brief Slot called when a part which has asked to download itself the URL has finished doing so
     *
     * @param job the DownloadJob used by the part to download the URL
     */
    void downloadForEmbeddingOrOpeningDone(KonqInterfaces::DownloadJob *job, const QUrl &url);

private:

    /**
     * @brief Remove actions which won't be available depending on the mimetype and URL
     *
     * This removes the
     */
    void updateAllowedActions();

    void embed();
    void open();
    void execute();
    void save();

    bool shouldEmbedThis() const;
    void performSave(const QUrl &orig, const QUrl &dest);
    void detectArchiveSettings();
    void detectSettingsForLocalFiles();
    void detectSettingsForRemoteFiles();
    void launchMimeTypeFinderJob();
    static bool isTextExecutable(const QString &mimeType);
    void openExternally();
    void killOpenUrlJob();
    static bool serviceIsKonqueror(KService::Ptr service);
    static bool isMimeTypeKnown(const QString &mimeType);
    void decideEmbedOpenOrSave();
    bool shouldUseDefaultHttpMimeype() const;
    void decideAction();
    bool isViewLocked() const;

    /**
     * @brief Whether the URL we are loading is executable
     *
     * To be executable, all the following must be true:
     * - the URL must be local (we don't want to execute files from the web)
     * - the mimetype should be in a list of known executable types (desktop files, executable files, libraries,
     *  shell scripts)
     * - the executable bit must be set
     * @return `true` if the file is executable and `false` otherwise
     */
    bool isUrlExecutable() const;

    /**
     * @brief Checks whether a file downloaded by a part really has the mime type described by the HTML header
     *
     * This function is called when the part requesting to download an URL wants to perform the download itself,
     * after the download has finished. It attempts to determine the mime type both using the file contents and the
     * file extension. The algorithm used is the following:
     * - determine the mime type using the contents
     * - determine the mime type using the extension
     * - if the mime type determined from the extension inherits the one determined by content, use the former, otherwise
     *  use the latter.
     * This way to determine the mimetype is different from that used by `QMimeDataBase::mimeTypeForFile` when called
     * with `QMimeDatabase::MatchDefault`, which only checks the content as a fallback.
     *
     * If the mime type is different from the original one, it replaces the original one and attempts to determine
     * again what to do with the file. If the determined mime type is `application/octet-stream`, the original mime type
     * is kept.
     *
     * This function is only called if the user had decided to embed or open the file; it isn't called if the user
     * decided to save it (saving doesn't depend on the mime type).
     *
     * @note This function is needed because sometimes the mime type in the HTTP header is different from the actual
     * mimetype of the file.
     */
    void checkDownloadedMimetype();

    /**
     * @brief Finds the part to use to embed the URL
     *
     * The part is found according to the mimetype and the user preferences. If #m_dontPassToWebEnginePart is `true`
     * `webenginepart` won't be returned (even if no other parts are available).
     *
     * If the `serviceName` variable of #m_request is not empty, that part will be used, regardless of other user
     * preferences. If @p forceServiceName is `false`, that part will only be used if it actually supports the mimetype
     * @param forceServiceName whether to force the use of the part whose name is in `m_request.serviceName`, even if
     * it doesn't support the mimetype
     * @return the metadata representing the chosen part or an empty `KPluginMetaData` if no suitable part can be found
     * @see m_dontPassToWebEnginePart
     */
    KPluginMetaData findEmbeddingPart(bool forceServiceName=true) const;

    /**
     * @brief Determines #m_mimeType in the constructor
     *
     * The algorithm used is the following:
     * - if #m_mimeType isn't empty or a default mimetype, use it
     * - if #m_request.args.mimeType() isn't empty or a default mimetype, use it
     * - otherwise use an empty string
     */
    void determineStartingMimetype();

    /**
     * @brief Downloads the URL using the job provided by #m_request, if any
     *
     * If \link BrowserArguments::downloadJob m_request.browserArguments.downloadJob\endlink is `nullptr`,
     * nothing is done
     */
    void downloadForEmbeddingOrOpening();

    /**
     * @brief Checks whether the given file can be read and, in case of a directory, entered into
     *
     * If the @p path can't be read, according to `QFileInfo::isReadable()`, it attempts to determine the reason:
     * - if the parent directory can be entered (according to `QFileInfo::isExecutable()`) and `QFileInfo::exists() returns `false`,
     *   it assumes the file actually doesn't exist
     * - if the parent directory can't be entered (according to `QFileInfo::isExecutable()`), it assumes that the permissions don't
     *   allow reading for the user.
     *
     * @return a `KIO::Error` value if @p can't be read or 0 if it can be read and, in case it's a directory, entered. In particular,
     *   it returns
     *      - `KIO::ERR_DOES_NOT_EXIST` if @p path doesn't exist
     *      - `KIO::ERR_CANNOT_OPEN_FOR_READING` if @p path exists but can't be read
     *      - `ERR_CANNOT_ENTER_DIRECTORY` if @p path is a directory which can't be entered
     */
    static int checkAccessToLocalFile(const QString &path);

    typedef QPair<OpenUrlAction, KService::Ptr> OpenSaveAnswer;

    enum class OpenEmbedMode{Open, Embed};

    void askEmbedSaveOrOpen();

    void decideExecute();

    void findAvailableParts();

    void findService();

    /**
     * @brief Whether we are allowed to perform the given action
     * @param action the action to check
     * @return `true` if #m_allowedActions contains @p action and `false` otherwise
     */
    bool can(OpenUrlAction action) const;

private:
    QPointer<KonqMainWindow> m_mainWindow;
    QUrl m_url;

    /**
     * @brief The mimetype of the URL
     *
     * If this is empty, it means that the mimetype isn't known and must be determined somehow.
     * If this is not empty, it is assumed that its value is the correct mimetype for the URL and
     * no other attempts to determine the mimetype will be done (even if this is the default mimetype).
     * @see launchMimeTypeFinderJob()
     * @see mimetypeDeterminedByJob()
     */
    QString m_mimeType;
    KonqOpenURLRequest m_request;
    KonqView *m_view;
    bool m_trustedSource;
    bool m_ready = false;
    bool m_isAsync = false;
    KPluginMetaData m_part;
    KService::Ptr m_service;
    QPointer<KIO::OpenUrlJob> m_openUrlJob;
    QPointer<KIO::ApplicationLauncherJob> m_applicationLauncherJob;
    QPointer<KIO::MimeTypeFinderJob> m_mimeTypeFinderJob;
    QString m_oldLocationBarUrl;
    int m_jobErrorCode = 0;
    bool m_ignoreDefaultHtmlPart;
    bool m_protocolAllowsReading;
    bool m_useDownloadJob = false; ///<Whether the URL should be downloaded by the part before opening/embedding/saving it
    QPointer<KonqInterfaces::DownloadJob> m_downloadJob; //!<The job to use to download the URL, if any
    /**
     * @brief The allowed actions for the url
     */
    Konq::AllowedUrlActions m_allowedActions;

    /**
     * @brief Whether only the `embed` action is allowed
     *
     * If `true`, the only action performed will be to embed the URL. If that action can't be performed for any reason (including
     * not having an available part able to display the URL or an error happening), nothing will be done (including displaying error messages).
     *
     * This variable is set if the metadata in the `KonqOpenURLRequest` passed to the constructor contains the `"EmbedOrNothing"` entry.
     *
     * @note This is a workaround to allow WebEnginePart to display a downloaded file without risking asking again the user to save
     *
     * @todo Remove this after a better way to allow the user to quickly display a file after downloading it has been implemented. See comment
     * for WebEnginePage::saveUrlToDiskAndDisplay
     */
    // bool m_embedOrNothing = false;

    /**
     * @brief The URL that the UrlLoader has been asked to open, in case the requesting parts wants to download it itself
     *
     * If the requesting part doesn't want to download the URL itself, this is empty. Otherwise, when the UrlLoader is created,
     * this is the same as #m_url. After the part has finished downloading the URL, #m_url will contain the temporary local file
     * the URL has been downloaded to, while #m_originalUrl will not be changed
     */
    QUrl m_originalUrl;
};

#endif // URLLLOADER_H
