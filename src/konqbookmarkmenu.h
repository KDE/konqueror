/* This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>
    SPDX-FileCopyrightText: 2006 Daniel Teske <teske@squorn.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef __konqbookmarkmenu_h__
#define __konqbookmarkmenu_h__

#include "kbookmarkmenu.h"
#include "kbookmarkactionmenu.h"
#include "kbookmarkcontextmenu.h"

#include <KActionCollection>
#include <kbookmarks_version.h>

#include <QPointer>

namespace Konqueror { // to avoid clashing with KF5::Bookmarks which had a KonqBookmarkMenu class. Remove once using KF6.

class KonqBookmarkMenu : public KBookmarkMenu
{
    //friend class KBookmarkBar;
    Q_OBJECT
public:
    /**
     * Fills a bookmark menu with konquerors bookmarks
     * (one instance of KonqBookmarkMenu is created for the toplevel menu,
     *  but also one per submenu).
     *
     * @param mgr The bookmark manager to use (i.e. for reading and writing)
     * @param owner implementation of the KBookmarkOwner callback interface.
     * Note: If you pass a null KBookmarkOwner to the constructor, the
     * URLs are openend by QDesktopServices::openUrl and "Add Bookmark" is disabled.
     * @param parentMenu menu to be filled
     */
    KonqBookmarkMenu(KBookmarkManager *mgr, KBookmarkOwner *owner, KBookmarkActionMenu *parentMenu)
        : KBookmarkMenu(mgr, owner, parentMenu->menu())
    {
        setBrowserMode(true);
    }
    ~KonqBookmarkMenu() override
    {}

    /**
     * Creates a bookmark submenu.
     * Only used internally and for bookmark toolbar.
     */
    KonqBookmarkMenu(KBookmarkManager *mgr, KBookmarkOwner *owner, KBookmarkActionMenu *parentMenu, QString parentAddress)
        : KBookmarkMenu(mgr, owner, parentMenu->menu(), parentAddress)
    {
    }

protected Q_SLOTS:
    void fillFavicons();

protected:
    void refill() override;
    void startFillingFavicons();
    QAction *actionForBookmark(const KBookmark &bm) override;
    QMenu *contextMenu(QAction *action) override;
};

class KonqBookmarkContextMenu : public KBookmarkContextMenu
{
    Q_OBJECT
public:
    KonqBookmarkContextMenu(const KBookmark &bm, KBookmarkManager *mgr, KBookmarkOwner *owner);
    ~KonqBookmarkContextMenu() override;
    void addActions() override;

public Q_SLOTS:
    void openInNewTab();
    void openInNewWindow();
    void toggleShowInToolbar();
};

}

#endif
