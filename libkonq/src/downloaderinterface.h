/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef DOWNLOADERINTERFACE_H
#define DOWNLOADERINTERFACE_H

#include <KJob>

#include <libkonq_export.h>

class QUrl;

/**
 * @brief Interface for parts which need to download files themselves before they're opened or embedded instead of letting Konqueror do it
 *
 * This interface only provides one method, downloadJob(), which returns a DownloaderJob, a `KJob` whose task is to perform the download.
 * Most likely, when creating a class implementing this interface, you'll also need to create a concrete class deriving from DownloadJob.
 *
 * To be allowed to handle the download, the part must emit the `BrowserExtension::openUrlRequest` signal adding to entries to the metadata
 * of the `OpenUrlArguments` passed to the signal. These entries are:
 * - requestDownloadByPartKey() (with any value): this tells Konqueror that the parts want to download the URL itself
 * - jobIDKey(): see the documentation for the @p id parameter in downloadJob(). Implementations are free to decide how to associate IDs with
 *  download requests
 *
 * @note The part doesn't need to implement this interface itself: it can be implemented by any of its children.
 */
class LIBKONQ_EXPORT DownloaderInterface
{
public:

    /**
     * @brief Destructor
     */
    virtual ~DownloaderInterface();

    /**
     * @brief Retrieves a the `KJob` to download the given URL.
     *
     * This method is called by Konqueror in response to a `BrowserExtension::openUrlRequest` signal
     * whose metadata include the key requestDownloadByPartKey(). This method will return a job which, when started,
     * will download the URL.
     *
     * Implementations of this method may return a new job or an existing job. @p id is a number
     * used to distinguish between different download operations for the same URL (in case, for example,
     * the user asks a second download of the same file before the first one has finished).
     *
     * @param url the URL to download
     * @param id the value in the entry with key jobIDKey() in the metadata passed to the `openUrlRequest` signal,
     * converted to an `uint`. It's used to distinguish different downloads for the same URLs. If this method is called
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
     * @brief Returns the given object or any of its children casted to a DownloaderInterface*
     *
     * This is essentially as `QObject::findChildren` except that it works even if DownloaderInterface
     * itself isn't a `QObject`.
     * @param obj the object which should be cast or have one of its children cast to a DownloaderInterface
     * @return @p obj or one of its children cast to a DownloaderInterface or `nullptr` if neither @p obj
     * nor any of its children are derived from DownloaderInterface
     */
    static DownloaderInterface *interface(QObject* obj);

    /**
     * @brief The key to insert in the metadata to tell Konqueror that the part wants handle itself the download
     */
    static QString requestDownloadByPartKey();

    /**
     * @brief The metadata key for the entry holding the download id
     * @see downloadJob()
     */
    static QString jobIDKey();
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
     * @brief The path where the URL will be saved
     * @return the path where the URL will be saved. This path is decided by the
     * part requesting the download
     */
    virtual QString downloadPath() const = 0;

    /**
     * @brief Whether or not the download has finished
     */
    virtual bool finished() const = 0;
};


#endif // DOWNLOADERINTERFACE_H
