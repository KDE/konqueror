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

#ifndef __bookmarkmodel_h
#define __bookmarkmodel_h

#include <QtCore/QAbstractItemModel>

class KBookmark;

class KBookmarkModelRemoveSentry;
class KBookmarkModelMoveSentry;
class KBookmarkModelInsertSentry;

class KBookmarkModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    // Those keditbookmarks classes need to access beginInsertRows etc.
    friend class KBookmarkModelInsertSentry;
    friend class KBookmarkModelRemoveSentry;

    KBookmarkModel(const KBookmark& root);
    void setRoot(const KBookmark& root);

    virtual ~KBookmarkModel();

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

    QModelIndex indexForBookmark(const KBookmark& bk) const;
    KBookmark bookmarkForIndex(const QModelIndex& index) const;
    void emitDataChanged(const KBookmark& bk);

    //drag and drop
    virtual bool dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent );
    virtual QStringList mimeTypes() const;
    virtual QMimeData * mimeData( const QModelIndexList & indexes ) const;
    virtual Qt::DropActions supportedDropActions () const;

private:
    class Private;
    Private * const d;
};

#endif
