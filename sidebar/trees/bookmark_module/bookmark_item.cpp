/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "bookmark_item.h"
#include <konq_sidebartree.h>
#include <kiconloader.h>

#include "bookmark_module.h"

#define MYMODULE static_cast<KonqSidebarBookmarkModule*>(module())

KonqSidebarBookmarkItem::KonqSidebarBookmarkItem(KonqSidebarTreeItem *parentItem, KonqSidebarTreeTopLevelItem *topLevelItem, const KBookmark &bk, int key)
    : KonqSidebarTreeItem(parentItem, topLevelItem), m_bk(bk), m_key(key)
{
    setText(0, bk.text());
    setPixmap(0, SmallIcon(bk.icon()));
}

bool KonqSidebarBookmarkItem::populateMimeData(QMimeData *mimeData, bool move)
{
    m_bk.populateMimeData(mimeData);
    // TODO honor bool move ?
    Q_UNUSED(move);
    return true;
}

void KonqSidebarBookmarkItem::middleButtonClicked()
{
    emit tree()->createNewWindow(externalURL());
}

void KonqSidebarBookmarkItem::rightButtonPressed()
{
    MYMODULE->showPopupMenu();
}

void KonqSidebarBookmarkItem::del()
{
    //maybe todo
}

QUrl KonqSidebarBookmarkItem::externalURL() const
{
    return m_bk.isGroup() ? QUrl() : m_bk.url();
}

QString KonqSidebarBookmarkItem::toolTipText() const
{
    return m_bk.url().prettyUrl();
}

void KonqSidebarBookmarkItem::itemSelected()
{
    tree()->enableActions(false, false, false);
}

QString KonqSidebarBookmarkItem::key(int /*column*/, bool /*ascending*/) const
{
    return QString::number(m_key).rightJustified(5, '0');
}

KBookmark &KonqSidebarBookmarkItem::bookmark()
{
    return m_bk;
}
