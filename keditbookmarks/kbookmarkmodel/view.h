/* This file is part of the KDE project
   Copyright (C) 2005 Daniel Teske <teske@squorn.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef BOOKMARKMODEL_BOOKMARKVIEW_H
#define BOOKMARKMODEL_BOOKMARKVIEW_H

#include <QTreeView>

#include "kbookmarkmodel_export.h"

class KBookmark;

class KBOOKMARKMODEL_EXPORT KBookmarkView : public QTreeView
{
    Q_OBJECT
public:
    explicit KBookmarkView(QWidget *parent = 0);
    virtual ~KBookmarkView();
    virtual KBookmark bookmarkForIndex(const QModelIndex & idx) const = 0;
    void loadFoldedState();

private Q_SLOTS:
    void slotExpanded(const QModelIndex& index);
    void slotCollapsed(const QModelIndex& index);

private:
    void loadFoldedState(const QModelIndex& parentIndex);
    bool m_loadingState;
};

#endif
