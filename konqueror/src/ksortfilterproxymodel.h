/*
 * Copyright (C) 2009 John Tapsell <tapsell@kde.org>
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
      KSortFilterProxyModel(QObject * parent = 0);
      /*! Destroys this sorting filter model. */
      ~KSortFilterProxyModel();

      /*! Whether to show the children of a matching parent.
       *  This is false by default. */
      bool showAllChildren() const;
      /*! Set whether to show the children of a matching parent.
       *  This is false by default. */
      void setShowAllChildren(bool showAllChildren);

    protected:
      /*! \reimp */
      virtual bool filterAcceptsRow ( int source_row, const QModelIndex & source_parent ) const;
      KSortFilterProxyModelPrivate * const d_ptr;

      Q_DISABLE_COPY( KSortFilterProxyModel )
};
#endif
