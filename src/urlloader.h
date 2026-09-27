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
 * By default, an URL loader will determine (possibly with the user's intervention) what to do with the URL choosing
 * from: embedding it in a part inside Konqueror, opening it in an external application, executing (if it's an
 * executable file from a trusted source) or saving it to a local file. When constructing the URL loader, some of these
 * actions can be disallowed by calling BrowserArguments::setAllowedActions() with the appropriate actions. This is
 * useful, for example, if the user has already chosen what to do with the URL.
 *
 * @internal
 * For remote files, what happens after goOn() depends on whether the BrowserArguments associated with the request
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

    ~UrlLoader(); //!< Destructor

    using OpenUrlAction = Konq::UrlAction;

    /** @brief Enum describing the view to use to embed an URL*/
    enum class ViewToUse{
        View, //!< Use the view passed as argument to the constructor
        CurrentView, //!< Use the current view
        NewTab //!< Create a new tab and use its view
    };

    /**
     * @brief Attempts to synchronously determine the mimetype of the URL
     *
     * This is the first function to call after creating the UrlLoader when opening an URL.
     *
     * If the mimetype can't be determined synchronously or if the URL must be
     * downloaded using a DownloadJob, after a call to this function, isAsync() will
     * return `true`, meaning that you have to wait before opening the URL.
     *
     * The mimetype can be determined synchronously in the following situations:
     * - a mimetype different from `application/octet-stream` is passed to the constructor
     * - the URL is a local file
     * - the URL scheme is `http`
     *
     * Note that if the KonqOpenURLRequest passed to the constructor has a DownloadJob, the
     * mimetype will never be determined synchronously, even if one of the above conditions
     * is `true`.
     *
     * @note A mimetype of `application/octet-stream` is treated as an unknown mimetype,
     * since itd doesn't give any information about how to open it.
     */
    void start();

    /**
     * @brief Asynchronously determines the mimetype, if needed, and performs the appropriate action
     *
     * If the mimetype of the URL hasn't been determined by start(), it launches an `OpenUrlJob` to do so.
     * Once the mimetype has been determined, it decides what action should be performed to load it (embed,
     * open in an external application, save, execute) and carries it out using performAction().
     *
     * If the `OpenUrlJob` is used, the rest of the function happens asynchronously.
     *
     */
    void goOn();

    /**
     * @brief Carries out the requested action
     *
     * Depending on the mimetype, and possibly by the user choices, this can mean:
     * - embed the URL in the appropriate view
     * - open the URL in an external application
     * - save the URL to file
     * - execute the URL.
     *
     * If needed, a `DownloadJob` is used to download the URL before attempting to load it.
     */
    void performAction();

    /**
     * @brief Stops loading the URL
     *
     * This kills any existing jobs and also deletes the UrlLoader itself.
     */
    void abort();

    /**
     * @brief The mimetype of the URL to load
     *
     * @return the mimetype of the URL to load or an empty string if it's unknown
     */
    QString mimeType() const;

    /**
     * @brief Whether the UrlLoader is ready to load the URL
     *
     * The UrlLoader is ready to load the URL if:
     * - the mimetype has been determined
     * - the URL has been downloaded using a DownloadJob if requested
     *
     * @return `true` if the UrloLoader is ready to load the URL and `false` otherwise
     */
    bool isReady() const {return m_ready;}

    /**
     * @brief The kind of view to use to load the URL
     *
     * By default, the URL is loaded in the view which requested it (the one passed
     * to the constructor), unless using a new tab or window is specifically requested
     * by the KonqOpenURLRequest passed to the constructor. The current view is
     * only used if no view requested to open the URL or if that view is following
     * the current one.
     *
     * @return the kind of view to use to load the URL
     */
    ViewToUse viewToUse() const;

    /**
     * @brief The URL this object is loading
     * @return the URL this object is loading
     */
    QUrl url() const {return m_url;}

    /**
     * @brief The object describing how to load the URL
     * @return the object describing how to load the URL
     */
    KonqOpenURLRequest request() const {return m_request;}

    /**
     * @brief The view to load the URL in
     *
     * This is the same view passed to the constructor unless it has been changed
     * using setView().
     *
     * @return The view to load the URL in. It is `nullptr` if no view has been
     * passed to the constructor and setView() hasn't been called
     */
    KonqView* view() const {return m_view;}

    /**
     * @brief Changes the view to load the URL in
     *
     * @param view the view to open the URL in. It must not be `nullptr`
     */
    void setView(KonqView *view);

    /**
     * @brief Whether or not the URL will start loading asynchronously
     *
     * Loading the URL is always an asynchronous operation, but starting loading
     * it can be either a synchronous or asynchronous operation depending on what
     * information is available when goOn() is called and on whether a DownloadJob
     * should be used.
     *
     * When starting loading the URL is synchronous, after goOn() returns, the
     * part which should display the URL has been created (if needed) and it's
     * `openUrl()` method has been called. If starting loading the URL is asynchronous,
     * instead, goOn() will start some `KJob` and starting loading the URL must
     * wait until the job finishes.
     *
     * @return `true` if the URL will start loading asynchronously and `false` if
     * it will start synchronously
     */
    bool isAsync() const {return m_isAsync;}

    /**
     * @brief Stores the content of the location bar as it was before starting loading the URL
     *
     * @param old the content of the location bar
     */
    void setOldLocationBarUrl(const QString &old);

    /**
     * @brief Whether trying to load the URL resulted in an error
     *
     * @return `true` if there was an error while trying to load the URL and `false`
     * if there were no errors
     */
    bool hasError() const {return m_jobErrorCode;}

    /**
     * @brief Tells the UrlLoader whether the URL should always be loaded in a new
     * tab or not
     *
     * @param newTab `true` if the URL should always be loaded in a new tab and
     * `false` if the UrlLoader can decide whether to load it in a new tab or not
     */
    void setNewTab(bool newTab);

    /**
     * @brief The suggested file name to use when saving the URL to a local file
     *
     * @return the file name to suggest the user when he wants to save the URL to
     * a local file
     */
    QString suggestedFileName() const {return m_request.suggestedFileName;}

    /**
     * @brief The @c ID of the part to use to open a local file
     *
     * @param path the file path
     * @return the plugin id for the preferred part for @p file (as returned by @c KPluginMetaData::pluginId() or
     * an empty string if no part could be found
     */
    static QString partForLocalFile(const QString &path);

    /**
     * @brief The @c ID of the part to use to open an URL with a given mimetype
     *
     * @param mimetype the mimetype to open
     * @return the plugin id for the preferred part for @p mimetype (as returned by @c KPluginMetaData::pluginId() or
     * an empty string if no part could be found
     */
    static QString partForMimetype(const QString &mimetype);

signals:

    /**
     * @brief Signal emitted when the UrlLoader finished its work
     *
     * This signal is emitted when the UrlLoader has passed the task of loading the
     * URL to a part or to a `KJob` or when an error occurs before doing so. This
     * means that when this signal is emitted, the URL won't have already finished
     * loading.
     *
     * @param self the UrlLoader itself
     */
    void finished(UrlLoader *self);

private slots:

    /**
     * @brief Slot called when the mimetype of the URL has been found using the #m_mimeTypeFinderJob job
     *
     * It sets the mimetype according to the job's result, including the case when
     * the job encountered an error.
     */
    void mimetypeDeterminedByJob();

    /**
     * @brief Slot called whenever one of the jobs launched by this instance finishes
     *
     * It records the error code of the job, if any.
     *
     * @param job the job
     */
    void jobFinished(KJob* job);

    /**
     * @brief Slot called when the UrlLoader has finished its work
     *
     * It ensures that the instance is in a consistent state, updates the list of
     * allowed actions in the associated KonqOpenURLRequest, emits the finished()
     * signal and calls `deleteLater()`.
     *
     * @param job the job which concluded the work of the UrlLoader, if any
     */
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

    /**
     * @brief Tries to display the URL in a part inside Konqueror
     *
     * If embedding fails because the view is locked, it calls KonqMainWindow::openUrl()
     * forcing it to open the URL in a new tab. If embedding fails for other reasons
     * it attempts to either save or open the URL in an external application.
     *
     * If embedding succeeds, it calls done() to signal that the UrlLoader has finished
     * its work.
     *
     * @note This function should only be called when all information about how to
     * embed the URL (in particular, the part to use) have already been found.
     */
    void embed();

    /**
     * @brief Attempts to open the URL in an external application
     *
     * The URL is opened using the default application for the mimetype unless a
     * different one has explicitly been chosen by the user.
     *
     * If the application to use is Konqueror itself, it creates a new main window
     * for the URL, forcing the new window to embed the URL. If no part to display
     * the URL is available, an error message is shown and nothing is done.
     *
     * After the application has been launched, or if an error occurs, it calls done()
     * to signal that the UrlLoader has finished its job.
     */
    void open();

    /**
     * @brief Attempts to run the URL as an executable file
     *
     * After the URL has been executed, it calls done() to signal that the UrlLoader
     * has finished its job.
     */
    void execute();

    /**
     * @brief Attempts to save the URL to a local file
     *
     * It asks the user where it wants to save the URL, then calls performSave()
     * to actually do the saving.
     *
     * @warning If a DownloadJob should be used to load the URL, the UrlLoader should
     * be synchronous so that this function is called synchronously because otherwise
     * DownloadJob::startDownload() wouldn't work.
     *
     * After the URL has been executed, it calls done() to signal that the UrlLoader
     * has finished its job.
     */
    void save();

    /**
     * @brief Whether the URL should be embedded or not
     *
     * This function takes into account whether embedding is allowed or forced and
     * the settings for the URL mimetype.
     *
     * @return `true` if the URL should be embedded and `false` if it should not
     * be embedded
     */
    bool shouldEmbedThis() const;

    /**
     * @brief Performs the operations needed to actually save the URL to a local file
     *
     * The download is done using a `KJob`: this is the DownloadJob provided by
     * #m_request or the `KIO::FileCopyJob` returned by `KIO::file_copy()` if no
     * DownloadJob was provided.
     *
     * After the job has completed, it calls done() to signal that the UrlLoader
     * has finished its work.
     */
    void performSave(const QUrl &orig, const QUrl &dest);

    /**
     * @brief Updates information about the URL if it represents an archive file
     *
     * If the URL is an archive file (for example a file with the `.tar` or `.zip`
     * extension) it uses information provided by `KProtocolManager::protocolForArchiveMimetype`
     * to modify the URL and the mimetype so that the file can be handled by parts
     * or application able to display that kind of archive.
     */
    void detectArchiveSettings();

    /**
     * @brief Finds out information about the URL when it represents a local file
     *
     * It determines the mimetype of the URL according to its name, redirects `.desktop`
     * files with type link to the their target and provides an error if the file can't
     * be read.
     */
    void detectSettingsForLocalFiles();

    /**
     * @brief Updates the mimetype for a remote file
     *
     * It sets unknown mimetypes for `konq`, `error`, `data`, `http` and `https`
     * schemes to `text/html` so that it will be processed by WebEnginePart which
     * will take care to detect the real mimetype.
     *
     * It also sets the mimetype of executable test files to `text/plain` unless
     * it comes from a trusted source (according to #m_trustedSource).
     *
     * This function does nothing if the URL represents a local file according to
     * `QUrl::isLocalFile()`.
     */
    void detectSettingsForRemoteFiles();

    /**
     * @brief Launches a job to determine the mimetype of the URL
     *
     * This function takes care of making the necessary signal-slot connections
     * for the job and stores the job itself in #m_mimeTypeFinderJob.
     */
    void launchMimeTypeFinderJob();

    /**
     * @brief Whether the given URL represents a text file which can be executed
     *
     * @return `true` if the URL is a text file which can be executed, which means
     * either a `.desktop` file or a shell script (mimetype `application/x-shellscript`)
     * and `false` otherwise
     */
    static bool isTextExecutable(const QString &mimeType);

    /**
     * @brief Whether the given path is a `.desktop` file which can be executed
     *
     * @param path the path of the local file to test
     * @return `true` if the path corresponds to a `.desktop` file of type
     * `Application` which the user is authorised to execute and `false` otherwise.
     */
    static bool isDesktopFileExecutable(const QString &path);

    /**
     * @brief Whether a service represents the Konqueror application
     *
     * @param service the service to test
     * @return `true` if the service corresponds to the `konqueror` or `kfmclient_*`
     * executables and `false` otherwise
     */
    static bool serviceIsKonqueror(KService::Ptr service);

    /**
     * @brief Whether or not the given string represents a known mimetype
     *
     * @param mimeType the mimetype to test
     * @return `true` if @p mimeType represents a known mimetype and `false`
     * otherwise. Currently, this only returns `false` if @p mimeType is empty
     */
    static bool isMimeTypeKnown(const QString &mimeType);

    /**
     * @brief Determines whether the URL should be embedded, opened in an external
     * application or saved to a file
     *
     * This takes into account which actions are allowed, the user settings and,
     * if needed, explicitly asks the user (using askEmbedSaveOrOpen()).
     *
     * When opening in an external application using a DownloadJob, it also ensures
     * that the file is downloaded in the temporary directory, to ensure that
     * closing Konqueror doesn't delete the file while it's open in the external
     * application.
     *
     * @warning If the externa application doesn't support the `--tempfile` command
     * line option, this will leave the downloaded file in the temporary directory
     * after the application has been closed.
     */
    void decideEmbedOpenOrSave();

    /**
     * @brief Whether or not the default `text/html` mimetype should be used
     *
     * @return `true` if the mimetype of the URL is unknown and its scheme starts
     * with `http` or it's `error`, `konq` or `data` and `false` otherwise.
     */
    bool shouldUseDefaultHttpMimeype() const;

    /**
     * @brief Determines which action should be used to load the URL
     *
     * It determines the part to embed the URL in or the application to open it
     * with then sets the `chosenAction` entry of #m_request.
     */
    void decideAction();

    /**
     * @brief Whether the view associated with this object is locked or not
     *
     * @return `true` if the view associated with this object is locked and `false`
     * if it isn't locked or there isn't a view associated with it
     */
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
     * @brief Determines the mimetype of an URL after it has been downloaded and
     * decides what to do with it
     *
     * Sometimes, the mimetype in the `HTTP` header for an URL is different from the real
     * mimetype of the file (either because the header is wrong or because locally
     * we have a more specific mimetype for the file).
     *
     * This function ensures that decisions about what to do with the URL are taken
     * using the correct mimetype, determined according to the file contents and
     * extension, according to this algorithm:
     * - determine the mime type using the contents
     * - determine the mime type using the extension
     * - if the mime type determined from the extension inherits the one determined by content, use the former, otherwise
     *  use the latter.
     *
     * If the mime type is different from #m_mimeType, it replaces the original one and attempts to determine
     * again what to do with the file. If the determined mime type is `application/octet-stream`, the original mime type
     * is kept.
     *
     * This function is only called if the user had decided to embed or open the file; it isn't called if the user
     * decided to save it (saving doesn't depend on the mime type).
     *
     * @note The algorithm used by this function is different from the one used
     * by `QMimeDataBase::mimeTypeForFile` when called with `QMimeDatabase::MatchDefault`,
     * which only checks the content as a fallback.
     */
    void checkDownloadedMimetype();

    /**
     * @brief Finds the part to use to embed the URL
     *
     * The part is found according to the mimetype and the user preferences.
     *
     * If the `serviceName` variable of #m_request is not empty, that part will be used, regardless of other user
     * preferences. If @p forceServiceName is `false`, that part will only be used if it actually supports the mimetype
     * @param forceServiceName whether to force the use of the part whose name is in `m_request.serviceName`, even if
     * it doesn't support the mimetype
     * @return the metadata representing the chosen part or an empty `KPluginMetaData` if no suitable part can be found
     */
    KPluginMetaData findEmbeddingPart(bool forceServiceName=true) const;

    /**
     * @brief Determines #m_mimeType in the constructor
     *
     * The algorithm used is the following:
     * - if #m_mimeType isn't empty or a default mimetype, use it
     * - if the `KParts::OpenUrlArguments` field of #m_request has a mimetype which isn't empty or
     * the default mimetype, use it
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

    /**
     * @brief Enum describing how to show the contents of an URL
     */
    enum class OpenEmbedMode{
        Open, //!< Open the URL in an external application
        Embed //!< Embed the URL in a part inside Konqueror
    };

    /**
     * @brief Asks the user whether he wants to embed, save or open the URL
     *
     * Since this function uses DownloadActionQuestion, depending on the user
     * settings and the allowed actions, it's possible that a default action has
     * already been chosen: in this case, the user isn't actually asked what to
     * do and the default action will be used.
     *
     * This function sets \link KonqOpenURLRequest::chosenAction m_request.chosenAction\endlink, #m_part,
     * \link KonqOpenURLRequest::serviceName m_request.serviceName\endlink and #m_service.
     * and
     */
    void askEmbedSaveOrOpen();

    /**
     * @brief Determine whether to execute the file or not
     *
     * If the file can be executed, it asks the user whether he really wants to
     * execute it or not, providing an alternative action if he chooses not to
     * execute it, if possible.
     *
     * This function sets \link KonqOpenURLRequest::chosenAction m_request.chosenAction\endlink
     * according to the user's choice.
     */
    void decideExecute();

    /**
     * @brief Determines the application to use to open the URL
     *
     * If an application is specified by \link KonqOpenURLRequest::browserArguments m_request.browserArguments\endlink,
     * that application is used, otherwise the preferred application for the mimetype is used.
     *
     * This function sets #m_service to the correct application. If opening is not
     * an allowed action, #m_service is set to `nullptr` and nothing else is done.
     */
    void findService();

    /**
     * @brief Whether we are allowed to perform the given action
     *
     * @param action the action to check
     * @return `true` if @p action is among the allowed actions and `false` otherwise
     *
     * @see #m_allowedActions
     */
    bool can(OpenUrlAction action) const;

    /**
     * @brief The directory to start the Save As dialog in
     *
     * This directory is determined in the following way:
     * - if the main window has a chosen save directory (according to KonqMainWindow::saveDir), use that
     * - if the main window has a valid property with name #s_lastSaveDirProperty, use the property value.
     *  This is the directory where an URL was last downloaded in this window and it's updated from save().
     * - as a fallback value, use the download directory as determined by `QStandardPaths`
     *
     * @return the directory where to start the Save As dialog in
     */
    QString startingSaveDir() const;

private:
    QPointer<KonqMainWindow> m_mainWindow; //!< The main window

    /**
     * @brief The URL to load
     *
     * When using a DownloadJob to download the URL, after the download has finihsed,
     * this will be the location of the downloaded file; the requested URL can
     * be accessed from #m_originalUrl.
     *
     * @see #m_originalUrl
     */
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

    KonqOpenURLRequest m_request; //!< The object describing how to load the URL
    KonqView *m_view; //!< The view where to embed the URL in

    /**
     * @brief Whether or not the URL comes from a trusted location
     *
     * If this is false, executing the URL will be disabled. If it's an
     * executable text file, it will be treated as if its mimetype were `text/plain`.
     */
    bool m_trustedSource;

    bool m_ready = false; //!< Whether all information required to load the URL are available
    bool m_isAsync = false; //!< Whether starting loading the URL will be synchronous or asynchronous

    KPluginMetaData m_part; //!< The metadata of the part to embed the URL with
    KService::Ptr m_service;//!< The service to open the URL with

    QPointer<KJob> m_launcherJob = nullptr; //!< Job to execute the URL or to open it in an external application

    QPointer<KIO::MimeTypeFinderJob> m_mimeTypeFinderJob = nullptr; //!< The job to find the mimetype of the URL

    QString m_oldLocationBarUrl; //!< The previous content of the location bar
    int m_jobErrorCode = 0; //!< The error code returned by the last run job
    bool m_protocolAllowsReading; //!< Whether the protocol associated to the URL allows reading
    bool m_useDownloadJob = false; ///<Whether the URL should be downloaded using a DownloadJob instead of letting the part displaying it do the download
    QPointer<KonqInterfaces::DownloadJob> m_downloadJob; //!<The job to use to download the URL, if any
    Konq::AllowedUrlActions m_allowedActions; //!< The actions which can be performed on the URL

    /**
     * @brief The URL that the UrlLoader has been asked to open, when using a DownloadJob
     *
     * This is empty when not using a DownloadJob.
     *
     * When using a DownloadJob, this is the same as #m_url until the DownloadJob finishes
     * downloading the URL. At that point, #m_url will point to the downloaded file,
     * while this will keep pointing to the original URL.
     *
     * @see m_url
     */
    QUrl m_originalUrl;

    static const constexpr char* s_lastSaveDirProperty{"lastSaveDir"}; //!< The name of the property where the directory an URL was last downloaded is stored
};

#endif // URLLLOADER_H
