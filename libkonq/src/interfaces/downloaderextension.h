/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef DOWNLOADEREXTENSION_H
#define DOWNLOADEREXTENSION_H

#include <KJob>
#include <KParts/OpenUrlArguments>
#include <KParts/BrowserArguments>

#include <QObject>

#include <libkonq_export.h>

class QUrl;

namespace KParts {
    class ReadOnlyPart;
}

namespace KonqInterfaces {

/**
 * @brief Interface for parts which need to download files themselves before they're opened or embedded instead of letting Konqueror do it
 *
 * This interface provides two pure virtual methods, downloadJob(), which returns a DownloaderJob, a `KJob` whose task is to perform the download and part(),
 * and a signal, downloadAndOpenUrl(), to be used instead of `KParts::BrowserExtension::openUrlRequest`.
 *
 * Most likely, when creating a class implementing this interface, you'll also need to create a concrete class deriving from DownloadJob.
 *
 * @warning When a part requests to download the URL itself, Konqueror doesn't try to auto-detect the mimetype of the URL, assuming that the
 * part has already tried to do so and passed the correct value to downloadAndOpenUrl().
 *
 * @note The part doesn't need to implement this interface itself: it can be implemented by any of its children.
 */
class LIBKONQ_EXPORT DownloaderExtension : public QObject
{
    Q_OBJECT

public:

    /**
     * @brief Constructor
     * @param parent the parent object
     */
    DownloaderExtension(QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    virtual ~DownloaderExtension();

    /**
     * @brief Retrieves a the `KJob` to download the given URL.
     *
     * This method is called by Konqueror in response to a downloadAndOpenUrl() signal
     * This method will return a job which, when started, will download the URL.
     *
     * Implementations of this method may return a new job or an existing job. @p id is a number
     * used to distinguish between different download operations for the same URL (in case, for example,
     * the user asks a second download of the same file before the first one has finished).
     *
     * @param url the URL to download
     * @param id the id of the download. It's used to distinguish different downloads for the same URLs. If this method is called
     * multiple times with the same @p url and @p id, implementations can either return the same job or
     * different jobs. They're _required_ to return different jobs when called with different @p url
     * and/or @p id.
     * @param parent the object to give as parent to the returned object. If this method is called multiple
     * times with the same @p url and @p id, and the implementation returns the same job, this will be
     * ignored in all calls after the first
     * @return the job to use to perform the download or `nullptr` if such a job can't be created. In theory,
     * this should never happen, unless this method is called for an URL for which the part didn't request
     * a download.
     */
    virtual class DownloaderJob* downloadJob(const QUrl &url, quint32 id, QObject *parent=nullptr)=0;

    /**
     * @brief The part associated with the extension
     * @return The part the extension belongs to
     */
    virtual KParts::ReadOnlyPart* part() const = 0;

    /**
     * @brief Returns the given object or any of its children casted to a DownloaderInterface*
     *
     * This is essentially as `QObject::findChildren` except that it works even if DownloaderInterface
     * itself isn't a `QObject`.
     * @param obj the object which should be cast or have one of its children cast to a DownloaderInterface
     * @return @p obj or one of its children cast to a DownloaderInterface or `nullptr` if neither @p obj
     * nor any of its children are derived from DownloaderInterface
     */
    static DownloaderExtension *downloader(QObject* obj);

signals:

    /**
     * @brief Signal emitted by the extension to signal the application to open an URL after the part has finished
     * downloading the URL
     *
     * It has the same role of `KParts::BrowserExtension::openUrlRequest`. The main difference (aside from the @p downloadId argument)
     * is the fact that, unlike `KParts::BrowserExtension::openUrlRequest`, the handler for this signal is called synchronously.
     * This is important for the `WebEnginePart` implementation, where it allows to set the download path before having to accept the
     * download request.
     *
     * @param url the URL to download
     * @param downloadId an unique identifier for the download
     * @param arguments information about how to open the URL
     * @param browserArguments other information about how to open the URL
     * @param temp whether the downloaded file, if opened or embedded, should be deleted after use. This will be ignored if the user chooses to save the URL
     */
    void downloadAndOpenUrl(const QUrl &url, quint32 downloadId, const KParts::OpenUrlArguments &arguments = KParts::OpenUrlArguments(),
                            const KParts::BrowserArguments &browserArguments = KParts::BrowserArguments(), bool temp=true);
};

/**
 * @brief Class allowing a part to perform a download
 */
class LIBKONQ_EXPORT DownloaderJob : public KJob {
    Q_OBJECT
public:

    /**
     * @brief Default constructor
     * @param parent the parent object
     */
    DownloaderJob(QObject* parent=nullptr);

    /**
     * @brief destructor
     */
    ~DownloaderJob() override {};

    /**
     * @brief Changes the path where the URL will be downloaded.
     *
     * @warning This function must be called from within a slot (or lambda) connected to the
     * `BrowserExtension::openUrlRequest` emitted by the part and before `start()` is called.
     * If called in other circumstances, whether it has any effect or not is up to the implementation.
     * @param path the new download path. It must be an absolute path.
     * @return `true` if the download path has been changed succesfully and `false` otherwise
     */
    virtual bool setDownloadPath(const QString &path) = 0;

    /**
     * @brief The path where the URL will be saved
     * @return the path where the URL will be saved. This path is decided by the
     * part requesting the download
     */
    virtual QString downloadPath() const = 0;

    /**
     * @brief Whether or not the download path can be changed
     *
     * Implementations are required to allow changing the download path from the slot connected
     * with the `BrowserExtension::openUrlRequest` signal and only before the job is started.
     * Implementations may also allow to change the download path later.
     * @return `true` if the download path can be changed and `false` otherwise.
     */
    virtual bool canChangeDownloadPath() const = 0;

    /**
     * @brief Whether or not the download has finished
     */
    virtual bool finished() const = 0;
};
}

#endif // DOWNLOADEREXTENSION_H
