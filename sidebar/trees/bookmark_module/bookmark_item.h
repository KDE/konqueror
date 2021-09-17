/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef bookmark_item_h
#define bookmark_item_h

#include <konq_sidebartreeitem.h>
#include <kbookmark.h>

/**
 * A bookmark item
 */
class KonqSidebarBookmarkItem : public KonqSidebarTreeItem
{
public:
    KonqSidebarBookmarkItem(KonqSidebarTreeItem *parentItem, KonqSidebarTreeTopLevelItem *topLevelItem,
                            const KBookmark &bk, int key);

    virtual ~KonqSidebarBookmarkItem() {}

    virtual bool populateMimeData(QMimeData *mimeData, bool move);

    virtual void middleButtonClicked();
    virtual void rightButtonPressed();

    virtual void del();

    // The URL to open when this link is clicked
    virtual QUrl externalURL() const;

    // overwrite this if you want a tooltip shown on your item
    virtual QString toolTipText() const;

    // Called when this item is selected
    virtual void itemSelected();

    virtual QString key(int column, bool /*ascending*/) const;

    virtual KBookmark &bookmark();

private:
    KBookmark m_bk;
    int m_key;
};

#endif
