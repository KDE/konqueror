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
#include "bookmarkmodel.h"
#include "toplevel.h"
#include "settings.h"
#include "commands.h"
#include "treeitem_p.h"
#include <QtGui/QHeaderView>
#include <QtGui/QItemSelection>
#include <QtGui/QMenu>
#include <QtGui/QKeyEvent>
#include <QtGui/QBrush>
#include <QtGui/QPalette>

#include <kdebug.h>

BookmarkView::BookmarkView( QWidget * parent )
    :QTreeView( parent )
{
    setAcceptDrops(true);
}

BookmarkView::~BookmarkView()
{

}

/* ----------- */

BookmarkFolderView::BookmarkFolderView( BookmarkListView * view, QWidget * parent )
    :BookmarkView( parent ), mview(view)
{
    mmodel = new BookmarkFolderViewFilterModel(parent);
    mmodel->setSourceModel(view->model());
    setModel(mmodel);
    header()->setVisible(false);
    setRootIsDecorated(false);
    expandAll();
    setCurrentIndex( mmodel->index(0,0, QModelIndex()));
}

KBookmarkModel* BookmarkView::model() const
{
    return dynamic_cast<KBookmarkModel*>(QTreeView::model());
}

////

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
    BookmarkView::selectionChanged( deselected, selected);
}

/********/


BookmarkListView::BookmarkListView( QWidget * parent )
    :BookmarkView( parent )
{
    setDragEnabled(true);
}


void BookmarkListView::setModel(QAbstractItemModel * model)
{
    BookmarkView::setModel(model);
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
        bk = model()->bookmarkForIndex(index);

    QMenu* popup;
    if( !index.isValid()
       || (bk.address() == CurrentMgr::self()->root().address())
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

//FIXME clean up and remove unneeded things
SelcAbilities BookmarkListView::getSelectionAbilities() const
{
    SelcAbilities selctionAbilities;
    const QModelIndexList & sel = selectionModel()->selectedIndexes();
    selctionAbilities.itemSelected   = false;
    selctionAbilities.group          = false;
    selctionAbilities.separator      = false;
    selctionAbilities.urlIsEmpty     = false;
    selctionAbilities.root           = false;
    selctionAbilities.multiSelect    = false;
    selctionAbilities.singleSelect   = false;
    selctionAbilities.notEmpty       = false;

    if ( sel.count() > 0)
    {
        KBookmark nbk     = model()->bookmarkForIndex(sel.first());
        selctionAbilities.itemSelected   = true;
        selctionAbilities.group          = nbk.isGroup();
        selctionAbilities.separator      = nbk.isSeparator();
        selctionAbilities.urlIsEmpty     = nbk.url().isEmpty();
        selctionAbilities.root           = nbk.address() == CurrentMgr::self()->root().address();
        selctionAbilities.multiSelect    = (sel.count() > model()->columnCount());
        selctionAbilities.singleSelect   = (!selctionAbilities.multiSelect && selctionAbilities.itemSelected);
    }
    //FIXME check next line, if it actually works
    selctionAbilities.notEmpty = CurrentMgr::self()->root().first().hasParent(); //FIXME that's insane, checks whether there exists at least one bookmark

//     kDebug()<<"sa.itemSelected "<<selctionAbilities.itemSelected<<"\nsa.group "<<selctionAbilities.group<<
//                 "\nsa.separator "<<selctionAbilities.separator<<"\nsa.urlIsEmpty "<<selctionAbilities.urlIsEmpty<<
//                 "\nsa.root "<<selctionAbilities.root<<"\nsa.multiSelect "<<selctionAbilities.multiSelect<<
//                 "\nsa.singleSelect "<<selctionAbilities.singleSelect<<endl;
    return selctionAbilities;
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
    // FIXME Probably bug in QT, send bug report
    kDebug()<<"BookmarkFolderViewFilterModel::dropMimeData"<<endl;
    QModelIndex idx;
    if(row == -1)
        idx = parent;
    else
        idx = index(row, column, parent);
    QModelIndex src = mapToSource(idx);
    return sourceModel()->dropMimeData( data, action, -1, -1, src);
}

BookmarkFolderViewFilterModel::~BookmarkFolderViewFilterModel()
{
}

bool BookmarkFolderViewFilterModel::filterAcceptsColumn ( int source_column, const QModelIndex & source_parent ) const
{
    //Show name, hide everything else
    return (source_column == 0);
}

bool BookmarkFolderViewFilterModel::filterAcceptsRow ( int source_row, const QModelIndex & source_parent ) const
{
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    return static_cast<TreeItem *>(index.internalPointer())->bookmark().isGroup();
}

#include "bookmarklistview.moc"
