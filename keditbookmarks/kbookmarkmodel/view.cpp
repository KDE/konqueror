/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>
   Copyright (C) 2005 Daniel Teske <teske@squorn.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) version 3.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "view.h"

#include <kbookmark.h>

KBookmarkView::KBookmarkView(QWidget *parent)
    : QTreeView(parent), m_loadingState(false)
{
    setAcceptDrops(true);
    setDefaultDropAction(Qt::MoveAction);
    connect(this, SIGNAL(expanded(QModelIndex)), this, SLOT(slotExpanded(QModelIndex)));
    connect(this, SIGNAL(collapsed(QModelIndex)), this, SLOT(slotCollapsed(QModelIndex)));
}

KBookmarkView::~KBookmarkView()
{
}

void KBookmarkView::loadFoldedState()
{
    m_loadingState = true;
    loadFoldedState(QModelIndex());
    m_loadingState = false;
}

void KBookmarkView::loadFoldedState(const QModelIndex& parentIndex)
{
    const int count = model()->rowCount(parentIndex);
    for (int row = 0; row < count; ++row) {
        const QModelIndex index = model()->index(row, 0, parentIndex);
        const KBookmark bk = bookmarkForIndex(index);
        if (bk.isNull()) {
            expand(index);
        }
        else if (bk.isGroup()) {
            setExpanded(index, bk.toGroup().isOpen());
            loadFoldedState(index);
        }
    }
}

void KBookmarkView::slotExpanded(const QModelIndex& index)
{
    if (!m_loadingState) {
        KBookmark bk = bookmarkForIndex(index);
        bk.internalElement().setAttribute("folded", "no");
    }
}

void KBookmarkView::slotCollapsed(const QModelIndex& index)
{
    if (!m_loadingState) {
        KBookmark bk = bookmarkForIndex(index);
        bk.internalElement().setAttribute("folded", "yes");
    }
}

#include "view.moc"
