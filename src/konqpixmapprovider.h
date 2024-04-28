/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>
                            2024 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQ_PIXMAPPROVIDER_H
#define KONQ_PIXMAPPROVIDER_H

#include "konqprivate_export.h"

#include <QMap>
#include <QPixmap>
#include <QUrl>
#include <QObject>

class KConfigGroup;
class KConfig;

namespace KIO {
    class FavIconRequestJob;
}

/**
 * @brief Class providing favicon management
 *
 * This class uses KIO::FavIconRequestJob to download favicons, then stores the
 * corresponding URLs in a cache for quick retrieval.
 *
 * Each host can have a favicon, but pages inside that host can have different favicon.
 *
 * If downloading many favicons at the same time (for example when creating a Bookmarks menu
 * with many entries in a single folder with an empty favicon cache), there's the risk that
 * many FavIconRequestJob are created at the same time, which makes Konqueror hang. To avoid
 * this issue, at most #s_maxJobs are created at the same time: the others are queued and start
 * automatically as soon as the number of running jobs goes below that threshold.
 */
class KONQUERORPRIVATE_EXPORT KonqPixmapProvider : public QObject
{
    Q_OBJECT
public:
    static KonqPixmapProvider *self();

    ~KonqPixmapProvider() override;

    /**
     * @brief Trigger a download of a default favicon
     */
    void downloadHostIcon(const QUrl &hostUrl);

    /**
     * @brief Trigger a download of the default favicon for a list of URLs
     *
     * This works as downloadHostIcon() except that it doesn't emit the changed()
     * signal after each download but only after all of them have finished.
     *
     * If you need to download the favicon for the host of many URLs, it's better to
     * call this rather than downloadHostIcon() as it will avoid many consecutive calls
     * to slot connected with the changed() signal.
     * @param urls the list of URLs to download the host favicon for
     * @warning In theory, the finishedDownloadingHostIcons() signal should be emitted after
     * all URLs in @p urls are downloaded, regardless of whether there are pending downloads
     * for other URLs. For implementation simplicity, however, the signal is actually emitted
     * when the download queue is empty, even if some dowlonads aren't for URLs in @p urls.
     */
    void downloadHostIcons(const QList<QUrl> &urls);

    /**
     * Trigger a download of a custom favicon (from the HTML page)
     */
    void setIconForUrl(const QUrl &hostUrl, const QUrl &iconUrl);

    /**
     * Looks up a pixmap for @p url. Uses a cache for the iconname of url.
     */
    QPixmap pixmapFor(const QString &url, int size);

    /**
     * Loads the cache to @p kc from key @p key.
     */
    void load(KConfigGroup &kc, const QString &key);
    /**
     * Saves the cache to @p kc as key @p key.
     * Only those @p items are saved, otherwise the cache would grow forever.
     */
    void save(KConfigGroup &kc, const QString &key, const QStringList &items);

    /**
     * Clears the pixmap cache
     */
    void clear();

    /**
     * Looks up an iconname for @p url. Uses a cache for the iconname of url.
     */
    QString iconNameFor(const QUrl &url);
    QIcon iconForUrl(const QUrl &url);
    QIcon iconForUrl(const QString &url_str);

Q_SIGNALS:
    /**
     * @brief Signal emitted when the icon for the URL changes
     */
    void changed();

private Q_SLOTS:

    /**
     * @brief Starts downloading the next favicon in queue
     *
     * If the number of running jobs is already the maximum allowed, nothing is done
     */
    void downloadNextFavIcon();

    /**
     * @brief Kills all jobs and empties the download queue
     *
     * This is called when the last window in the application is closed, as having existing jobs would
     * prevent Konqueror from exiting.
     */
    void cleanupDownloadsQueue();

private:
    QPixmap loadIcon(const QString &icon, int size);

    /**
     * @brief Type of functions to pass as callback to startFavIconJob()
     */
    using FavIconRequestCallback = std::function<void(KIO::FavIconRequestJob*)>;

    /**
     * @brief Starts the job to request a favicon
     *
     * If the number of running jobs is equal to the allowed maximum, the new request
     * is queued and will be started as soon as one of the existing jobs ends
     *
     * @param hostUrl the URL to request the favicon for
     * @param callback the function to call in response to the job's `result()` signal. It will be passed the job as argument
     * @param iconUrl an icon to associate with @p hostUrl. If it's a valid URL, KIO::FavIconRequestJob::setIconUrl() will be
     *  called on the job
     */
    void startFavIconJob(const QUrl &hostUrl, FavIconRequestCallback callback, const QUrl &iconUrl = {});

    /**
     * @brief Struct encapsulating the data about a favicon request
     */
    struct FavIconRequestData {
        QUrl hostUrl; //!< The URL to retrieve the favicon for
        FavIconRequestCallback callback; //!< The callback to use in response to the job's `result()` signal
        QUrl iconUrl; //!< The URL of the icon to pass to KIO::FavIconRequestJob::setIconUrl(), if any
    };
    void startFavIconJob(const FavIconRequestData &data);

    /**
     * @brief Enum describing how updateIcons() should behave
     */
    enum class UpdateMode{
        Host, //!< update the generic favicon for a host
        Page //!< update the favicon for a specific page
    };
    /**
     * @brief Updates the cached icon URLs
     *
     * @param job the job used to retrieve the favicon
     * @param mode whether the update is for the generic favicon for a host or the favicon for a specific page
     * @return `true` if any of the icon URLs have changed and `false` otherwise
     */
    bool updateIcons(KIO::FavIconRequestJob* job, UpdateMode mode = UpdateMode::Host);

    KonqPixmapProvider();
    friend class KonqPixmapProviderSingleton;

    QMap<QUrl, QString> iconMap;

    //TODO currently the number 10 is chosen arbitrarily. There should be a better way to choose it
    int static constexpr s_maxJobs = 10; //!< The maximum number of jobs to run at the same time
    QList<FavIconRequestData> m_pendingRequests; //!< Data describing the favicon requests for which a job hasn't been started yet
    /**
     * @brief A list of the currently running jobs
     * @note Jobs are automatically removed when they finish
     */
    QList<QObject*> m_jobs;
};

#endif // KONQ_PIXMAPPROVIDER_H
