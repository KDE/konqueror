/* This file is part of the KDE project
   Copyright (C) 2003 Waldo Bastian <bastian@kde.org>
   Copyright (C) 2003, 2007 David Faure <faure@kde.org>
   Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>

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
#include <kicon.h>


TypesListItem::TypesListItem(QTreeWidget *parent, const QString & major)
  : QTreeWidgetItem(parent),
    m_mimetypeData(major)
{
    setText(0, major);
}

TypesListItem::TypesListItem(TypesListItem *parent, KMimeType::Ptr mimetype)
  : QTreeWidgetItem(parent),
    m_mimetypeData(mimetype)
{
    setText(0, m_mimetypeData.minorType());
}

TypesListItem::TypesListItem(TypesListItem *parent, const QString& newMimetype)
  : QTreeWidgetItem(parent),
    m_mimetypeData(newMimetype, true)
{
    setText(0, m_mimetypeData.minorType());
}

TypesListItem::~TypesListItem()
{
}

void TypesListItem::setIcon( const QString& icon )
{
    m_mimetypeData.setUserSpecifiedIcon(icon);
    loadIcon(true);
}

void TypesListItem::loadIcon(bool forceReload)
{
    if ((!m_mimetypeData.icon().isEmpty() && icon(0).isNull()) || forceReload) {
        QTreeWidgetItem::setIcon(0, KIcon(m_mimetypeData.icon()));
    }
}

