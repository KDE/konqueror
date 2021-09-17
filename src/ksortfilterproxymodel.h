/*
    SPDX-FileCopyrightText: 2009 John Tapsell <tapsell@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KSORTFILTERPROXYMODEL_H
#define KSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

/**
 * @class KSortFilterProxyModel
 *
 * This class extends QSortFilterProxyModel to allow filtering for a matching child
 * in a tree.
 * It can also show all the children of a matching parent, if setShowAllChildren is set.
 *
 * @author John Tapsell <tapsell@kde.org>
 * @since 4.4
 */
class KSortFilterProxyModelPrivate;

class KSortFilterProxyModel
    : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    /*! Constructs a sorting filter model with the given parent. */
    KSortFilterProxyModel(QObject *parent = nullptr);
    /*! Destroys this sorting filter model. */
    ~KSortFilterProxyModel() override;

    /*! Whether to show the children of a matching parent.
     *  This is false by default. */
    bool showAllChildren() const;
    /*! Set whether to show the children of a matching parent.
     *  This is false by default. */
    void setShowAllChildren(bool showAllChildren);

protected:
    /*! \reimp */
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    KSortFilterProxyModelPrivate *const d_ptr;

    Q_DISABLE_COPY(KSortFilterProxyModel)
};
#endif
