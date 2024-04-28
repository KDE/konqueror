/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>
                            2024 Stefano Crocco <stefano.crocco@alice.it>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqpixmapprovider.h"

#include <QMimeDatabase>
#include <QMimeType>
#include <QIcon>
#include "konqdebug.h"

#include <KIO/FavIconRequestJob>
#include <kio/global.h>
#include <kprotocolinfo.h>
#include <kconfiggroup.h>
#include <KIconLoader>

#include <QApplication>

class KonqPixmapProviderSingleton
{
public:
    KonqPixmapProvider self;
};
Q_GLOBAL_STATIC(KonqPixmapProviderSingleton, globalPixmapProvider)

KonqPixmapProvider *KonqPixmapProvider::self()
{
    return &globalPixmapProvider->self;
}

KonqPixmapProvider::KonqPixmapProvider()
    : QObject()
{
    connect(qApp, &QApplication::lastWindowClosed, this, &KonqPixmapProvider::cleanupDownloadsQueue);
}

KonqPixmapProvider::~KonqPixmapProvider()
{
}

//Only attempt to download icon for http(s) URLs
static bool canDownloadFavIconForScheme(const QUrl &url) {
    return url.scheme().startsWith(QLatin1String("http"));
}

void KonqPixmapProvider::downloadHostIcons(const QList<QUrl>& urls)
{
    //We use a dynamic property rather than an instance variable since it's only used here
    setProperty("modified", false);
    auto proc = [this](KIO::FavIconRequestJob *job){
        bool res = updateIcons(job);
        if (res) {
            setProperty("modified", true);
        }
        if (m_pendingRequests.isEmpty()) {
            if (property("modified").toBool()) {
                emit changed();
            }
            setProperty("modified", {});
        }
    };
    QList<QUrl> filteredUrls;
    std::copy_if(urls.constBegin(), urls.constEnd(), std::back_inserter(filteredUrls), &canDownloadFavIconForScheme);
    for (const QUrl &url : filteredUrls) {
        startFavIconJob(url, proc);
    };
}

void KonqPixmapProvider::startFavIconJob(const FavIconRequestData& data)
{
    startFavIconJob(data.hostUrl, data.callback, data.iconUrl);
}

void KonqPixmapProvider::startFavIconJob(const QUrl& hostUrl, FavIconRequestCallback proc, const QUrl &iconUrl)
{
    if (m_jobs.count() >= s_maxJobs) {
        m_pendingRequests.append({hostUrl, proc, {}});
        return;
    }
    KIO::FavIconRequestJob *job = new KIO::FavIconRequestJob(hostUrl);
    m_jobs.append(job);
    connect(job, &QObject::destroyed, this, [this](QObject *o){m_jobs.removeOne(o);});
    if (iconUrl.isValid()) {
        job->setIconUrl(iconUrl);
    }
    connect(job, &KIO::FavIconRequestJob::result, this, [job, proc, this](KJob *){
        proc(job);
        m_jobs.removeOne(job);
        downloadNextFavIcon();
    });
}

bool KonqPixmapProvider::updateIcons(KIO::FavIconRequestJob* job, UpdateMode mode)
{
    bool modified = false;
    const QUrl _hostUrl = job->hostUrl();
    QMap<QUrl, QString>::iterator itEnd = iconMap.end();
    for (QMap<QUrl, QString>::iterator it = iconMap.begin(); it != itEnd; ++it) {
        const QUrl url(it.key());
        QString icon;
        switch (mode) {
            case UpdateMode::Host:
                if (url.host() == _hostUrl.host()) {
                    // For host default-icons still query the favicon manager to get
                    // the correct icon for pages that have an own one.
                    icon = KIO::favIconForUrl(url);
                    modified = true;
                }
                break;
            case UpdateMode::Page:
                if (url.host() == _hostUrl.host() && url.path() == _hostUrl.path()) {
                    icon = job->iconFile();
                    modified = true;
                }
                break;
        }
        if (modified && !icon.isEmpty() && *it != icon) {
            *it = icon;
        }
    }
    return modified;
}

void KonqPixmapProvider::downloadNextFavIcon()
{
    if (m_pendingRequests.isEmpty()) {
        return;
    }
    if (m_jobs.count() >= s_maxJobs) {
        return;
    }
    auto data = m_pendingRequests.takeFirst();
    startFavIconJob(data);
}

void KonqPixmapProvider::downloadHostIcon(const QUrl &hostUrl)
{
    if (!canDownloadFavIconForScheme(hostUrl)) {
        return;
    }

    auto proc = [this](KIO::FavIconRequestJob *job) {
        if (updateIcons(job)) {
            emit changed();
        }
    };
    startFavIconJob(hostUrl, proc);
}

void KonqPixmapProvider::cleanupDownloadsQueue()
{
    m_pendingRequests.clear();
    QList<KIO::FavIconRequestJob*> jobs = findChildren<KIO::FavIconRequestJob*>();
    for (auto o : m_jobs) {
        auto j = qobject_cast<KIO::FavIconRequestJob*>(o);
        if (o) {
            j->kill(KJob::Quietly);
            //Calling this seems to be necessary to allow the application to close
            //successfully
            j->deleteLater();
        }
    }
}

void KonqPixmapProvider::setIconForUrl(const QUrl &hostUrl, const QUrl &iconUrl)
{
    auto callback = [this] (KIO::FavIconRequestJob *job) {
        if (updateIcons(job, UpdateMode::Page)) {
            emit changed();
        }
    };
    startFavIconJob(hostUrl, callback, iconUrl);
}

// at first, tries to find the iconname in the cache
// if not available, tries to find the pixmap for the mimetype of url
// if that fails, gets the icon for the protocol
// finally, inserts the url/icon pair into the cache
QString KonqPixmapProvider::iconNameFor(const QUrl &url)
{
    QMap<QUrl, QString>::iterator it = iconMap.find(url);
    QString icon;
    if (it != iconMap.end()) {
        icon = it.value();
        if (!icon.isEmpty()) {
            return icon;
        }
    }

    if (url.url().isEmpty()) {
        // Use the Konqueror icon for the empty URL
        icon = "konqueror";
    } else {
        icon = KIO::iconNameForUrl(url);
        Q_ASSERT(!icon.isEmpty());
    }

    // cache the icon found for url
    iconMap.insert(url, icon);

    return icon;
}

QPixmap KonqPixmapProvider::pixmapFor(const QString &url, int size)
{
    return loadIcon(iconNameFor(QUrl::fromUserInput(url)), size);
}

void KonqPixmapProvider::load(KConfigGroup &kc, const QString &key)
{
    iconMap.clear();
    const QStringList list = kc.readPathEntry(key, QStringList());
    QStringList::const_iterator it = list.begin();
    QStringList::const_iterator itEnd = list.end();
    while (it != itEnd) {
        const QString url(*it);
        if ((++it) == itEnd) {
            break;
        }
        const QString icon(*it);
        iconMap.insert(QUrl::fromUserInput(url), icon);
        ++it;
    }
}

// only saves the cache for the given list of items to prevent the cache
// from growing forever.
void KonqPixmapProvider::save(KConfigGroup &kc, const QString &key,
                              const QStringList &items)
{
    QStringList list;
    QStringList::const_iterator itEnd = items.end();
    for (QStringList::const_iterator it = items.begin(); it != itEnd; ++it) {
        QMap<QUrl, QString>::const_iterator mit = iconMap.constFind(QUrl::fromUserInput(*it));
        if (mit != iconMap.constEnd()) {
            list.append(mit.key().url());
            list.append(mit.value());
        }
    }
    kc.writePathEntry(key, list);
}

void KonqPixmapProvider::clear()
{
    iconMap.clear();
}

QPixmap KonqPixmapProvider::loadIcon(const QString &icon, int size)
{
    if (size == 0) {
        size = KIconLoader::SizeSmall;
    }
    return QIcon::fromTheme(icon).pixmap(size);
}

QIcon KonqPixmapProvider::iconForUrl(const QUrl &url)
{
    return QIcon::fromTheme(iconNameFor(url));
}

QIcon KonqPixmapProvider::iconForUrl(const QString &url_str)
{
    return iconForUrl(QUrl::fromUserInput(url_str));
}

