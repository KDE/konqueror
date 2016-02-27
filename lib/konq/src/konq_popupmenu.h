/* This file is part of the KDE project
   Copyright (C) 1998-2016 David Faure <faure@kde.org>
   Copyright (C) 2001 Holger Freyther <freyther@yahoo.com>

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

#ifndef __konqpopupmenu_h
#define __konqpopupmenu_h

#include <QMenu>

#include <kparts/browserextension.h>

#include <libkonq_export.h>

class KFileItemList;
class KNewFileMenu;
class KFileItemActions;

class KActionCollection;
class KBookmarkManager;
class KonqPopupMenuPrivate;
class QUrl;

/**
 * This class implements the popup menu for URLs in file managers.
 *
 * The recommended usage is to use QMenu::popup() to avoid modal popupmenus.
 *
 * Either reuse the instance, or delete it after use with
 *   connect(menu, &QMenu::aboutToHide, [menu]() { menu->deleteLater(); });
 *
 * Users of KonqPopupMenu include: konqueror, kfind, the folderview desktop plasmoid.
 */
class LIBKONQ_EXPORT KonqPopupMenu : public QMenu
{
    Q_OBJECT
public:

    /**
     * Flags set by the calling application (e.g. konqueror), unlike
     * KParts::BrowserExtension::PopupFlags, which are set by the calling part
     */
    typedef uint Flags;
    enum { NoFlags = 0,
           ShowNewWindow = 1,
           NoPlugins = 2 /*for the unittest*/
         };
    // WARNING: bitfield. Next item is 4

    /**
     * Constructor
     * @param manager the bookmark manager for the "add to bookmark" action
     * Only used if KParts::BrowserExtension::ShowBookmark is set
     * @param items the list of file items the popupmenu should be shown for
     * @param viewURL the URL shown in the view, to test for RMB click on view background
     * @param actions list of actions the caller wants to see in the menu
     * @param newMenu "New" menu, shared with the File menu, in konqueror
     * @param parentWidget the widget we're showing this popup for. Helps destroying
     * the popup if the widget is destroyed before the popup.
     * @param appFlags flags from the KonqPopupMenu::Flags enum, set by the calling application
     * @param partFlags flags from the BrowserExtension enum, set by the calling part
     *
     * The actions to pass in include :
     * showmenubar, go_back, go_forward, go_up, cut, copy, paste, pasteto
     * The others items are automatically inserted.
     *
     * @todo that list is probably not be up-to-date
     */
    KonqPopupMenu(const KFileItemList &items,
                  const QUrl &viewURL,
                  KActionCollection &actions,
                  KNewFileMenu *newMenu,
                  Flags appFlags,
                  KParts::BrowserExtension::PopupFlags partFlags /*= KParts::BrowserExtension::DefaultPopupItems*/,
                  QWidget *parentWidget,
                  KBookmarkManager *manager = 0,
                  const KParts::BrowserExtension::ActionGroupMap &actionGroups = KParts::BrowserExtension::ActionGroupMap()
                 );

    /**
     * Don't forget to destroy the object
     */
    ~KonqPopupMenu();

    /**
     * Set the title of the URL, when the popupmenu is opened for a single URL.
     * This is used if the user chooses to add a bookmark for this URL.
     */
    void setURLTitle(const QString &urlTitle);

Q_SIGNALS:
    /**
     * Emitted before the "Open With" dialog is shown
     * This is used e.g in folderview to close the folder peek popups on invoking the "Open With" menu action
     */
    void openWithDialogAboutToBeShown();

private:
    KonqPopupMenuPrivate *d;
};

#endif
