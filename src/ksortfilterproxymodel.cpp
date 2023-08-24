/*
    SPDX-FileCopyrightText: 2007-2008 Omat Holding B.V. <info@omat.nl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ksortfilterproxymodel.h"

/**
 * Private class that helps to provide binary compatibility between releases.
 * @internal
 */
//@cond PRIVATE
class KSortFilterProxyModelPrivate
{
public:
    KSortFilterProxyModelPrivate()
    {
        showAllChildren = false;
    }
    ~KSortFilterProxyModelPrivate() {}

    bool showAllChildren;
};

KSortFilterProxyModel::KSortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent), d_ptr(new KSortFilterProxyModelPrivate)
{
}

KSortFilterProxyModel::~KSortFilterProxyModel()
{
    delete d_ptr;
}

bool KSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (filterRegularExpression().pattern().isEmpty()) {
        return true;    //Shortcut for common case
    }

    if (QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent)) {
        return true;
    }

    //one of our children might be accepted, so accept this row if one of our children are accepted.
    QModelIndex source_index = sourceModel()->index(source_row, 0, source_parent);
    for (int i = 0; i < sourceModel()->rowCount(source_index); i++) {
        if (filterAcceptsRow(i, source_index)) {
            return true;
        }
    }

    //one of our parents might be accepted, so accept this row if one of our parents is accepted.
    if (d_ptr->showAllChildren) {
        QModelIndex parent_index = source_parent;
        while (parent_index.isValid()) {
            int row = parent_index.row();
            parent_index = parent_index.parent();
            if (QSortFilterProxyModel::filterAcceptsRow(row, parent_index)) {
                return true;
            }
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
    if (showAllChildren == d_ptr->showAllChildren) {
        return;
    }
    d_ptr->showAllChildren = showAllChildren;
    invalidateFilter();
}

