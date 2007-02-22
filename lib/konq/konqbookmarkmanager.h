/*  This file is part of the KDE project
    Copyright (C) 2006  David Faure <faure@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef KONQBOOKMARKMANAGER_H
#define KONQBOOKMARKMANAGER_H

#include <kbookmarkmanager.h>
#include <kstandarddirs.h>
#include <libkonq_export.h>

class LIBKONQ_EXPORT KonqBookmarkManager
{
public:
    static KBookmarkManager * self() {
        if ( !s_bookmarkManager )
        {
            QString bookmarksFile = KStandardDirs::locateLocal("data", QLatin1String("konqueror/bookmarks.xml"));
            s_bookmarkManager = KBookmarkManager::managerForFile( bookmarksFile, QLatin1String("konqueror") );
        }
        return s_bookmarkManager;
    }

private:
    static KBookmarkManager *s_bookmarkManager;
};

#endif
