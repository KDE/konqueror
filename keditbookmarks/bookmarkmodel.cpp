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

#include "bookmarkmodel.h"
#include "treeitem_p.h"

#include <KIcon>
#include <kdebug.h>
#include <klocale.h>
#include <QtGui/QIcon>
#include <QtCore/QVector>
#include <QtGui/QPixmap>
#include <QtCore/QStringList>
#include <QtCore/QMimeData>

class KBookmarkModel::Private
{
public:
    Private(const KBookmark& root)
        : mRoot(root)
    {
        mRootItem = new TreeItem(root, 0);
    }
    ~Private()
    {
        delete mRootItem;
    }
    TreeItem * mRootItem;
    KBookmark mRoot;

    // for move support
    QModelIndex mOldParent;
    int mFirst;
    int mLast;
    QModelIndex mNewParent;
    int mPosition;
    QVector<QModelIndex> mMovedIndexes;
    QVector<QModelIndex> mOldParentIndexes;
    QVector<QModelIndex> mNewParentIndexes;
};

KBookmarkModel::KBookmarkModel(const KBookmark& root)
    : QAbstractItemModel(), d(new Private(root))
{
}

KBookmarkModel::~KBookmarkModel()
{
    delete d;
}

void KBookmarkModel::resetModel()
{
    delete d->mRootItem;
    d->mRootItem = 0;
    reset();
    d->mRootItem = new TreeItem(d->mRoot, 0);
}

void KBookmarkModel::beginMoveRows(const QModelIndex & oldParent, int first, int last, const QModelIndex & newParent, int position)
{
    emit aboutToMoveRows(oldParent, first, last, newParent, position);
    if(oldParent != newParent) // different parents
    {
        int columnsCount = columnCount(QModelIndex());
        for(int i=first; i<=last; ++i)
            for(int j=0; j<columnsCount; ++j)
                d->mMovedIndexes.push_back( index(i, j, oldParent));

        int rowsCount = rowCount(oldParent);
        for(int i=last+1; i<rowsCount; ++i)
            for(int j=0; j<columnsCount; ++j)
                d->mOldParentIndexes.push_back( index(i, j, oldParent));

        rowsCount = rowCount(newParent);
        for(int i=position; i<rowsCount; ++i)
            for(int j=0; j<columnsCount; ++j)
                d->mNewParentIndexes.push_back( index(i, j, newParent));
    }
    else //same parent
    {
        int columnsCount = columnCount(QModelIndex());
        if(first > position)
        {
            // swap around
            int tempPos = position;
            position = last + 1;
            last = first - 1;
            first = tempPos;
        }
        // Invariant first > positionx
        for(int i=first; i<=last; ++i)
            for(int j=0; j<columnsCount; ++j)
                d->mMovedIndexes.push_back( index(i, j, oldParent));

        for(int i=last+1; i<position; ++i)
            for(int j=0; j<columnsCount; ++j)
                d->mOldParentIndexes.push_back( index(i, j, oldParent));
    }
    d->mOldParent = oldParent;
    d->mFirst = first;
    d->mLast = last;
    d->mNewParent = newParent;
    d->mPosition = position;
}
void KBookmarkModel::endMoveRows()
{
    if(d->mOldParent != d->mNewParent)
    {
        int count = d->mMovedIndexes.count();
        int delta = d->mPosition - d->mFirst;
        for(int i=0; i <count; ++i)
        {
            QModelIndex idx = createIndex(d->mMovedIndexes[i].row()+delta,
                                          d->mMovedIndexes[i].column(),
                                          d->mMovedIndexes[i].internalPointer());
            changePersistentIndex( d->mMovedIndexes[i], idx);
        }

        count = d->mOldParentIndexes.count();
        delta = d->mLast - d->mFirst + 1;
        for(int i=0; i<count; ++i)
        {
            QModelIndex idx = createIndex( d->mOldParentIndexes[i].row()-delta,
                                           d->mOldParentIndexes[i].column(),
                                           d->mOldParentIndexes[i].internalPointer());
            changePersistentIndex( d->mOldParentIndexes[i], idx);
        }

        count = d->mNewParentIndexes.count();
        for(int i=0; i<count; ++i)
        {
            int delta = (d->mLast-d->mFirst+1);
            QModelIndex idx = createIndex( d->mNewParentIndexes[i].row() + delta,
                                           d->mNewParentIndexes[i].column(),
                                           d->mNewParentIndexes[i].internalPointer());
            changePersistentIndex( d->mNewParentIndexes[i], idx);
        }
    }
    else
    {
        int count = d->mMovedIndexes.count();
        int delta = d->mPosition - (d->mLast +1);
        for(int i=0; i<count; ++i)
        {
            QModelIndex idx = createIndex(d->mMovedIndexes[i].row()+delta, d->mMovedIndexes[i].column(), d->mMovedIndexes[i].internalPointer());
            changePersistentIndex(d->mMovedIndexes[i], idx);
        }

        count = d->mOldParentIndexes.count();
        delta = d->mLast-d->mFirst + 1;
        for(int i=0; i<count; ++i)
        {
            QModelIndex idx = createIndex(d->mOldParentIndexes[i].row() - delta, d->mOldParentIndexes[i].column(), d->mOldParentIndexes[i].internalPointer());
            changePersistentIndex(d->mOldParentIndexes[i], idx);
        }
    }
    emit rowsMoved(d->mOldParent, d->mFirst, d->mLast, d->mNewParent, d->mPosition);
}

QVariant KBookmarkModel::data(const QModelIndex &index, int role) const
{
    //Text
    if(index.isValid() && (role == Qt::DisplayRole || role == Qt::EditRole))
    {
        KBookmark bk = bookmarkForIndex(index);
        if(bk.address().isEmpty())
            if(index.column() == 0)
                return QVariant( i18n("Bookmarks") );
            else
                return QVariant();

        switch( index.column() )
        {
            case 0:
                return QVariant( bk.fullText() );
            case 1:
                return QVariant( bk.url().pathOrUrl() );
            case 2:{
                QDomNode subnode = bk.internalElement().namedItem("desc");
                return (subnode.firstChild().isNull())
                    ? QString()
                    : subnode.firstChild().toText().data();
            }
            case 3:
                return QVariant( QString() ); //FIXME status column
            default:
                return QVariant( QString() ); //can't happen
        }
    }

    //Icon
    if(index.isValid() && role == Qt::DecorationRole && index.column() == 0)
    {
        KBookmark bk = bookmarkForIndex(index);
        if(bk.address().isEmpty())
            return KIcon("bookmark");
        return KIcon(bk.icon());
    }
    return QVariant();
}

//FIXME QModelIndex KBookmarkModel::buddy(const QModelIndex & index) //return parent for empty folder padders


Qt::ItemFlags KBookmarkModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    KBookmark bk = bookmarkForIndex(index);
    if( !bk.address().isEmpty() ) // non root
    {
        if( bk.isGroup())
        {
            if(index.column() == 0 || index.column() == 2)
                return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        }
        else
            if(index.column() < 3)
                return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    }

    // root
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool KBookmarkModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(index.isValid() && role == Qt::EditRole)
    {
        //FIXME don't create a command if still the same
        // and ignore if name column is empty
        emit textEdited(bookmarkForIndex(index),
                        index.column(), value.toString());
        return true;
    }
    return false;
}

QVariant KBookmarkModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch(section)
        {
            case 0:
                return i18n("Bookmark");
            case 1:
                return i18n("URL");
            case 2:
                return i18n("Comment");
            case 3:
                return i18n("Status");
            default:
                return QString(); // Can't happpen
        }
    }
    else
        return QVariant();
}

QModelIndex KBookmarkModel::index(int row, int column, const QModelIndex &parent) const
{
    if( ! parent.isValid())
        return createIndex(row, column, d->mRootItem);

    TreeItem * item = static_cast<TreeItem *>(parent.internalPointer());
    return createIndex(row, column, item->child(row));
}



QModelIndex KBookmarkModel::parent(const QModelIndex &index) const
{
    if(!index.isValid())
    {
        // qt asks for the parent of an invalid parent
        // either we are in a inconsistent case or more likely
        // we are dragging and dropping and qt didn't check
        // what itemAt() returned
        return index;

    }
    KBookmark bk = bookmarkForIndex(index);;
    const QString rootAddress = d->mRoot.address();

    if(bk.address() == rootAddress)
        return QModelIndex();

    KBookmarkGroup parent  = bk.parentGroup();
    TreeItem * item = static_cast<TreeItem *>(index.internalPointer());
    if(parent.address() != rootAddress)
        // TODO replace first argument with parent.positionInParent()
        return createIndex( KBookmark::positionInParent(parent.address()) , 0, item->parent());
    else //parent is root
        return createIndex( 0, 0, item->parent());
}

int KBookmarkModel::rowCount(const QModelIndex &parent) const
{
    if(parent.isValid())
            return static_cast<TreeItem *>(parent.internalPointer())->childCount();
    else //root case
    {
        return 1;
    }
}

int KBookmarkModel::columnCount(const QModelIndex &) const
{
    return 4;
}

QModelIndex KBookmarkModel::bookmarkToIndex(const KBookmark& bk) const
{
    // TODO replace first argument with bk.positionInParent()
    return createIndex( KBookmark::positionInParent(bk.address()), 0, d->mRootItem->treeItemForBookmark(bk));
}

void KBookmarkModel::emitDataChanged(const KBookmark& bk)
{
    QModelIndex index = bookmarkToIndex(bk);
    emit dataChanged(index, index );
}

QMimeData * KBookmarkModel::mimeData( const QModelIndexList & indexes ) const
{
    QMimeData *mimeData = new QMimeData;
    KBookmark::List bookmarks;
    QModelIndexList::const_iterator it, end;
    end = indexes.constEnd();
    for( it = indexes.constBegin(); it!= end; ++it)
        bookmarks.push_back( bookmarkForIndex(*it) );
    bookmarks.populateMimeData(mimeData);
    return mimeData;
}

Qt::DropActions KBookmarkModel::supportedDropActions () const
{
    //FIXME check if that actually works
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList KBookmarkModel::mimeTypes () const
{
    return KBookmark::List::mimeDataTypes();
}

bool KBookmarkModel::dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent)
{
    Q_UNUSED(action)

    //FIXME this only works for internal drag and drops

    QModelIndex idx;
    // idx is the index on which the data was dropped
    // work around stupid design of the qt api:
    if(row == -1)
        idx = parent;
    else
        idx = index(row, column, parent);
    KBookmark bk = bookmarkForIndex(idx);

    emit dropped(data, bk);

    return true;
}

KBookmark KBookmarkModel::bookmarkForIndex(const QModelIndex& index) const
{
    return static_cast<TreeItem *>(index.internalPointer())->bookmark();
}

#include "bookmarkmodel.moc"
