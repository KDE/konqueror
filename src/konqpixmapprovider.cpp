/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>

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
}

KonqPixmapProvider::~KonqPixmapProvider()
{
}

void KonqPixmapProvider::downloadHostIcon(const QUrl &hostUrl)
{
    //Only attempt to download icon for http(s) URLs
    if (!hostUrl.scheme().startsWith(QLatin1String("http"))) {
        return;
    }
    KIO::FavIconRequestJob *job = new KIO::FavIconRequestJob(hostUrl);
    connect(job, &KIO::FavIconRequestJob::result, this, [job, this](KJob *) {
        bool modified = false;
        const QUrl _hostUrl = job->hostUrl();
        QMap<QUrl, QString>::iterator itEnd = iconMap.end();
        for (QMap<QUrl, QString>::iterator it = iconMap.begin(); it != itEnd; ++it) {
            const QUrl url(it.key());
            if (url.host() == _hostUrl.host()) {
                // For host default-icons still query the favicon manager to get
                // the correct icon for pages that have an own one.
                const QString icon = KIO::favIconForUrl(url);
                if (!icon.isEmpty() && *it != icon) {
                    *it = icon;
                    modified = true;
                }
            }
        }
        if (modified) {
            emit changed();
        }
    });
}

void KonqPixmapProvider::setIconForUrl(const QUrl &hostUrl, const QUrl &iconUrl)
{
    KIO::FavIconRequestJob *job = new KIO::FavIconRequestJob(hostUrl);
    job->setIconUrl(iconUrl);
    connect(job, &KIO::FavIconRequestJob::result, this, [job, this](KJob *) {
        bool modified = false;
        const QUrl _hostUrl = job->hostUrl();
        QMap<QUrl, QString>::iterator itEnd = iconMap.end();
        for (QMap<QUrl, QString>::iterator it = iconMap.begin(); it != itEnd; ++it) {
            const QUrl url(it.key());
            if (url.host() == _hostUrl.host() && url.path() == _hostUrl.path()) {
                const QString icon = job->iconFile();
                if (!icon.isEmpty() && *it != icon) {
                    *it = icon;
                    modified = true;
                }
            }
        }
        if (modified) {
            emit changed();
        }
    });
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
        // Use the folder icon for the empty URL
        QMimeDatabase db;
        const QMimeType directoryType = db.mimeTypeForName(QStringLiteral("inode/directory"));
        icon = directoryType.iconName();
        icon = "konqueror";
        Q_ASSERT(!icon.isEmpty());
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

