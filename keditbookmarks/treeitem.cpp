/* This file is part of the KDE project
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

#include "treeitem_p.h"
#include <kdebug.h>
#include <QtCore/QVector>

TreeItem::TreeItem(const KBookmark& bk, TreeItem * parent)
    : mparent(parent), mbk(bk)
{
    init = false;
}

TreeItem::~TreeItem()
{
    qDeleteAll(children);
    children.clear();
}

TreeItem * TreeItem::child(int row)
{
    if(!init)
        initChildren();
    if(row < 0 || row > children.count())
      return parent();
    return children.at(row);
}

int TreeItem::childCount()
{
    if(!init)
        initChildren();
    return children.count();
}

TreeItem * TreeItem::parent() const
{
    return mparent;
}

void TreeItem::insertChildren(int first, int last)
{
    // Find child number last
    KBookmarkGroup parent = bookmark().toGroup();
    KBookmark child = parent.first();
    for(int j=0; j < last; ++j)
        child = parent.next(child);

    //insert children
    int i = last;
    do
    {
        children.insert(i, new TreeItem(child, this));
        child = parent.previous(child);
        --i;
    } while(i >= first);

}

void TreeItem::deleteChildren(int first, int last)
{
    QList<TreeItem *>::iterator firstIt, lastIt, it;
    firstIt = children.begin() + first;
    lastIt = children.begin() + last + 1;
    for( it = firstIt; it != lastIt; ++it)
    {
        delete *it;
    }
    children.erase(firstIt, lastIt);
}

KBookmark TreeItem::bookmark() const
{
    return mbk;
}

void TreeItem::initChildren()
{
    init = true;
    if(mbk.isGroup())
    {
        KBookmarkGroup parent = mbk.toGroup();
        for(KBookmark child = parent.first(); child.hasParent(); child = parent.next(child) )
        {
            TreeItem * item = new TreeItem(child, this);
            children.append(item);
        }
    }
}

TreeItem * TreeItem::treeItemForBookmark(const KBookmark& bk)
{
    if(bk.address() == mbk.address())
        return this;
    QString commonParent = KBookmark::commonParent(bk.address(), mbk.address());
    if(commonParent == mbk.address()) //mbk is a parent of bk
    {
        QList<TreeItem *>::const_iterator it, end;
        end = children.constEnd();
        for( it = children.constBegin(); it != end; ++it)
        {
            KBookmark child = (*it)->bookmark();
            if( KBookmark::commonParent(child.address(), bk.address()) == child.address())
                    return (*it)->treeItemForBookmark(bk);
        }
        return 0;
    }
    else
    {
        if(parent() == 0)
            return 0;
        return parent()->treeItemForBookmark(bk);
    }
}

