/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>

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

class KONQUERORPRIVATE_EXPORT KonqPixmapProvider : public QObject
{
    Q_OBJECT
public:
    static KonqPixmapProvider *self();

    ~KonqPixmapProvider() override;

    /**
     * Trigger a download of a default favicon
     */
    void downloadHostIcon(const QUrl &hostUrl);
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
    void changed();

private:
    QPixmap loadIcon(const QString &icon, int size);

    KonqPixmapProvider();
    friend class KonqPixmapProviderSingleton;

    QMap<QUrl, QString> iconMap;
};

#endif // KONQ_PIXMAPPROVIDER_H
