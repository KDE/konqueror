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

#include <kurl.h>
#include <kpixmapprovider.h>
#include "favicon_interface.h"

#include <QtCore/QMap>
#include <QtGui/QPixmap>

class KConfigGroup;
class KConfig;

// Ideally OrgKdeFavIconInterface should be exported with KONQUERORPRIVATE_EXPORT, but the cmake macro
// doesn't allow that. Doesn't seem to be a problem though, at least on linux, since the methods are all inline.

class KONQUERORPRIVATE_EXPORT KonqPixmapProvider : public org::kde::FavIcon, virtual public KPixmapProvider
{
    Q_OBJECT
public:
    static KonqPixmapProvider * self();

    virtual ~KonqPixmapProvider();

    /**
     * Looks up a pixmap for @p url. Uses a cache for the iconname of url.
     */
    virtual QPixmap pixmapFor( const QString& url, int size = 0 );

    /**
     * Loads the cache to @p kc from key @p key.
     */
    void load( KConfigGroup& kc, const QString& key );
    /**
     * Saves the cache to @p kc as key @p key.
     * Only those @p items are saved, otherwise the cache would grow forever.
     */
    void save( KConfigGroup& kc, const QString& key, const QStringList& items );

    /**
     * Clears the pixmap cache
     */
    void clear();

    /**
     * Looks up an iconname for @p url. Uses a cache for the iconname of url.
     */
    QString iconNameFor( const KUrl& url );

Q_SIGNALS:
    void changed();

private Q_SLOTS:
    /**
     * Connected to the iconChanged signal emitted by the kded module
     */
    void notifyChange( bool isHost, const QString& hostOrURL, const QString& iconName );

private:
    QPixmap loadIcon( const QString& icon, int size );

    KonqPixmapProvider();
    friend class KonqPixmapProviderSingleton;

    QMap<KUrl,QString> iconMap;
};


#endif // KONQ_PIXMAPPROVIDER_H
