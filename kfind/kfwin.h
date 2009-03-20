/***********************************************************************
 *
 *  Kfwin.h
 *
 ***********************************************************************/

#ifndef KFWIN__H
#define KFWIN__H

#include <QtGui/QTreeView>
#include <QtGui/QStandardItemModel>
#include <QtGui/QSortFilterProxyModel>
#include <QtGui/QDragMoveEvent>

#include <kurl.h>
#include <kfileitem.h>

class KfindWindow;
class KActionCollection;


class KFindItem: public QStandardItem
{
    public:
        KFindItem( KFileItem );
        KFileItem  fileItem;
};

class KFindItemModel: public QStandardItemModel
{
    public:
        KFindItemModel( KfindWindow* parent);

        void insertFileItem( KFileItem, QString );
        void removeItem(const KUrl &);
        bool isInserted(const KUrl &);
        void reset();
        
        Qt::DropActions supportedDropActions() const
        {
            return Qt::CopyAction | Qt::MoveAction;
        }
        
        Qt::ItemFlags flags(const QModelIndex &) const;
        QMimeData * mimeData(const QModelIndexList &) const;
        
        /*
        KUrl urlFromItem( QStandardItem * );
        KUrl urlFromIndex( const QModelIndex & );
        QStandardItem * itemFromUrl( KUrl );
        */
        
        QMap<KUrl, QStandardItem*> urls() { return m_urlMap; }
        
    private:
        QMap<KUrl, QStandardItem*> m_urlMap;
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

class KfindWindow: public QTreeView
{
  Q_OBJECT
    public:
        KfindWindow( QWidget * parent = 0 );
        ~KfindWindow();

        void beginSearch(const KUrl& baseUrl);
        void endSearch();

        void insertItem(const KFileItem &item, const QString& matchingLine);
        void removeItem(const KUrl & url) { m_model->removeItem( url ); }
        
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
   
    protected:
        void dragMoveEvent( QDragMoveEvent *e ) { e->accept(); }
        void resizeEvent( QResizeEvent *e );

    Q_SIGNALS:
        void resultSelected(bool);

    private:
        void resetColumns();
        
        QString m_baseDir;
        
        KFindItemModel *    m_model;
        KFindSortFilterProxyModel * m_proxyModel;
        KActionCollection *     m_actionCollection;
};

#endif
