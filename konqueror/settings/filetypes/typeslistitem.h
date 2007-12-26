/* This file is part of the KDE project
   Copyright (C) 2003 Waldo Bastian <bastian@kde.org>
   Copyright (C) 2003 David Faure <faure@kde.org>

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

#ifndef _TYPESLISTITEM_H
#define _TYPESLISTITEM_H

#include "mimetypedata.h"
#include <Qt3Support/Q3ListView>

#include <kmimetype.h>

// TODO different subclasses for mimetypes and groups?
class TypesListItem : public Q3ListViewItem
{
public:
    /**
     * Create a filetype group
     */
    TypesListItem(Q3ListView *parent, const QString & major );

    /**
     * Create a filetype item inside a group
     */
    TypesListItem(TypesListItem *parent, KMimeType::Ptr mimetype, bool newItem=false);

    /**
     * Create a filetype item not inside a group (used by keditfiletype)
     */
    TypesListItem(Q3ListView *parent, KMimeType::Ptr mimetype, bool newItem=false);

    ~TypesListItem();

    //void setIcon( const QString& icon );

    QString name() const { return m_mimetypeData.name(); }
    const MimeTypeData& mimeTypeData() const { return m_mimetypeData; }
    MimeTypeData& mimeTypeData() { return m_mimetypeData; }

private:
    MimeTypeData m_mimetypeData;
};

#endif
