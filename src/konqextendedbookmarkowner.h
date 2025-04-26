/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2003 Alexander Kellett <lypanov@kde.org>
    SPDX-FileCopyrightText: 1998, 1999 Simon Hausmann <hausmann@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQEXTENDEDBOOKMARKOWNER_H
#define KONQEXTENDEDBOOKMARKOWNER_H

#include <konqbookmarkmenu.h>
#include <kbookmarkowner.h>

/**
 * @brief Subclass of `KBookmarkOwner` which adds Konqueror-specific behavior
 */
class KonqExtendedBookmarkOwner : public KBookmarkOwner
{
public:

    /**
     * @brief Constructor
     *
     * @param w the main window the new object is associated with
     */
    KonqExtendedBookmarkOwner(KonqMainWindow *w);

    /**
     * @brief Override of `KBookmarkOwner::currentTitle()`
     *
     * @return The title of the main window
     */
    QString currentTitle() const override;

    /**
     * @brief Override of `KBookmarkOwner::currentUrl()`
     *
     * @return The URL of the current view or an empty URL if there's no current view
     */
    QUrl currentUrl() const override;

    /**
     * @brief Override of `KBookmarkOwner::supportsTabs()`
     *
     * @return `true`
     */
    bool supportsTabs() const override;

    /**
     * @brief Override of `KBookmarkOwner::currentBookmarkList()`
     *
     * @return A list containing the bookmark data for the active view in each tab
     */
    QList<FutureBookmark> currentBookmarkList() const override;

    /**
     * @brief Override of `KBookmarkOwner::openBookmark()`
     *
     * It opens the bookmark in the current tab, a new tab or a new window depending on
     * the mouse button, the keyboard modifiers and the user settings:
     * - if @p km includes `Ctrl`, it opens the bookmark in a new tab
     * - if the @p mb is `Qt::MiddleButton`, it opens it in a new tab or a new window
     * depending on the user settings for the middle mouse button
     * - otherwise it opens it in the current tab
     *
     * @param bm the bookmark to open
     * @param mb the mouse button clicked by the user
     * @param km the keyboard modifiers when the user clicked the mouse
     */
    void openBookmark(const KBookmark &bm, Qt::MouseButtons mb, Qt::KeyboardModifiers km) override;

    /**
     * @brief Override of `KBookmarkOwner::openInNewTab()`
     *
     * It opens the bookmark in a new tab
     * @param bm the bookmark to open
     */
    void openInNewTab(const KBookmark &bm) override;

    /**
     * @brief Override of `KBookmarkOwner::openInNewWindow()`
     *
     * It opens the bookmark in a new window
     * @param bm the bookmark to open
     */
    void openInNewWindow(const KBookmark &bm) override;

    /**
     * @brief Override of `KBookmarkOwner::openFolderinTabs()`
     *
     * It opens all the bookmarks in the given group in new tabs
     * @param grp the bookmark group to open
     */
    void openFolderinTabs(const KBookmarkGroup &grp) override;

private:
    KonqMainWindow *m_pKonqMainWindow; //!< The main window
};

#endif
