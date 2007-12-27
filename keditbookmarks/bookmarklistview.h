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

#ifndef __bookmarklistview_h
#define __bookmarklistview_h

#include <QtGui/QTreeView>
#include <QSortFilterProxyModel>

class KBookmarkModel;
class KBookmark;
class BookmarkListView;
class BookmarkFolderViewFilterModel;

class BookmarkView : public QTreeView
{
    Q_OBJECT
public:
    BookmarkView( QWidget * parent = 0 );
    virtual ~BookmarkView();    
    virtual KBookmark bookmarkForIndex(const QModelIndex & idx) const = 0;
};

class BookmarkFolderView : public BookmarkView
{
    Q_OBJECT
public:
    explicit BookmarkFolderView( BookmarkListView * view, QWidget * parent = 0 );
    virtual ~BookmarkFolderView();
    virtual void selectionChanged ( const QItemSelection & selected, const QItemSelection & deselected );
    virtual KBookmark bookmarkForIndex(const QModelIndex & idx) const;
private:
    BookmarkListView * mview;
    BookmarkFolderViewFilterModel * mmodel;
};

class BookmarkListView : public BookmarkView
{
    Q_OBJECT
public:
    BookmarkListView( QWidget * parent = 0 );
    virtual ~BookmarkListView();
    void loadColumnSetting();
    void saveColumnSetting ();
    virtual void setModel(QAbstractItemModel * model);
    virtual KBookmark bookmarkForIndex(const QModelIndex & idx) const;
    KBookmarkModel * bookmarkModel() const;
protected:
    virtual void contextMenuEvent ( QContextMenuEvent * e );
};


class BookmarkFolderViewFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    BookmarkFolderViewFilterModel(QObject * parent = 0);
    virtual ~BookmarkFolderViewFilterModel();
    virtual QStringList mimeTypes() const;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
protected:
    bool filterAcceptsColumn ( int source_column, const QModelIndex & source_parent ) const;
    bool filterAcceptsRow ( int source_row, const QModelIndex & source_parent ) const;
    //FIXME check
    virtual Qt::DropActions supportedDropActions() const
        { return sourceModel()->supportedDropActions(); }
};
#endif
