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
     * @param collec parent collection for the KActions.
     */
    KonqBookmarkMenu(KBookmarkManager *mgr, KBookmarkOwner *owner, KBookmarkActionMenu *parentMenu, KActionCollection *collec)
        : KBookmarkMenu(mgr, owner, parentMenu->menu())
    {
        m_actionCollection = collec;
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
//KBookmarkMenu doesn't create an action collection only in version 5.69.0
#if KBOOKMARKS_VERSION == QT_VERSION_CHECK(5, 69, 0)
        m_actionCollection = new KActionCollection(this);
#endif
    }

protected:
    /**
     * Structure used for storing information about
     * the dynamic menu setting
     */
    struct DynMenuInfo {
        bool show;
        QString location;
        QString type;
        QString name;
        class DynMenuInfoPrivate *d;
    };

    /**
     * @return dynmenu info block for the given dynmenu name
     */
    static DynMenuInfo showDynamicBookmarks(const QString &id);

    /**
     * Shows an extra menu for the given bookmarks file and type.
     * Upgrades from option inside XBEL to option in rc file
     * on first call of this function.
     * @param id the unique identification for the dynamic menu
     * @param info a DynMenuInfo struct containing the to be added/modified data
     */
    static void setDynamicBookmarks(const QString &id, const DynMenuInfo &info);

    /**
     * @return list of dynamic menu ids
     */
    static QStringList dynamicBookmarksList();

    void refill() override;
    QAction *actionForBookmark(const KBookmark &bm) override;
    QMenu *contextMenu(QAction *action) override;
    void fillDynamicBookmarks();
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
