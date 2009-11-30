/*
 * Copyright (C) 2007-2008 Omat Holding B.V. <info@omat.nl>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "ksortfilterproxymodel.h"

/**
 * Private class that helps to provide binary compatibility between releases.
 * @internal
 */
//@cond PRIVATE
class KSortFilterProxyModelPrivate {
    public:
        KSortFilterProxyModelPrivate() { 
            showAllChildren = false;
        }
        ~KSortFilterProxyModelPrivate() {}
       
        bool showAllChildren;
};

KSortFilterProxyModel::KSortFilterProxyModel(QObject * parent)
   : QSortFilterProxyModel(parent), d_ptr( new KSortFilterProxyModelPrivate )
{
}

KSortFilterProxyModel::~KSortFilterProxyModel()
{
}

bool KSortFilterProxyModel::filterAcceptsRow ( int source_row, const QModelIndex & source_parent ) const
{
    if( filterRegExp().isEmpty() ) return true; //Shortcut for common case

    if( QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent) )
        return true;

    //one of our children might be accepted, so accept this row if one of our children are accepted.
    QModelIndex source_index = sourceModel()->index(source_row, 0, source_parent);
    for(int i = 0 ; i < sourceModel()->rowCount(source_index); i++) {
        if(filterAcceptsRow(i, source_index)) return true;
    }

    //one of our parents might be accepted, so accept this row if one of our parents is accepted.
    if(d_ptr->showAllChildren) {
        QModelIndex parent_index = source_parent;
        while(parent_index.isValid()) {
            int row = parent_index.row();
            parent_index = parent_index.parent();
            if(QSortFilterProxyModel::filterAcceptsRow(row, parent_index)) return true;
        }
    }

    return false;
}

bool KSortFilterProxyModel::showAllChildren() const
{
    return d_ptr->showAllChildren;
}
void KSortFilterProxyModel::setShowAllChildren(bool showAllChildren)
{
    if(showAllChildren == d_ptr->showAllChildren)
        return;
    d_ptr->showAllChildren = showAllChildren;
    invalidateFilter();
}

