/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef DOWNLOADJOB_H
#define DOWNLOADJOB_H

#include <KJob>
#include <KParts/OpenUrlArguments>

#include <QObject>
#include <QUrl>

#include "browserarguments.h"

#include <libkonq_export.h>

class QUrl;

namespace KParts {
    class ReadOnlyPart;
}

namespace KonqInterfaces {

/**
 * @brief Class allowing a part to perform a download
 */
class LIBKONQ_EXPORT DownloadJob : public KJob {
    Q_OBJECT
public:

    enum Intent {
        Unknown, //!< We don't know what this download will be used for
        Save, //!< This download is for storing a file on disk
        Embed, //!< This download is to create a temporary local copy of the URL before embedding it in Konqueror
        Open, //!< This download is to create a temporary local copy of the URL before opening it in an external application
    };

    /**
     * @brief Default constructor
     * @param parent the parent object
     */
    DownloadJob(QObject* parent=nullptr);

    /**
     * @brief destructor
     */
    ~DownloadJob() override {};

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

    /**
     * @brief The URL to download
     * @return the URL to download
     */
    virtual QUrl url() const = 0;

    /**
     * @brief Returns the intent of the download
     *
     * @note This is just an intent: there's no warrant that the download will actually be used
     * this way (for example, the intent may be to embed it, but an error could
     * occur while creating a part for it and the downloaded file could be opened in an external
     * application as a result).
     * @note Users of this method should also check that the job completed with no errors.
     * @return the intent of the download
     */
    virtual Intent intent() const = 0;

    /**
     * @brief Sets the intent for the download
     *
     * The user of this class should call this function before calling start(). If this isn't
     * called, the intent is Intent::Unknown.
     *
     * @note It is allowed for the downloaded file to be used in a different way than what set
     * here, but this should only happen if something unexpected happens after the job has
     * been started
     * @param intent the intent of the download
     */
    virtual void setIntent(Intent intent) = 0;

    /**
     * @brief Convenience function to start the download
     *
     * This sets the job up, setting the Ui delegate and the job tracker interface for the job and changing the
     * download path, connects the \link DownloadJob::downloadResult downloadResult \endlink signal to the given
     * functor and starts the job.
     *
     * @param destPath the path where the URL should be downloaded. If empty, the download path won't be changed
     * @param widget the widget to use for the Ui delegate
     * @param context the receiver or the context of the signal (depending on whether functor is actually a lambda or functor
     *  or a pointer to member function)
     * @param functor the lambda, functor or pointer to member function to connect to the
     * \link DownloadJob::downloadResult downloadResult \endlink signal
     * @warning In Qt5, functor cannot be a member function. If it is, no connection is done
     */
    template <typename Functor>
    void startDownload(const QString &destPath, QWidget *widget, QObject *context, Functor functor);

    /**
     * @brief Convenience function to start the download
     *
     * @overload
     *
     * As startDownload(const QString &, QWidget*, QObject*, Functor), except that it doesn't change the download path
     * @see startDownload(const QString &, QWidget*, QObject*, Functor)
     */
    template <typename Functor>
    void startDownload(QWidget *widget, QObject *context, Functor functor);

private:

    /**
     * @brief Helper function called by startDownload()
     *
     * It sets up the job UI delegate and tracker interface and, optionally, changes the
     * download destination.
     *
     * @param widget the widget to use for the Ui delegate
     * @param destPath the new download destination. If empty, the download destination won't be changed
     */
    void prepareDownloadJob(QWidget *widget, const QString &destPath={});

signals:

    /**
     * @brief Signal emitted when a download has finished
     *
     * This is a convenience signal which is emitted in response to `KJob::result()`. It
     * provides a the job already cast to DownoaderJob and the local URL where the remote
     * URL was saved to. You can connect to this signal as you would to `KJob::result()`.
     *
     * @param job a pointer to the job. It's the same as the `job` argument in `KJob::result()`
     * but you don't need to cast it to a DownloadJob
     * @param url the URL where the remote URL was saved to. There are three possibilities:
     *  - if an error occurred, it will have the `error` scheme
     *  - if the user canceled the download, it will be empty
     *  - if the download was completed successfully, it will be a local URL
     */
    void downloadResult(DownloadJob *job, const QUrl &url);

    void started(DownloadJob *job);
};

}

template<typename Functor>
void KonqInterfaces::DownloadJob::startDownload(const QString& destPath, QWidget* widget, QObject* context, Functor functor)
{
    prepareDownloadJob(widget, destPath);
    connect(this, &DownloadJob::downloadResult, context, functor);
    start();
}
template<typename Functor>
void KonqInterfaces::DownloadJob::startDownload(QWidget* widget, QObject* context, Functor functor)
{
    prepareDownloadJob(widget, {});
    connect(this, &DownloadJob::downloadResult, context, functor);
    start();
}

#endif // DOWNLOADJOB_H
