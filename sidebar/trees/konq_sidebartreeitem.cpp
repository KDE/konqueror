/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2003 Waldo Bastian <bastian@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "konq_sidebartree.h"
//#include "konq_sidebartreepart.h"

KonqSidebarTreeItem::KonqSidebarTreeItem(KonqSidebarTreeItem *parentItem, KonqSidebarTreeTopLevelItem *topLevelItem)
    : Q3ListViewItem(parentItem)
{
    initItem(topLevelItem);
}

KonqSidebarTreeItem::KonqSidebarTreeItem(KonqSidebarTree *parent, KonqSidebarTreeTopLevelItem *topLevelItem)
    : Q3ListViewItem(parent)
{
    initItem(topLevelItem);
}

KonqSidebarTreeItem::~KonqSidebarTreeItem()
{
    KonqSidebarTree *t = tree();
    if (t) {
        t->itemDestructed(this);
    }
}

void KonqSidebarTreeItem::initItem(KonqSidebarTreeTopLevelItem *topLevelItem)
{
    m_topLevelItem = topLevelItem;
    m_bListable = true;
    m_bClickable = true;

    setExpandable(true);
}

void KonqSidebarTreeItem::middleButtonClicked()
{
    emit tree()->createNewWindow(externalURL());
}

KonqSidebarTreeModule *KonqSidebarTreeItem::module() const
{
    return m_topLevelItem->module();
}

KonqSidebarTree *KonqSidebarTreeItem::tree() const
{
    return static_cast<KonqSidebarTree *>(listView());
}
