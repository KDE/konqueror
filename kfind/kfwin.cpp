/***********************************************************************
 *
 *  Kfwin.cpp
 *
 **********************************************************************/
#include "kfwin.h"

#include <QtCore/QTextStream>
#include <QtCore/QFileInfo>
#include <QtGui/QClipboard>
#include <QtGui/QHeaderView>
#include <QtCore/QDate>

#include <kfiledialog.h>
#include <klocale.h>
#include <kapplication.h>
#include <krun.h>
#include <kmessagebox.h>
#include <kglobal.h>
#include <kdebug.h>
#include <kiconloader.h>

#include <kio/netaccess.h>
#include <kio/copyjob.h>
#include <konq_popupmenu.h>
#include <konq_operations.h>
#include <knewmenu.h>

// Permission strings
static const char* perm[4] = {
  I18N_NOOP( "Read-write" ),
  I18N_NOOP( "Read-only" ),
  I18N_NOOP( "Write-only" ),
  I18N_NOOP( "Inaccessible" ) };
#define RW 0
#define RO 1
#define WO 2
#define NA 3

//BEGIN KFindItemModel

KFindItemModel::KFindItemModel( KfindWindow * parentView ) : 
    QStandardItemModel( parentView )
{
    reset();
}

void KFindItemModel::insertFileItem( KFileItem fileItem, QString matchingLine )
{
    QFileInfo fileInfo(fileItem.url().path());

    int perm_index;
    if(fileInfo.isReadable())
        perm_index = fileInfo.isWritable() ? RW : RO;
    else
        perm_index = fileInfo.isWritable() ? WO : NA;

    //Generate list item
    QList<QStandardItem*> items;
    
    //Size item (visible size + bytes for sorting)
    QStandardItem * sizeItem = new QStandardItem( KIO::convertSize( fileItem.size() ) );
    sizeItem->setData( fileItem.size(), Qt::UserRole ); 
    
    QStandardItem * dateItem = new QStandardItem( fileItem.timeString(KFileItem::ModificationTime) );
    dateItem->setData( fileItem.time(KFileItem::ModificationTime).toTime_t() , Qt::UserRole );
    
    items.append( new KFindItem( fileItem ) );
    items.append( new QStandardItem( (static_cast<KfindWindow*>(parent()))->reducedDir(fileItem.url().directory(KUrl::AppendTrailingSlash)) ) );
    items.append( sizeItem );
    items.append( dateItem );
    items.append( new QStandardItem( i18n(perm[perm_index]) ) );
    items.append( new QStandardItem( matchingLine ) );
    
    m_urlMap.insert( fileItem.url(), items.at(0) );
    
    appendRow( items );
}

/*
KUrl KFindItemModel::urlFromItem( QStandardItem * item )
{
    return m_urlMap.key( item );
}

KUrl KFindItemModel::urlFromIndex( const QModelIndex & index )
{
    return m_urlMap.key( item( index.row() ) );
}

QStandardItem * KFindItemModel::itemFromUrl( KUrl url )
{
    return m_urlMap.value( url );
}
*/

void KFindItemModel::removeItem( const KUrl & url )
{
    QStandardItem * item = m_urlMap.value( url );
    removeRow( item->index().row() );
    m_urlMap.remove( url );
}

bool KFindItemModel::isInserted( const KUrl & url )
{
    return m_urlMap.contains( url );
}

void KFindItemModel::reset()
{
    clear();
    setHorizontalHeaderLabels( QStringList() << i18n("Name") << i18n("In Subfolder") << i18n("Size") << i18n("Modified") << i18n("Permissions") << i18n("First Matching Line"));
}

Qt::ItemFlags KFindItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (index.isValid())
        return Qt::ItemIsDragEnabled | defaultFlags;
    return defaultFlags;
}
 
QMimeData * KFindItemModel::mimeData(const QModelIndexList &indexes) const
{
    KUrl::List uris;
    
    foreach ( const QModelIndex & index, indexes )
    {
        if( index.isValid())
        {
            if( index.column() == 0 ) //Only use the first column item
            {
                KFindItem * findItem = (KFindItem*)item( index.row() );
                if( findItem )
                    uris.append( findItem->fileItem.url() );
            }
        }
    }

    if ( uris.count() <= 0 )
        return 0;

    QMimeData * mimeData = new QMimeData();
    uris.populateMimeData( mimeData );
    
    return mimeData;
}

//END KFindItemModel

//BEGIN KFindItem

KFindItem::KFindItem( KFileItem _fileItem ):
    QStandardItem()
{
    fileItem = _fileItem;
    setText( fileItem.url().fileName(KUrl::ObeyTrailingSlash) );
    setIcon( QIcon( fileItem.pixmap( 16 ) ) );
}

//END KFindItem

//BEGIN KFindSortFilterProxyModel

bool KFindSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    //Order by UserData size in bytes or unix date
    if( left.column() == 2 || left.column() == 3)
    {
        qulonglong leftData = sourceModel()->data( left, Qt::UserRole ).toULongLong();
        qulonglong rightData = sourceModel()->data( right, Qt::UserRole ).toULongLong();

        return leftData < rightData;
    }
    // Default sorting rules for string values
    else
    {
        return QSortFilterProxyModel::lessThan( left, right );
    }
}

//END KFindSortFilterProxyModel

//BEGIN KfindWindow

KfindWindow::KfindWindow( QWidget *parent )
    : QTreeView( parent ) ,
    m_baseDir()
{
    //Configure model and proxy model
    m_model = new KFindItemModel( this );
    m_proxyModel = new KFindSortFilterProxyModel();
    m_proxyModel->setSourceModel( m_model );
    setModel( m_proxyModel );
    
    //Configure QTreeView
    setRootIsDecorated( false );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setSortingEnabled( true );
    setDragEnabled( true );
    setContextMenuPolicy( Qt::CustomContextMenu );
    
    connect( this, SIGNAL( customContextMenuRequested( const QPoint &) ),
                 this, SLOT( contextMenuRequested( const QPoint & )));
    connect( this, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(slotExecute(QModelIndex)) );
    
    //Generate popup menu actions
    m_actionCollection = new KActionCollection( this );

    KAction * open = KStandardAction::open(this, SLOT( slotExecuteSelected() ), this);
    m_actionCollection->addAction( "file_open", open );
    
    KAction * copy = KStandardAction::copy(this, SLOT( copySelection() ), this);
    m_actionCollection->addAction( "edit_copy", copy );
    
    KAction * openFolder = new KAction( KIcon("window-new"), i18n("&Open containing folder(s)"), this );
    connect( openFolder, SIGNAL(triggered()), this, SLOT( openContainingFolder() ) );
    m_actionCollection->addAction( "openfolder", openFolder );
    
    KAction * del = new KAction( KIcon("edit-delete"), i18n("&Delete"), this );
    connect( del, SIGNAL(triggered()), this, SLOT( deleteSelectedFiles() ) );
    del->setShortcut(Qt::SHIFT + Qt::Key_Delete);
    m_actionCollection->addAction( "del", del );
   
    KAction * trash = new KAction( KIcon("user-trash"), i18n("&Move to Trash"), this );
    connect( trash, SIGNAL(triggered()), this, SLOT( moveToTrashSelectedFiles() ) );
    trash->setShortcut(Qt::Key_Delete);
    m_actionCollection->addAction( "trash", trash );
    
    header()->setStretchLastSection( true );
    
    resetColumns();
}

KfindWindow::~KfindWindow()
{
    delete m_model;
    delete m_proxyModel;
    delete m_actionCollection;
}

QString KfindWindow::reducedDir(const QString& fullDir)
{
    if (fullDir.indexOf(m_baseDir)==0)
    {
        QString tmp=fullDir.mid(m_baseDir.length());
        return tmp;
    };
    return fullDir;
}

void KfindWindow::beginSearch(const KUrl& baseUrl)
{
    kDebug() << QString("beginSearch in: %1").arg(baseUrl.path());
    m_baseDir = baseUrl.path(KUrl::AddTrailingSlash);
    m_model->reset();
}

void KfindWindow::endSearch()
{
}

void KfindWindow::insertItem(const KFileItem &item, const QString& matchingLine)
{
    m_model->insertFileItem( item, matchingLine );
    sortByColumn( 0, Qt::AscendingOrder );
    resetColumns();
}

// copy to clipboard
void KfindWindow::copySelection()
{
    QMimeData * mime = m_model->mimeData( m_proxyModel->mapSelectionToSource( selectionModel()->selection() ).indexes() );
    if (mime)
    {
        QClipboard * cb = kapp->clipboard();
        cb->setMimeData( mime );
    }
}

void KfindWindow::saveResults()
{
    QMap<KUrl, QStandardItem*> urls = m_model->urls();

    KFileDialog *dlg = new KFileDialog(QString(), QString(), this);
    dlg->setOperationMode (KFileDialog::Saving);
    dlg->setCaption( i18n("Save Results As") );
    dlg->setFilter( QString("*.html|%1\n*.txt|%2").arg( i18n("HTML page"), i18n("Text file") ) );
    dlg->setConfirmOverwrite(true);    
    
    dlg->exec();

    KUrl u = dlg->selectedUrl();
    
    QString filter = dlg->currentFilter();
    delete dlg;

    if (!u.isValid() || !u.isLocalFile())
        return;

    QString filename = u.toLocalFile();

    QFile file(filename);

    if ( !file.open(QIODevice::WriteOnly) )
    {
        KMessageBox::error(parentWidget(),
                i18n("Unable to save results."));
    }
    else
    {
        QTextStream stream( &file );
        stream.setCodec( QTextCodec::codecForLocale() );

        if ( filter == "*.html" ) 
        {
            stream << QString::fromLatin1("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\""
            "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\"><html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
                            "<head>\n"
                            "<title>%2</title></head>\n"
                            "<body>\n<h1>%2</h1>\n"
                            "<dl>\n")
            .arg(i18n("KFind Results File"));

            Q_FOREACH( const KUrl & url, urls.keys() )
            {
                QString path = url.url();
                QString pretty = url.prettyUrl();
                
                stream << QString::fromLatin1("<dt><a href=\"%1\">%2</a></dt>\n").arg( path, pretty );

            }
            stream << QString::fromLatin1("</dl>\n</body>\n</html>\n");
        }
        else 
        {
            Q_FOREACH( const KUrl & url, urls.keys() )
            {
                QString path= url.url();
                stream << path << endl;
            }
        }

        file.close();
        KMessageBox::information(parentWidget(),
                    i18n("Results were saved to file\n")+
                    filename);
    }
}

void KfindWindow::openContainingFolder()
{
    KUrl::List uris = selectedUrls();
    QMap<KUrl, int> folderMaps;
    
    //Generate *unique* folders
    Q_FOREACH( const KUrl & url, uris )
    {
        KUrl dir = url;
        dir.setFileName( QString() );
        folderMaps.insert( dir, 0 );
    }

    Q_FOREACH( const KUrl & url, folderMaps.keys() )
    {
        (void) new KRun(url, this);
    }
}

void KfindWindow::slotExecuteSelected()
{
    QModelIndexList selected = m_proxyModel->mapSelectionToSource( selectionModel()->selection() ).indexes();
    if ( selected.size() == 0 )
        return;
        
    Q_FOREACH( const QModelIndex & index, selected )
    {
        if( index.column() == 0 )
        {
            KFindItem * findItem = (KFindItem*)m_model->itemFromIndex( index );
            if( findItem )
                findItem->fileItem.run();
        }
    }
}

void KfindWindow::slotExecute( const QModelIndex & index )
{
    if ( !index.isValid() )
        return;
        
    QModelIndex realIndex = m_proxyModel->mapToSource( index );
    
    if ( !realIndex.isValid() )
        return;
        
    KFindItem * findItem = (KFindItem *)m_model->item( realIndex.row() );
    if( findItem )
        findItem->fileItem.run();
}

void KfindWindow::resizeEvent(QResizeEvent *e)
{
    Q_UNUSED( e );
    resetColumns();
}

void KfindWindow::resetColumns()
{
    resizeColumnToContents( 0 );
    resizeColumnToContents( 1 );
}

void KfindWindow::contextMenuRequested( const QPoint & p)
{
    KFileItemList fileList;
    
    QModelIndexList selected = m_proxyModel->mapSelectionToSource( selectionModel()->selection() ).indexes();
    if ( selected.size() == 0 )
        return;
    
    Q_FOREACH( const QModelIndex & index, selected )
    {
        if( index.column() == 0 )
        {
            KFindItem * findItem = (KFindItem*)m_model->itemFromIndex( index );
            if( findItem )
                fileList.append( findItem->fileItem );
        }
    }
    
    KParts::BrowserExtension::PopupFlags flags = KParts::BrowserExtension::ShowProperties; // | KParts::BrowserExtension::ShowUrlOperations;
    
    QList<QAction*> editActions;
    editActions.append(m_actionCollection->action("file_open"));
    editActions.append(m_actionCollection->action("openfolder"));
    editActions.append(m_actionCollection->action("edit_copy"));
    editActions.append(m_actionCollection->action("del"));
    editActions.append(m_actionCollection->action("trash"));
    
    KParts::BrowserExtension::ActionGroupMap actionGroups;
    actionGroups.insert("editactions", editActions);
    
    KonqPopupMenu * menu = new KonqPopupMenu( fileList, KUrl(), *m_actionCollection, new KNewMenu( m_actionCollection, this, "new_menu"), 0, flags, this, 0, actionGroups);

    menu->popup( this->mapToGlobal( p ) );
    connect( menu, SIGNAL(aboutToHide()), menu, SLOT(deleteLater()) );
}

KUrl::List KfindWindow::selectedUrls()
{
    KUrl::List uris;
    
    QModelIndexList indexes = m_proxyModel->mapSelectionToSource( selectionModel()->selection() ).indexes();
    Q_FOREACH( const QModelIndex & index, indexes )
    {
        if( index.column() == 0 && index.isValid() )
        {
            KFindItem * item = (KFindItem*)m_model->itemFromIndex( index );
            if( item )
                uris.append( item->fileItem.url() );
        }
    }
    
    return uris;
}

void KfindWindow::deleteSelectedFiles()
{
    KUrl::List uris = selectedUrls();
    
    bool done = KonqOperations::askDeleteConfirmation( uris, KonqOperations::DEL, KonqOperations::FORCE_CONFIRMATION, this );
    if ( done )
    {
        Q_FOREACH( const KUrl & url, uris )
            KIO::NetAccess::del( url, this );
        
        //This should be done by KDirWatch integration in the main dialog, but it could fail?
        Q_FOREACH( const KUrl & url, uris )
            m_model->removeItem( url );
    }
}

void KfindWindow::moveToTrashSelectedFiles()
{
    KUrl::List uris = selectedUrls();
    
    bool done = KonqOperations::askDeleteConfirmation( uris, KonqOperations::TRASH, KonqOperations::FORCE_CONFIRMATION, this );
    if ( done )
    {
        KIO::trash( uris );

        //This should be done by KDirWatch integration in the main dialog, but it could fail?
        Q_FOREACH( const KUrl & url, uris )
            m_model->removeItem( url );
    }
}

//END KfindWindow

#include "kfwin.moc"
