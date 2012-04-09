/*******************************************************************
* kfindtreeview.h
* Copyright 2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
* 
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of 
* the License, or (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* 
******************************************************************/

#ifndef KFINDTREEVIEW__H
#define KFINDTREEVIEW__H

#include <QTreeView>
#include <QtCore/QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QDragMoveEvent>

#include <kurl.h>
#include <kicon.h>
#include <kdebug.h>
#include <kfileitem.h>
#include <konq_popupmenu.h>

class KFindTreeView;
class KActionCollection;
class KfindDlg;

class KFindItem
{
    public:
        explicit KFindItem( const KFileItem & = KFileItem(), const QString & subDir = QString(), const QString & matchingLine = QString() );
        
        QVariant data(int column, int role) const;
        
        KFileItem getFileItem() const { return m_fileItem; }
        bool isValid() const { return !m_fileItem.isNull(); }
        
    private:
        KFileItem       m_fileItem;
        QString         m_matchingLine;
        QString         m_subDir;
        QString         m_permission;
        KIcon           m_icon;
};
 
class KFindItemModel: public QAbstractTableModel
{
    public:
        KFindItemModel( KFindTreeView* parent);

        void insertFileItems( const QList< QPair<KFileItem,QString> > &);

        void removeItem(const KUrl &);
        bool isInserted(const KUrl &);
        
        void clear();
        
        Qt::DropActions supportedDropActions() const { return Qt::CopyAction | Qt::MoveAction; }
        
        Qt::ItemFlags flags(const QModelIndex &) const;
        QMimeData * mimeData(const QModelIndexList &) const;
        
        int columnCount ( const QModelIndex & parent = QModelIndex() ) const {  Q_UNUSED(parent); return 6; }
        int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
        QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const;
        
        KFindItem itemAtIndex( const QModelIndex & index ) const;
        
        QList<KFindItem> getItemList() const { return m_itemList; }
        
    private:
        QList<KFindItem>    m_itemList;
        KFindTreeView*        m_view;
};

class KFindSortFilterProxyModel: public QSortFilterProxyModel
{
    Q_OBJECT
    
    public:
        KFindSortFilterProxyModel(QObject * parent = 0):
            QSortFilterProxyModel(parent){};

    protected:
        bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

};

class KFindTreeView: public QTreeView
{
  Q_OBJECT
    public:
        KFindTreeView( QWidget * parent, KfindDlg * findDialog);
        ~KFindTreeView();

        void beginSearch(const KUrl& baseUrl);
        void endSearch();

        void insertItems(const QList< QPair<KFileItem,QString> > &);
        void removeItem(const KUrl & url);
        
        bool isInserted(const KUrl & url) { return m_model->isInserted( url ); }
        
        QString reducedDir(const QString& fullDir);
        
        int itemCount() { return m_model->rowCount(); }
        
    public Q_SLOTS:
        void copySelection();
        void contextMenuRequested( const QPoint & p );

    private Q_SLOTS:
        KUrl::List selectedUrls();
        
        void deleteSelectedFiles();
        void moveToTrashSelectedFiles();
        
        void slotExecute( const QModelIndex & index );
        void slotExecuteSelected();
        
        void openContainingFolder();
        void saveResults();
        
        void reconfigureMouseSettings();
        void updateMouseButtons();
   
    protected:
        void dragMoveEvent( QDragMoveEvent *e ) { e->accept(); }

    Q_SIGNALS:
        void resultSelected(bool);

    private:
        void resizeToContents();
        
        QString                     m_baseDir;
        
        KFindItemModel *            m_model;
        KFindSortFilterProxyModel * m_proxyModel;
        KActionCollection *         m_actionCollection;
        KonqPopupMenu *             m_contextMenu;
        
        Qt::MouseButtons            m_mouseButtons;

        KfindDlg *                  m_kfindDialog;
};

#endif
