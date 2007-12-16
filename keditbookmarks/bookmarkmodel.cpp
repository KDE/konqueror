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
#include "toplevel.h"
#include "commands.h"

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
    Private(const KBookmark& root)
        : mRoot(root)
    {
        mRootItem = new TreeItem(root, 0);
    }
    ~Private()
    {
        delete mRootItem;
        
        //TESTING
        mRootItem = 0;
    }
    TreeItem * mRootItem;
    KBookmark mRoot;
};

KBookmarkModel::KBookmarkModel(const KBookmark& root)
    : QAbstractItemModel(), d(new Private(root))
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
            case 2: {
                QDomNode subnode = bk.internalElement().namedItem("desc");
                return (subnode.firstChild().isNull())
                    ? QString()
                    : subnode.firstChild().toText().data();
            }
            case 3: { //Status column
                QString text1 = ""; //FIXME favicon state
                QString text2 = ""; //FIXME link state
                if(text1.isNull() || text2.isNull())
                    return QVariant( text1 + text2);
                else
                    return QVariant( text1 + "  --  " + text2);
            }
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
// no empty folder padders atm


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
        CmdHistory::self()->addCommand(new EditCommand(bookmarkForIndex(index).address(), index.column(), value.toString()));
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
        return 1; // Only one child: "Bookmarks"
    }
}

int KBookmarkModel::columnCount(const QModelIndex &) const
{
    return 4; // Name, URL, Comment and Status
}

QModelIndex KBookmarkModel::indexForBookmark(const KBookmark& bk) const
{
    return createIndex(KBookmark::positionInParent(bk.address()) , 0, d->mRootItem->treeItemForBookmark(bk));
}

void KBookmarkModel::emitDataChanged(const KBookmark& bk)
{
    QModelIndex idx = indexForBookmark(bk);
    emit dataChanged(idx, idx.sibling(idx.row(), columnCount(QModelIndex())-1) );
}

QMimeData * KBookmarkModel::mimeData( const QModelIndexList & indexes ) const
{
    QMimeData *mimeData = new QMimeData;
    KBookmark::List bookmarks;
    QByteArray addresses;

    QModelIndexList::const_iterator it, end;
    end = indexes.constEnd();
    for( it = indexes.constBegin(); it!= end; ++it)
        if( it->column() == 0)
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
    Q_UNUSED(action)

    //FIXME this only works for internal drag and drops

    QModelIndex idx;
    if(row == -1)
        idx = parent;
    else
        idx = index(row, column, parent);

    KBookmark bk = bookmarkForIndex(idx);
    QString addr = bk.address();
    if(bk.isGroup())
        addr += "/0";
        
    if(action == Qt::CopyAction)
    {
        KEBMacroCommand * cmd = CmdGen::insertMimeSource("Copy", data, addr);
        CmdHistory::self()->didCommand(cmd);
    }
    else if(action == Qt::MoveAction)
    {
        KBookmark::List bookmarks;
        if(data->hasFormat("application/x-keditbookmarks"))
        {
            QList<QByteArray> addresses = data->data("application/x-keditbookmarks").split(';');
            QList<QByteArray>::const_iterator it, end;
            end = addresses.constEnd();
            for(it = addresses.constBegin(); it != end; ++it)
            {
                KBookmark bk = CurrentMgr::self()->mgr()->findByAddress(QString::fromLatin1(*it));
                kDebug()<<"Extracted bookmark xxx to list: "<<bk.address();
                bookmarks.push_back(bk);
            }

            KEBMacroCommand * cmd = CmdGen::itemsMoved(bookmarks, addr, false);
            CmdHistory::self()->didCommand(cmd);            
        }
        else
        {
            kDebug()<<"NO FORMAT";
            bookmarks = KBookmark::List::fromMimeData(data);
            KEBMacroCommand * cmd = CmdGen::insertMimeSource("Copy", data, addr);
            CmdHistory::self()->didCommand(cmd);
        }
    }

    //FIXME drag and drop implementation

    return true;
}

KBookmark KBookmarkModel::bookmarkForIndex(const QModelIndex& index) const
{
    return static_cast<TreeItem *>(index.internalPointer())->bookmark();
}

#include "bookmarkmodel.moc"
