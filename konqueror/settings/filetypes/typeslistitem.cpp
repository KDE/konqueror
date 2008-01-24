/* This file is part of the KDE project
   Copyright (C) 2003 Waldo Bastian <bastian@kde.org>
   Copyright (C) 2003, 2007 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 or at your option version 3 as published by
   the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

// Own
#include "typeslistitem.h"

// KDE
#include <kdebug.h>
#include <kiconloader.h>


TypesListItem::TypesListItem(Q3ListView *parent, const QString & major)
  : Q3ListViewItem(parent),
    m_mimetypeData(major)
{
    setText(0, major);
}

TypesListItem::TypesListItem(TypesListItem *parent, KMimeType::Ptr mimetype, bool newItem)
  : Q3ListViewItem(parent),
    m_mimetypeData(mimetype, newItem)
{
    setText(0, m_mimetypeData.minorType());
}

TypesListItem::~TypesListItem()
{
}

void TypesListItem::paintCell(QPainter *painter, const QColorGroup & cg, int column, int width, int align)
{
    if (parent() && !pixmap(0)) {
        // Load icon here instead of loading it in the constructor. This way
        // the user won't wait for icons he won't see.
        setPixmap(0, KIconLoader::global()->loadMimeTypeIcon(m_mimetypeData.icon(), KIconLoader::Small));
    }
    Q3ListViewItem::paintCell(painter, cg, column, width, align);
}

#if 0
void TypesListItem::setIcon( const QString& icon )
{
    m_mimetypeData.setIcon(icon);
    setPixmap( 0, SmallIcon( icon ) );
}
#endif
