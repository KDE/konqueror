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

#ifndef __treeitem_h
#define __treeitem_h

#include <QtCore/QList>
#include <kbookmark.h>

class TreeItem
{
public:
    TreeItem(const KBookmark& bk, TreeItem * parent);
    ~TreeItem();
    TreeItem * child(int row);
    TreeItem * parent() const;

    void insertChildren(int first, int last);
    void deleteChildren(int first, int last);
    void moveChildren(int first, int last, TreeItem * newParent, int position);
    KBookmark bookmark() const;
    int childCount();
    TreeItem * treeItemForBookmark(const KBookmark& bk);
    bool deleted;
private:
    void initChildren();
    bool init;
    QList<TreeItem *> children;
    TreeItem * mparent;
    KBookmark mbk;
};
#endif
