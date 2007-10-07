/* This file is part of the KDE project
   Copyright (C) 1998-2007 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KONQ_MENUACTIONS_H
#define KONQ_MENUACTIONS_H

#include <kfileitem.h>
#include <libkonq_export.h>

class QMenu;
class KonqMenuActionsPrivate;

/**
 * This class handles the user-defined actions for a url in a popupmenu.
 * User-defined actions include:
 * - builtin services like mount/unmount for old-style device desktop files
 * - user-defined actions for a .desktop file, defined in the file itself (see the desktop entry standard)
 * - servicemenus actions, defined in .desktop files and selected based on the mimetype of the url
 */
class LIBKONQ_EXPORT KonqMenuActions
{
public:
    /**
     * Creates a KonqMenuActions instance.
     * Note that this instance must stay alive for at least as long as the popupmenu;
     * it has the slots for the actions created by addActionsTo.
     */
    KonqMenuActions();

    /**
     * Sets the list of fileitems which the actions apply to.
     * This call is mandatory.
     */
    void setItems(const KFileItemList& items);

    /**
     * Sets the URL which the actions apply to. This call is optional,
     * the url of the first item given to setItems is used otherwise.
     */
    void setUrl(const KUrl& url);

    /**
     * Call this if actions that modify the files should not be shown.
     * This is controlled by Require=Write in the servicemenu desktop files
     */
    void setReadOnly(bool ro);

    /**
     * Generate the actions and submenus, and adds them to the @p menu.
     * All actions are created as children of the menu.
     * @return the number of actions added
     */
    int addActionsTo(QMenu* menu);

private:
    KonqMenuActionsPrivate* const d;
};

#endif /* KONQ_MENUACTIONS_H */
