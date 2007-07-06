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

#ifndef __bookmarkmodel_h
#define __bookmarkmodel_h

#include <QtCore/QAbstractItemModel>
#include <QtCore/QVector>
#include <kbookmark.h>
#include "treeitem.h"

class QDropEvent;

class KBookmarkModelRemoveSentry;
class KBookmarkModelMoveSentry;
class KBookmarkModelInsertSentry;

class BookmarkModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    // Those keditbookmarks classes need to access beginInsertRows etc.
    friend class KBookmarkModelInsertSentry;
    friend class KBookmarkModelRemoveSentry;
    friend class KBookmarkModelMoveSentry;

    BookmarkModel(const KBookmark& root);

    virtual ~BookmarkModel();

    //reimplemented functions
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &index) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual void resetModel();

    QModelIndex bookmarkToIndex(const KBookmark& bk);
    void emitDataChanged(const KBookmark& bk);

    //drag and drop
    virtual bool dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent );
    virtual QStringList mimeTypes () const;
    virtual QMimeData * mimeData ( const QModelIndexList & indexes ) const;
    virtual Qt::DropActions supportedDropActions () const;

Q_SIGNALS:
    //FIXME searchline should respond too
    void aboutToMoveRows(const QModelIndex &, int, int, const QModelIndex &, int);
    void rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int);

    void dropped(const QMimeData* data, const KBookmark& bookmark);
    void textEdited(const KBookmark& bookmark, int column, const QString& text);

private:
    void beginMoveRows(const QModelIndex & oldParent, int first, int last, const QModelIndex & newParent, int position);
    void endMoveRows();

    TreeItem * rootItem;
    KBookmark mRoot;

    // for move support
    QModelIndex mOldParent;
    int mFirst;
    int mLast;
    QModelIndex mNewParent;
    int mPosition;
    QVector<QModelIndex> movedIndexes;
    QVector<QModelIndex> oldParentIndexes;
    QVector<QModelIndex> newParentIndexes;
};

#endif
