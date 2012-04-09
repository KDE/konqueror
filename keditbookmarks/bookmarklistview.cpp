/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>
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

#include "bookmarklistview.h"
#include "globalbookmarkmanager.h"
#include "kbookmarkmodel/model.h"
#include "toplevel.h" // for KEBApp
#include "settings.h"
#include "kbookmarkmodel/commands.h"

#include <QHeaderView>
#include <QItemSelection>
#include <QMenu>
#include <QKeyEvent>
#include <QBrush>
#include <QPalette>

#include <kdebug.h>

BookmarkFolderView::BookmarkFolderView( BookmarkListView * view, QWidget * parent )
    : KBookmarkView(parent), mview(view)
{
    mmodel = new BookmarkFolderViewFilterModel(parent);
    mmodel->setSourceModel(view->model());
    setModel(mmodel);
    header()->setVisible(false);
    setRootIsDecorated(false);
    setDropIndicatorShown(true);
    setCurrentIndex( mmodel->index(0,0, QModelIndex()));

    connect(mmodel, SIGNAL(modelReset()), this, SLOT(slotReset()));
}


BookmarkFolderView::~BookmarkFolderView()
{

}

void BookmarkFolderView::selectionChanged ( const QItemSelection & deselected, const QItemSelection & selected)
{
    const QModelIndexList & list = selectionModel()->selectedIndexes();
    if(list.count())
        mview->setRootIndex( mmodel->mapToSource(list.at(0)) );
    else
        mview->setRootIndex( QModelIndex());
    KBookmarkView::selectionChanged(deselected, selected);
}

void BookmarkFolderView::slotReset()
{
    setCurrentIndex( mmodel->index(0,0, QModelIndex()));
    loadFoldedState();
}

KBookmark BookmarkFolderView::bookmarkForIndex(const QModelIndex & idx) const
{
    qDebug()<<"BookmarkFolderView::bookmarkForIndex"<<idx;
    const QModelIndex & index = mmodel->mapToSource(idx);
    return static_cast<KBookmarkModel *>(mmodel->sourceModel())->bookmarkForIndex(index);
}

/********/


BookmarkListView::BookmarkListView( QWidget * parent )
    : KBookmarkView(parent)
{
    setDragEnabled(true);
}


void BookmarkListView::setModel(QAbstractItemModel * model)
{
    KBookmarkView::setModel(model);
}

KBookmarkModel* BookmarkListView::bookmarkModel() const
{
    return dynamic_cast<KBookmarkModel*>(QTreeView::model());
}

KBookmark BookmarkListView::bookmarkForIndex(const QModelIndex & idx) const
{
    return bookmarkModel()->bookmarkForIndex(idx);
}


BookmarkListView::~BookmarkListView()
{
    saveColumnSetting();
}

void BookmarkListView::contextMenuEvent ( QContextMenuEvent * e )
{
    QModelIndex index = indexAt(e->pos());
    KBookmark bk;
    if(index.isValid())
        bk = bookmarkForIndex(index);

    QMenu* popup;
    if( !index.isValid()
       || (bk.address() == GlobalBookmarkManager::self()->root().address())
       || (bk.isGroup())) //FIXME add empty folder padder
    {
        popup = KEBApp::self()->popupMenuFactory("popup_folder");
    }
    else
    {
        popup = KEBApp::self()->popupMenuFactory("popup_bookmark");
    }
    if (popup)
        popup->popup(e->globalPos());
}

void BookmarkListView::loadColumnSetting()
{
    header()->resizeSection(KEBApp::NameColumn, KEBSettings::name());
    header()->resizeSection(KEBApp::UrlColumn, KEBSettings::uRL());
    header()->resizeSection(KEBApp::CommentColumn, KEBSettings::comment());
    header()->resizeSection(KEBApp::StatusColumn, KEBSettings::status());
}

void BookmarkListView::saveColumnSetting()
{
    KEBSettings::setName( header()->sectionSize(KEBApp::NameColumn));
    KEBSettings::setURL( header()->sectionSize(KEBApp::UrlColumn));
    KEBSettings::setComment( header()->sectionSize(KEBApp::CommentColumn));
    KEBSettings::setStatus( header()->sectionSize(KEBApp::StatusColumn));
    KEBSettings::self()->writeConfig();
}

/************/

BookmarkFolderViewFilterModel::BookmarkFolderViewFilterModel(QObject * parent)
    : QSortFilterProxyModel(parent)
{
}

QStringList BookmarkFolderViewFilterModel::mimeTypes() const
{
    return sourceModel()->mimeTypes();
}

bool BookmarkFolderViewFilterModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    QModelIndex dropDestProxyIndex;
    bool isInsertBetweenOp = false;
    if(row == -1)
    {
        dropDestProxyIndex = parent;
    }
    else
    {
        dropDestProxyIndex = index(row, column, parent);
        isInsertBetweenOp = true;
    }
    QModelIndex dropDestIndex = mapToSource(dropDestProxyIndex);
    if (!isInsertBetweenOp)
    {
        // Just dropping it onto dropDestIndex - ignore row and column.
        return sourceModel()->dropMimeData( data, action, -1, -1, dropDestIndex);
    }
    else
    {
        // Dropping before dropDestIndex.  We want to keep row and column
        // relative to the parent.
        // I'm reasonably certain the parent must be valid in this case.  If you get a crash here - nag me!
        Q_ASSERT(parent.isValid());
        QModelIndex dropDestParentIndex = mapToSource(parent);
        return sourceModel()->dropMimeData( data, action, dropDestIndex.row(), dropDestIndex.column(), dropDestParentIndex);
    }
}

BookmarkFolderViewFilterModel::~BookmarkFolderViewFilterModel()
{
}

bool BookmarkFolderViewFilterModel::filterAcceptsColumn ( int source_column, const QModelIndex & source_parent ) const
{
    Q_UNUSED(source_parent);

    //Show name, hide everything else
    return (source_column == 0);
}

bool BookmarkFolderViewFilterModel::filterAcceptsRow ( int source_row, const QModelIndex & source_parent ) const
{
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    const KBookmark bk = index.data(KBookmarkModel::KBookmarkRole).value<KBookmark>();
    return bk.isGroup();
}

#include "bookmarklistview.moc"
