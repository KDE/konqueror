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
#include <KBookmarkAction>

#include <QPointer>

#if KBOOKMARKS_VERSION < QT_VERSION_CHECK(6,16,0)
#define KBOOKMARKS_FIX_REQUIRED
#endif

namespace Konqueror { // to avoid clashing with KF5::Bookmarks which had a KonqBookmarkMenu class. Remove once using KF6.

class KonqBookmarkMenu : public KBookmarkMenu
{
    //friend class KBookmarkBar;
    Q_OBJECT
public:
    /**
     * @brief Fills a bookmark menu with Konqueror's bookmarks
     *
     * One instance of KonqBookmarkMenu is created for the toplevel menu,
     *  but also one per submenu.
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
#ifdef KBOOKMARKS_FIX_REQUIRED
        parentMenu->menu()->installEventFilter(this);
#endif
    }

    /**
     * @brief Destructor
     */
    ~KonqBookmarkMenu() override
    {}

    /**
     * @brief Creates a bookmark submenu
     *
     * @note This is only used internally and for bookmark toolbar.
     *
     * @param mgr the bookmark manager to use
     * @param owner the implementation of `KBookmarkOwner` to use
     * @param parentMenu the menu to be filled
     * @param parentAddress the bookmark address of the parent bookmark group
     */
    KonqBookmarkMenu(KBookmarkManager *mgr, KBookmarkOwner *owner, KBookmarkActionMenu *parentMenu, QString parentAddress)
        : KBookmarkMenu(mgr, owner, parentMenu->menu(), parentAddress)
    {
#ifdef KBOOKMARKS_FIX_REQUIRED
        parentMenu->menu()->installEventFilter(this);
#endif
    }

#ifdef KBOOKMARKS_FIX_REQUIRED
    /**
     * @brief Override of `QObject::eventFilter()`
     *
     * It sets the triggering mouse button for BookmarkAction when @p watched is
     * the parent menu, @p event is a `Qt::MouseButtonRelease` and there's a BookmarkAction
     * at the event position.
     *
     * @todo Remove this function when the issue described in BookmarkAction is fixed
     * @param watched the watched object
     * @param event the event
     * @return `false`
     */
    bool eventFilter(QObject *watched, QEvent *event) override;
#endif

protected Q_SLOTS:
    /**
     * @brief Loads favicons for the bookmarks in the menu
     */
    void fillFavicons();

protected:
    /**
     * @brief Fills the menu
     *
     * If this is the root item, the menu will have first the common bookmarks actions (add bookmark and so on), then
     * the list of bookmarks; if it represents a bookmark group, the common actions will be at the end.
     *
     * After creating the actions, it starts the asynchronous process of loading favicons by calling startFillingFavicons()
     */
    void refill() override;

    /**
     * @brief Starts the asynchronous process of loading favicons for the bookmarks
     *
     * Favicons are provided by KonqPixmapProvider. When the KonqPixmapProvider has finished loading the favicons,
     * fillFavicons() is automatically called.
     */
    void startFillingFavicons();

    /**
     * @brief Override of `KBookmarkMenu::actionForBookmark`
     *
     * It only differs from the base class version because it creates a `KonqBookmarkMenu` instead of `KBookmarkMenu`
     * for submenus.
     * @param bm the bookmark to create the menu for
     */
    QAction *actionForBookmark(const KBookmark &bm) override;

    /**
     * @brief Override of `KBookmarkMenu::contextMenu()`
     *
     * Unlike the base class version, the context menu is represented by a KonqBookmarkContextMenu.
     * @param action the action to display the context menu for
     */
    QMenu *contextMenu(QAction *action) override;
};

/**
 * @brief Class which represent the context menu for a bookmark
 */
class KonqBookmarkContextMenu : public KBookmarkContextMenu
{
    Q_OBJECT
public:

    /**
     * @brief Constructor
     *
     * @param bm the bookmark this context menu is for
     * @param mgr the bookmark manager
     * @param owner the bookmark owner
     */
    KonqBookmarkContextMenu(const KBookmark &bm, KBookmarkManager *mgr, KBookmarkOwner *owner);

    /**
     * @brief Destructor
     */
    ~KonqBookmarkContextMenu() override;

    /**
     * @brief Override of `KBookmarkContextMenu::addActions()`
     *
     * Relative to the base class version, it adds actions for toggling the bookmark visibility
     * in filtered toolbars and actions to open a bookmark in a new window or tab
     */
    void addActions() override;

public Q_SLOTS:
    /**
     * @brief Opens the boomark in a new tab
     */
    void openInNewTab();

    /**
     * @brief Opens the boomark in a new window
     */
    void openInNewWindow();

    /**
     * @brief Enables or disables showing the bookmark in the toolbar
     */
    void toggleShowInToolbar();

    /**
     * @brief Adds the bookmark to the speed dial
     */
    void addToSpeedDial();
};

#ifdef KBOOKMARKS_FIX_REQUIRED
/**
 * @brief A BookmarkAction which allows passing the correct mouse button to `KBookmarkOwner::openBookmark()`
 *
 * The problem is that `KBookmarkAction` uses `QGuiApplication::mouseButtons()` to determine which buttons
 * were pressed when the action was triggered (see `KBookmarkAction::slotTriggered()`). However, when clicking
 * on an action in a menu `QGuiApplication::mouseButtons()` always returns `Qt::NoButton`, which makes it
 * impossible to determine whether the action was triggered by a left or middle click.
 *
 * To work around this, this class provides a setTriggeringMouseButton() function which should be called by
 * a function with access to the mouse event which triggered the action to set the correct mouse button. This is
 * then used by slotCallSelected() to pass the correct button to `KBookmarkAction::slotSelected()`.
 *
 * @todo Remove this class and use `KBookmarkAction` when the issue described above is solved either in
 * `QGuiApplication::mouseButtons()` or in `KBookmarkAction`.
 */
class BookmarkAction : public KBookmarkAction
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     *
     * It takes the same arguments as `KBookmarkAction` constructor
     * @param bm the bookmark associated with the action
     * @param owner the bookmark owner to be used by the action
     * @param parent the parent object
     */
    BookmarkAction(const KBookmark &bm, KBookmarkOwner *owner, QObject *parent=nullptr);
    ~BookmarkAction() override; //!< Destructor

    /**
     * @brief Sets the button which triggered the action
     *
     * This is automatically reset to `Qt::NoButton` by callSlotSelected()
     * @param btn the mouse button which triggered the action
     */
    void setTriggeringMouseButton(Qt::MouseButton btn);

private Q_SLOTS:
    /**
     * @brief Slot connected to the `triggered()` signal
     *
     * It calls `KBookmarkAction::slotSelected()` passing the correct mouse button
     * (set using setTriggeringMouseButton()), then sets the triggering button to
     * the default value (`Qt::NoButton`)
     */
    void callSlotSelected();

private:
    Qt::MouseButton m_triggeringMouseButton = Qt::NoButton; //!< The mouse button which triggered the action

};
#else
//TODO: remove this once requiring KBookmarks >= 6.16.0
using BookmarkAction = KBookmarkAction;
#endif

}

#endif
