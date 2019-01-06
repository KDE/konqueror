/* This file is part of the KDE project
   Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KONQ_PIXMAPPROVIDER_H
#define KONQ_PIXMAPPROVIDER_H

#include "konqprivate_export.h"

#include <kpixmapprovider.h>

#include <QMap>
#include <QPixmap>
#include <QUrl>

class KConfigGroup;
class KConfig;

class KONQUERORPRIVATE_EXPORT KonqPixmapProvider : public QObject, public KPixmapProvider
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
    QPixmap pixmapFor(const QString &url, int size) override;

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

Q_SIGNALS:
    void changed();

private:
    QPixmap loadIcon(const QString &icon, int size);

    KonqPixmapProvider();
    friend class KonqPixmapProviderSingleton;

    QMap<QUrl, QString> iconMap;
};

#endif // KONQ_PIXMAPPROVIDER_H
