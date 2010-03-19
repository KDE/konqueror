/* This file is part of the KDE project
   Copyright (C) 2005 Daniel Teske <teske@squorn.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 or at your option version 3 as published by
   the Free Software Foundation.

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
#include "commands.h"
#include "commandhistory.h"

#include <kbookmarkmanager.h>
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
    Private(const KBookmark& root, KBookmarkManager* manager)
        : mRoot(root), mManager(manager)
    {
        mRootItem = new TreeItem(root, 0);
    }
    ~Private()
    {
        delete mRootItem;
        mRootItem = 0;
    }
    TreeItem * mRootItem;
    KBookmark mRoot;
    KBookmarkManager* mManager;
};

KBookmarkModel::KBookmarkModel(const KBookmark& root, KBookmarkManager* manager, QObject* parent)
    : QAbstractItemModel(parent), d(new Private(root, manager))
{
}

void KBookmarkModel::setRoot(const KBookmark& root)
{
    d->mRoot = root;
    resetModel();
}

KBookmarkModel::~KBookmarkModel()
{
    delete d;
}

void KBookmarkModel::resetModel()
{
    delete d->mRootItem;
    d->mRootItem = new TreeItem(d->mRoot, 0);
    reset();
}

QVariant KBookmarkModel::data(const QModelIndex &index, int role) const
{
    //Text
    if(index.isValid() && (role == Qt::DisplayRole || role == Qt::EditRole))
    {
        const KBookmark bk = bookmarkForIndex(index);
        if(bk.address().isEmpty())
        {
            if(index.column() == NameColumnId)
                return QVariant( i18nc("name of the container of all browser bookmarks","Bookmarks") );
            else
                return QVariant();
        }

        switch( index.column() )
        {
            case NameColumnId:
                return QVariant( bk.fullText() );
            case UrlColumnId:
                return QVariant( bk.url().pathOrUrl() );
            case CommentColumnId:
                return QVariant( bk.description() );
            case StatusColumnId: {
                QString text1; //FIXME favicon state
                QString text2; //FIXME link state
                if(text1.isEmpty() || text2.isEmpty())
                    return QVariant( text1 + text2 );
                else
                    return QVariant( text1 + "  --  " + text2 );
            }
            default:
                return QVariant( QString() ); //can't happen
        }
    }

    //Icon
    if(index.isValid() && role == Qt::DecorationRole && index.column() == NameColumnId)
    {
        KBookmark bk = bookmarkForIndex(index);
        if(bk.address().isEmpty())
            return KIcon("bookmarks");
        return KIcon(bk.icon());
    }
    return QVariant();
}

//FIXME QModelIndex KBookmarkModel::buddy(const QModelIndex & index) //return parent for empty folder padders
// no empty folder padders atm


Qt::ItemFlags KBookmarkModel::flags(const QModelIndex &index) const
{
    const Qt::ItemFlags baseFlags = QAbstractItemModel::flags(index);

    if (!index.isValid())
        return (Qt::ItemIsDropEnabled | baseFlags);

    static const Qt::ItemFlags groupFlags =            Qt::ItemIsDropEnabled;
    static const Qt::ItemFlags groupDragEditFlags =    groupFlags | Qt::ItemIsDragEnabled | Qt::ItemIsEditable;
    static const Qt::ItemFlags groupEditFlags =        groupFlags | Qt::ItemIsEditable;
    static const Qt::ItemFlags rootFlags =             groupFlags;
    static const Qt::ItemFlags bookmarkFlags =         0;
    static const Qt::ItemFlags bookmarkDragEditFlags = bookmarkFlags | Qt::ItemIsDragEnabled | Qt::ItemIsEditable;
    static const Qt::ItemFlags bookmarkEditFlags =     bookmarkFlags | Qt::ItemIsEditable;

    Qt::ItemFlags result = baseFlags;

    const int column = index.column();
    const KBookmark bookmark = bookmarkForIndex(index);
    if (bookmark.isGroup())
    {
        const bool isRoot = bookmark.address().isEmpty();
        result |=
            (isRoot) ?                    rootFlags :
            (column == NameColumnId) ?    groupDragEditFlags :
            (column == CommentColumnId) ? groupEditFlags :
            /*else*/                      groupFlags;
    }
    else
    {
        result |=
            (column == NameColumnId) ?   bookmarkDragEditFlags :
            (column != StatusColumnId) ? bookmarkEditFlags
            /* else */                 : bookmarkFlags;
    }

    return result;
}

bool KBookmarkModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(index.isValid() && role == Qt::EditRole)
    {
        kDebug() << value.toString();
        CommandHistory::self()->addCommand(new EditCommand(bookmarkForIndex(index).address(), index.column(), value.toString()));
        return true;
    }
    return false;
}

QVariant KBookmarkModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        QString result;
        switch(section)
        {
            case NameColumnId:
                result = i18nc("@title:column name of a bookmark",
                               "Name");
                break;
            case UrlColumnId:
                result = i18nc("@title:column name of a bookmark",
                               "Location");
                break;
            case CommentColumnId:
                result = i18nc("@title:column comment for a bookmark",
                               "Comment");
                break;
            case StatusColumnId:
                result = i18nc("@title:column status of a bookmark",
                               "Status");
                break;
        }
        return result;
    }
    else
        return QVariant();
}

QModelIndex KBookmarkModel::index(int row, int column, const QModelIndex &parent) const
{
    if( ! parent.isValid())
        return createIndex(row, column, d->mRootItem);

    TreeItem * item = static_cast<TreeItem *>(parent.internalPointer());
        if(row == item->childCount()) {// Received drop below last row, simulate drop on last row
        return createIndex(row-1, column, item->child(row-1));
    }

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
        return 1; // Only one child: "Bookmarks"
    }
}

int KBookmarkModel::columnCount(const QModelIndex &) const
{
    return NoOfColumnIds;
}

QModelIndex KBookmarkModel::indexForBookmark(const KBookmark& bk) const
{
    return createIndex(KBookmark::positionInParent(bk.address()) , 0, d->mRootItem->treeItemForBookmark(bk));
}

void KBookmarkModel::emitDataChanged(const KBookmark& bk)
{
    QModelIndex idx = indexForBookmark(bk);
    kDebug() << idx;
    emit dataChanged(idx, idx.sibling(idx.row(), columnCount()-1) );
}

QMimeData * KBookmarkModel::mimeData( const QModelIndexList & indexes ) const
{
    QMimeData *mimeData = new QMimeData;
    KBookmark::List bookmarks;
    QByteArray addresses;

    QModelIndexList::const_iterator it, end;
    end = indexes.constEnd();
    for( it = indexes.constBegin(); it!= end; ++it)
        if( it->column() == NameColumnId)
        {
            bookmarks.push_back( bookmarkForIndex(*it) );
            if(!addresses.isEmpty())
                addresses.append(";");
            addresses.append( bookmarkForIndex(*it).address().toLatin1() );
            kDebug()<<"appended"<<bookmarkForIndex(*it).address().toLatin1();
        }

    bookmarks.populateMimeData(mimeData);
    mimeData->setData( "application/x-keditbookmarks", addresses);
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
    //FIXME this only works for internal drag and drops
    //FIXME Moving is *very* buggy
    QModelIndex dropDestIndex;
    bool isInsertBetweenOp = false;
    if(row == -1)
    {
        dropDestIndex = parent;
    }
    else
    {
        isInsertBetweenOp = true;
        dropDestIndex= index(row, column, parent);
    }

    KBookmark dropDestBookmark = bookmarkForIndex(dropDestIndex);
    if (dropDestBookmark.isNull())
    {
        // Presumably an invalid index: assume we want to place this in the root bookmark
        // folder.
        dropDestBookmark = d->mRoot;
    }

    QString addr = dropDestBookmark.address();
    if(dropDestBookmark.isGroup() && !isInsertBetweenOp)
    {
      addr += "/0";
    }
    // bookmarkForIndex(...) does not distinguish between the last item in the folder
    // and the point *after* the last item in the folder (and its hard to see how to
    // modify it so it does), so do the check here.
    if (isInsertBetweenOp && row == dropDestBookmark.positionInParent() + 1)
    {
        // Attempting to insert underneath the last item in a folder; adjust the address.
        addr = KBookmark::nextAddress(addr);
    }

    if(action == Qt::CopyAction)
    {
        KEBMacroCommand * cmd = CmdGen::insertMimeSource("Copy", data, addr);
        CommandHistory::self()->addCommand(cmd);
    }
    else if(action == Qt::MoveAction)
    {
        if(data->hasFormat("application/x-keditbookmarks"))
        {
            KBookmark::List bookmarks;
            QList<QByteArray> addresses = data->data("application/x-keditbookmarks").split(';');
            QList<QByteArray>::const_iterator it, end;
            end = addresses.constEnd();
            for(it = addresses.constBegin(); it != end; ++it)
            {
                KBookmark bk = d->mManager->findByAddress(QString::fromLatin1(*it));
                kDebug()<<"Extracted bookmark xxx to list: "<<bk.address();
                bookmarks.push_back(bk);
            }

            KEBMacroCommand * cmd = CmdGen::itemsMoved(bookmarks, addr, false);
            CommandHistory::self()->addCommand(cmd);
        }
        else
        {
            kDebug()<<"NO FORMAT";
            KEBMacroCommand * cmd = CmdGen::insertMimeSource("Copy", data, addr);
            CommandHistory::self()->addCommand(cmd);
        }
    }

    //FIXME drag and drop implementation

    return true;
}

KBookmark KBookmarkModel::bookmarkForIndex(const QModelIndex& index) const
{
    if (!index.isValid())
    {
      return KBookmark();
    }
    return static_cast<TreeItem *>(index.internalPointer())->bookmark();
}

#include "bookmarkmodel.moc"
