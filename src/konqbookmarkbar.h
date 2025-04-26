//  -*- c-basic-offset:4; indent-tabs-mode:nil -*-
/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1999 Kurt Granroth <granroth@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef KONQBOOKMARKBAR_H
#define KONQBOOKMARKBAR_H

#include <QObject>
#include <QPointer>
#include <QList>
#include <kbookmark.h>
#include <kactioncollection.h>

class KToolBar;
class KBookmarkMenu;
class KBookmarkOwner;
class KBookmarkBarPrivate;
class KBookmarkManager;

/**
 * @brief This class provides a representation of a bookmark toolbar.
 *
 * Using this class is nearly identical to using KBookmarkMenu so follow the directions
 * there.
 */
//FIXME rename KonqBookmarkBar
class KBookmarkBar : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Fills a bookmark toolbar
     *
     * @param manager the bookmark manager
     * @param owner implementation of the KBookmarkOwner interface (callbacks)
     * @param toolBar toolbar to fill
     * @param parent the parent object
     */
    KBookmarkBar(KBookmarkManager *manager,
                 KBookmarkOwner *owner, KToolBar *toolBar,
                 QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~KBookmarkBar() override;

    /**
     * @brief The address of the parent bookmark group
     * @return The address of the parent bookmark group. If the bar is filtered,
     * always returns an empty string
     */
    QString parentAddress();

public Q_SLOTS:
    /**
     * @brief Deletes all elements in the bookmark bar
     *
     * It also clears the associated toolbar, if any
     */
    void clear();

    /**
     * @brief Displays a context menu for the toolbar
     *
     * If at @p pt there's an action, it displays the context menu for that action,
     * otherwise it displays the default context menu
     * @param pt the point where to display the context menu
     */
    void contextMenu(const QPoint &pt);

    /**
     * @brief Updates all the entries in the bookmark bar
     *
     * It also causes child menus to be updated
     */
    void slotBookmarksChanged(const QString &);

    /**
     * @brief Reads the configuration settings and updates the bar accordingly
     */
    void slotConfigChanged();

protected:

    /**
     * @brief Fills the toolbar with the contents of a bookmarks group
     * @param parent the bookmark group
     */
    void fillBookmarkBar(const KBookmarkGroup &parent);

    /**
     * @brief Override of `QObject::eventFilter` which handles drag and drop
     */
    bool eventFilter(QObject *o, QEvent *e) override;

private:
    /**
     * @brief The root of the toolbar menu
     *
     * @return the same as `KBookmarkManager::toolbar()` of the associated `KBookmarkManager`
     * or the root bookmark if the `KBookmarkBar` is filtered
     */
    KBookmarkGroup getToolbar();

    /**
     * @brief Removes the temporary separator
     */
    void removeTempSep();

    /**
    * Handle a QDragMoveEvent event on a toolbar drop
    * @return `true` if the event should be accepted, `false` if the event should be ignored
    * @param pos the current QDragMoveEvent position
    * @param actions the list of actions plugged into the bar
    *        returned action was dropped on
    * @param text the text to use for the toolbar separator
    */
    bool handleToolbarDragMoveEvent(const QPoint &pos, const QList<QAction *> &actions, const QString &text);

    KBookmarkOwner *m_pOwner; //!<The `KBookmarkOwner` used by this object
    QPointer<KToolBar> m_toolBar; //!<The toolbar represented by this object
    KBookmarkManager *m_pManager; //!<The `KBookmarkManager` used by this object
    QList<KBookmarkMenu *> m_lstSubMenus; //!<A list of submenus
    QAction *m_toolBarSeparator; //!<An action to mark the position of the toolbar while dragging it

    KBookmarkBarPrivate *const d; //!<The D object
};

#endif // KONQBOOKMARKBAR_H
