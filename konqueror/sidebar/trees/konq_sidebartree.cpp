/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
                 2000 Carsten Pfeiffer <pfeiffer@kde.org>
                 2003 Waldo Bastian <bastian@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_sidebartree.h"
#include "konq_sidebartreemodule.h"
#include "konqsidebar_oldtreemodule.h"

#include <QClipboard>
#include <QCursor>
#include <QtCore/QDir>
#include <Qt3Support/Q3Header>
#include <QMenu>
#include <QtCore/QTimer>
#include <QApplication>
#include <QPixmap>
#include <QKeyEvent>
#include <QtCore/QEvent>
#include <QFrame>

#include <kaction.h>
#include <kactioncollection.h>
#include <kdebug.h>
#include <kdirnotify.h>
#include <kdesktopfile.h>
#include <kglobalsettings.h>
#include <klibrary.h>
#include <kicon.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kshell.h>
#include <kpropertiesdialog.h>
#include <kprotocolinfo.h>
#include <kstandarddirs.h>
#include <k3urldrag.h>

#include <stdlib.h>
#include <assert.h>


static const int autoOpenTimeout = 750;


getModule KonqSidebarTree::getPluginFactory(const QString &name)
{
  if (!pluginFactories.contains(name))
  {
    QString libName    = pluginInfo[name];
    KLibrary lib(libName);
    if (lib.load())
    {
      // get the create_ function
      QString factory = "create_" + libName;
      KLibrary::void_function_ptr create    = lib.resolveFunction(QFile::encodeName(factory));
      if (create)
      {
        getModule func = (getModule)create;
        pluginFactories.insert(name, func);
        kDebug()<<"Added a module";
      }
      else
      {
        kWarning()<<"No create function found in"<<libName;
      }
    }
    else
      kWarning() << "Module " << libName << " can't be loaded!" ;
  }

  return pluginFactories[name];
}

void KonqSidebarTree::loadModuleFactories()
{
  pluginFactories.clear();
  pluginInfo.clear();
  KStandardDirs *dirs=KGlobal::dirs();
  const QStringList list=dirs->findAllResources("data","konqsidebartng/dirtree/*.desktop",KStandardDirs::NoDuplicates);


  for (QStringList::ConstIterator it=list.begin();it!=list.end();++it)
  {
    KConfig _ksc( *it, KConfig::SimpleConfig );
    KConfigGroup ksc(&_ksc, "Desktop Entry");
    QString name    = ksc.readEntry("X-KDE-TreeModule");
    QString libName = ksc.readEntry("X-KDE-TreeModule-Lib");
    if ((name.isEmpty()) || (libName.isEmpty()))
        {kWarning()<<"Bad Configuration file for a dirtree module "<<*it; continue;}

    //Register the library info.
    pluginInfo[name] = libName;
  }
}


class KonqSidebarTree_Internal
{
public:
    DropAcceptType m_dropMode;
    QStringList m_dropFormats;
};


KonqSidebarTree::KonqSidebarTree( KonqSidebarOldTreeModule *parent, QWidget *parentWidget, ModuleType moduleType, const QString& path )
    : K3ListView( parentWidget ),
      m_currentTopLevelItem( 0 ),
      m_scrollingLocked( false ),
      m_collection( 0 )
{
    d = new KonqSidebarTree_Internal;
    d->m_dropMode = SidebarTreeMode;

    loadModuleFactories();

    setAcceptDrops( true );
    viewport()->setAcceptDrops( true );
    installEventFilter(this);
    m_lstModules.setAutoDelete( true );

    setSelectionMode( Q3ListView::Single );
    setDragEnabled(true);

    m_sidebarModule = parent;

    m_animationTimer = new QTimer( this );
    connect( m_animationTimer, SIGNAL(timeout()),
             this, SLOT(slotAnimation()) );

    m_currentBeforeDropItem = 0;
    m_dropItem = 0;
    m_bOpeningFirstChild=false;

    addColumn( QString() );
    header()->hide();
    setTreeStepSize(15);

    m_autoOpenTimer = new QTimer( this );
    connect( m_autoOpenTimer, SIGNAL(timeout()),
             this, SLOT(slotAutoOpenFolder()) );

    connect( this, SIGNAL(doubleClicked(Q3ListViewItem*)),
             this, SLOT(slotDoubleClicked(Q3ListViewItem*)) );
    connect( this, SIGNAL(mouseButtonPressed(int,Q3ListViewItem*,QPoint,int)),
             this, SLOT(slotMouseButtonPressed(int,Q3ListViewItem*,QPoint,int)) );
    connect( this, SIGNAL(mouseButtonClicked(int,Q3ListViewItem*,QPoint,int)),
             this, SLOT(slotMouseButtonClicked(int,Q3ListViewItem*,QPoint,int)) );
    connect( this, SIGNAL(returnPressed(Q3ListViewItem*)),
             this, SLOT(slotDoubleClicked(Q3ListViewItem*)) );
    connect( this, SIGNAL(selectionChanged()),
             this, SLOT(slotSelectionChanged()) );
    connect(qApp->clipboard(), SIGNAL(dataChanged()),
            this, SLOT(slotSelectionChanged())); // so that "paste" can be updated

    connect( this, SIGNAL(itemRenamed(Q3ListViewItem*,QString,int)),
             this, SLOT(slotItemRenamed(Q3ListViewItem*,QString,int)));

    if (moduleType == VIRT_Folder) {
        m_dirtreeDir.dir.setPath(KGlobal::dirs()->saveLocation("data","konqsidebartng/virtual_folders/"+path+'/'));
        m_dirtreeDir.relDir = path;
    } else {
        m_dirtreeDir.dir.setPath( path );
    }
    kDebug(1201) << m_dirtreeDir.dir.path();
    m_dirtreeDir.type = moduleType;
    // Initial parsing
    rescanConfiguration();

    if (firstChild())
    {
        m_bOpeningFirstChild = true;
        firstChild()->setOpen(true);
        m_bOpeningFirstChild = false;
    }

    OrgKdeKDirNotifyInterface *kdirnotify = new OrgKdeKDirNotifyInterface(QString(), QString(), QDBusConnection::sessionBus());
    kdirnotify->setParent(this);
    connect(kdirnotify, SIGNAL(FilesAdded(QString)), SLOT(slotFilesAdded(QString)));
    connect(kdirnotify, SIGNAL(FilesChanged(QStringList)), SLOT(slotFilesChanged(QStringList)));
    connect(kdirnotify, SIGNAL(FilesRemoved(QStringList)), SLOT(slotFilesRemoved(QStringList)));

    m_collection = new KActionCollection(this);
    m_collection->addAssociatedWidget(this);
    m_collection->setObjectName( QLatin1String("bookmark actions" ));
    QAction *action = new KAction(KIcon("folder-new"), i18n("&Create New Folder..."), this);
    m_collection->addAction("create_folder", action);
    connect(action, SIGNAL(triggered(bool)), SLOT(slotCreateFolder()));

    action = new KAction(KIcon("edit-delete"), i18n("Delete Folder"), this);
    m_collection->addAction("delete", action);
    connect(action, SIGNAL(triggered(bool)), SLOT(slotDelete()));

    action = new KAction(KIcon("user-trash"), i18n("Move to Trash"), this);
    m_collection->addAction("trash", action);
    connect(action, SIGNAL(triggered(bool)), SLOT(slotTrash()));

    action = new KAction(i18n("Rename"), this);
    action->setIcon(KIcon("edit-rename"));
    m_collection->addAction("rename", action);
    connect(action, SIGNAL(triggered(bool)), SLOT(slotRename()));

    action = new KAction(KIcon("edit-delete"), i18n("Delete Link"), this);
    m_collection->addAction("delete_link", action);
    connect(action, SIGNAL(triggered(bool)), SLOT(slotDelete()));

    action = new KAction(KIcon("document-properties"), i18n("Properties"), this);
    m_collection->addAction("item_properties", action);
    connect(action, SIGNAL(triggered(bool)), SLOT(slotProperties()));

    action = new KAction(KIcon("window-new"), i18n("Open in New Window"), this);
    m_collection->addAction("open_window", action);
    connect(action, SIGNAL(triggered(bool)), SLOT(slotOpenNewWindow()));

    action = new KAction(KIcon("tab-new"), i18n("Open in New Tab"), this);
    m_collection->addAction("open_tab", action);
    connect(action, SIGNAL(triggered(bool)), SLOT(slotOpenTab()));

    action = new KAction(KIcon("edit-copy"), i18n("Copy Link Address"), this);
    m_collection->addAction("copy_location", action);
    connect(action, SIGNAL(triggered(bool)), SLOT(slotCopyLocation()));
}

KonqSidebarTree::~KonqSidebarTree()
{
    clearTree();

    delete d;
}

void KonqSidebarTree::itemDestructed( KonqSidebarTreeItem *item )
{
    stopAnimation(item);

    if (item == m_currentBeforeDropItem)
    {
       m_currentBeforeDropItem = 0;
    }
}

void KonqSidebarTree::setDropFormats(const QStringList &formats)
{
    d->m_dropFormats = formats;
}

void KonqSidebarTree::clearTree()
{
    m_lstModules.clear();
    m_topLevelItems.clear();
    m_mapCurrentOpeningFolders.clear();
    m_currentBeforeDropItem = 0;
    clear();

    if (m_dirtreeDir.type == VIRT_Folder)
    {
        setRootIsDecorated( true );
    }
    else
    {
        setRootIsDecorated( false );
    }
}

void KonqSidebarTree::followURL( const KUrl &url )
{
    // Maybe we're there already ?
    KonqSidebarTreeItem *selection = static_cast<KonqSidebarTreeItem *>( selectedItem() );
    if (selection && selection->externalURL().equals( url, KUrl::CompareWithoutTrailingSlash ))
    {
        ensureItemVisible( selection );
        return;
    }

    kDebug(1201) << url.url();
    Q3PtrListIterator<KonqSidebarTreeTopLevelItem> topItem ( m_topLevelItems );
    for (; topItem.current(); ++topItem )
    {
        if ( topItem.current()->externalURL().isParentOf( url ) )
        {
            topItem.current()->module()->followURL( url );
            return; // done
        }
    }
    kDebug(1201) << "Not found";
}

void KonqSidebarTree::contentsDragEnterEvent( QDragEnterEvent *ev )
{
    m_dropItem = 0;
    m_currentBeforeDropItem = selectedItem();
    // Save the available formats
    m_lstDropFormats.clear();
    for( int i = 0; ev->format( i ); i++ )
      if ( *( ev->format( i ) ) )
         m_lstDropFormats.append( ev->format( i ) );
    K3ListView::contentsDragEnterEvent(ev);
}

void KonqSidebarTree::contentsDragMoveEvent( QDragMoveEvent *e )
{
    Q3ListViewItem *item = itemAt( contentsToViewport( e->pos() ) );

    // Accept drops on the background, if URLs
    if ( !item && m_lstDropFormats.contains("text/uri-list") )
    {
        m_dropItem = 0;
        e->acceptProposedAction();
        if (selectedItem())
        setSelected( selectedItem(), false ); // no item selected
        return;
    }

    if (item && static_cast<KonqSidebarTreeItem*>(item)->acceptsDrops( m_lstDropFormats )) {
        d->m_dropMode = SidebarTreeMode;

        if ( !item->isSelectable() )
        {
            m_dropItem = 0;
            m_autoOpenTimer->stop();
            e->ignore();
            return;
        }

        e->acceptProposedAction();

        setSelected( item, true );

        if ( item != m_dropItem )
        {
            m_autoOpenTimer->stop();
            m_dropItem = item;
            m_autoOpenTimer->start( autoOpenTimeout );
        }
    } else {
        d->m_dropMode = K3ListViewMode;
        K3ListView::contentsDragMoveEvent(e);
    }
}

void KonqSidebarTree::contentsDragLeaveEvent( QDragLeaveEvent *ev )
{
    // Restore the current item to what it was before the dragging (#17070)
    if ( m_currentBeforeDropItem )
        setSelected( m_currentBeforeDropItem, true );
    else
        setSelected( m_dropItem, false ); // no item selected
    m_currentBeforeDropItem = 0;
    m_dropItem = 0;
    m_lstDropFormats.clear();

    if (d->m_dropMode == K3ListViewMode) {
        K3ListView::contentsDragLeaveEvent(ev);
    }
}

void KonqSidebarTree::contentsDropEvent( QDropEvent *ev )
{
    if (d->m_dropMode == SidebarTreeMode) {
        m_autoOpenTimer->stop();

        if ( !selectedItem() )
        {
    //        KonqOperations::doDrop( 0L, m_dirtreeDir.dir, ev, this );
            KUrl::List urls;
            if ( K3URLDrag::decode( ev, urls ) )
            {
               for(KUrl::List::ConstIterator it = urls.constBegin();
                   it != urls.constEnd(); ++it)
               {
                  addUrl(0, *it);
               }
            }
        }
        else
        {
            KonqSidebarTreeItem *selection = static_cast<KonqSidebarTreeItem *>( selectedItem() );
            selection->drop( ev );
        }
    } else {
        K3ListView::contentsDropEvent(ev);
    }
}

static QString findUniqueFilename(const QString &path, const QString &filename)
{
    QString tempFilename = filename;
    if (tempFilename.endsWith(".desktop"))
       tempFilename.truncate(tempFilename.length()-8);

    QString name = tempFilename;
    int n = 2;
    while(QFile::exists(path + tempFilename + ".desktop"))
    {
       tempFilename = QString("%2_%1").arg(n++).arg(name);
    }
    return path+tempFilename+".desktop";
}

void KonqSidebarTree::addUrl(KonqSidebarTreeTopLevelItem* item, const KUrl & url)
{
    QString path;
    if (item)
       path = item->path();
    else
       path = m_dirtreeDir.dir.path();

    KUrl destUrl;

    if (url.isLocalFile() && url.fileName().endsWith(".desktop"))
    {
       QString filename = findUniqueFilename(path, url.fileName());
       destUrl.setPath(filename);
       KIO::NetAccess::file_copy(url, destUrl, this);
    }
    else
    {
       QString name = url.host();
       if (name.isEmpty())
          name = url.fileName();
       QString filename = findUniqueFilename(path, name);
       destUrl.setPath(filename);

       KDesktopFile desktopFile(filename);
       KConfigGroup cfg = desktopFile.desktopGroup();
       cfg.writeEntry("Encoding", "UTF-8");
       cfg.writeEntry("Type","Link");
       cfg.writeEntry("URL", url.url());
       QString icon = "folder";
       if (!url.isLocalFile())
          icon = KMimeType::favIconForUrl(url);
       if (icon.isEmpty())
          icon = KProtocolInfo::icon( url.protocol() );
       cfg.writeEntry("Icon", icon);
       cfg.writeEntry("Name", name);
       cfg.writeEntry("Open", false);
       cfg.sync();
    }

    destUrl.setPath( destUrl.directory() );
    OrgKdeKDirNotifyInterface::emitFilesAdded( destUrl.url() );

    if (item)
       item->setOpen(true);
}

bool KonqSidebarTree::acceptDrag(QDropEvent* e) const
{
    // for K3ListViewMode...
    for( int i = 0; e->format( i ); i++ )
        if ( d->m_dropFormats.contains(e->format( i ) ) )
            return true;
    return false;
}

Q3DragObject* KonqSidebarTree::dragObject()
{
    KonqSidebarTreeItem* item = static_cast<KonqSidebarTreeItem *>( selectedItem() );
    if ( !item )
        return 0;

    QMimeData *mimeData = new QMimeData;
    if ( item->populateMimeData( mimeData, false ) )
    {
        QDrag* drag = new QDrag( viewport() );
        drag->setMimeData(mimeData);
        const QPixmap *pix = item->pixmap(0);
        if ( pix && drag->pixmap().isNull() )
            drag->setPixmap( *pix );
    }
    else
    {
        delete mimeData;
        mimeData = 0;
    }

#ifdef __GNUC__
#warning TODO port to QDrag (only possible once porting away from Q3ListView?)
#endif
    return 0;
    //return drag;
}

void KonqSidebarTree::leaveEvent( QEvent *e )
{
    K3ListView::leaveEvent( e );
//    emitStatusBarText( QString() );
}


void KonqSidebarTree::slotDoubleClicked( Q3ListViewItem *item )
{
    //kDebug(1201) << item;
    if ( !item )
        return;

    if ( !static_cast<KonqSidebarTreeItem*>(item)->isClickable() )
        return;

    slotExecuted( item );
    item->setOpen( !item->isOpen() );
}

void KonqSidebarTree::slotExecuted( Q3ListViewItem *item )
{
    kDebug(1201) << item;
    if ( !item )
        return;

    if ( !static_cast<KonqSidebarTreeItem*>(item)->isClickable() )
        return;

    KonqSidebarTreeItem *dItem = static_cast<KonqSidebarTreeItem *>( item );

    KParts::OpenUrlArguments args;
    args.setMimeType(dItem->externalMimeType());
    KParts::BrowserArguments browserArgs;
    browserArgs.trustedSource = true; // is this needed?
    KUrl externalURL = dItem->externalURL();
    if ( !externalURL.isEmpty() )
        openUrlRequest( externalURL, args, browserArgs );
}

void KonqSidebarTree::slotMouseButtonPressed( int _button, Q3ListViewItem* _item, const QPoint&, int col )
{
    KonqSidebarTreeItem * item = static_cast<KonqSidebarTreeItem*>( _item );
    if (_button == Qt::RightButton)
    {
        if ( item && col < 2)
        {
            item->setSelected( true );
            item->rightButtonPressed();
        }
    }
}

void KonqSidebarTree::slotMouseButtonClicked(int _button, Q3ListViewItem* _item, const QPoint&, int col)
{
    KonqSidebarTreeItem * item = static_cast<KonqSidebarTreeItem*>(_item);
    if(_item && col < 2)
    {
        switch( _button ) {
        case Qt::LeftButton:
            slotExecuted( item );
            break;
        case Qt::MidButton:
            item->middleButtonClicked();
            break;
        }
    }
}

void KonqSidebarTree::slotAutoOpenFolder()
{
    m_autoOpenTimer->stop();

    if ( !m_dropItem || m_dropItem->isOpen() )
        return;

    m_dropItem->setOpen( true );
    m_dropItem->repaint();
}

void KonqSidebarTree::rescanConfiguration()
{
    kDebug(1201);
    m_autoOpenTimer->stop();
    clearTree();
    if (m_dirtreeDir.type==VIRT_Folder)
    {
        kDebug(1201) << "-->scanDir";
        scanDir( 0, m_dirtreeDir.dir.path(), true);

    }
    else
    {
        kDebug(1201)<<"-->loadTopLevel";
        loadTopLevelItem( 0, m_dirtreeDir.dir.path() );
    }
}

void KonqSidebarTree::slotSelectionChanged()
{
    if ( !m_dropItem ) // don't do this while the dragmove thing
    {
        KonqSidebarTreeItem * item = static_cast<KonqSidebarTreeItem *>( selectedItem() );
        if ( item )
            item->itemSelected();
        /* else   -- doesn't seem to happen
           {} */
    }
}

void KonqSidebarTree::slotFilesAdded( const QString & dir )
{
    KUrl urlDir( dir );
    kDebug(1201) << dir;
    if ( m_dirtreeDir.dir.isParentOf( urlDir ) )
        // We use a timer in case of DBus re-entrance..
        QTimer::singleShot( 0, this, SLOT(rescanConfiguration()) );
}

void KonqSidebarTree::slotFilesRemoved( const QStringList & urls )
{
    //kDebug(1201) << "KonqSidebarTree::slotFilesRemoved " << urls.count();
    for ( QStringList::ConstIterator it = urls.constBegin() ; it != urls.constEnd() ; ++it )
    {
        KUrl u( *it );
        //kDebug(1201) <<  "KonqSidebarTree::slotFilesRemoved " << u;
        if ( m_dirtreeDir.dir.isParentOf( u ) )
        {
            QTimer::singleShot( 0, this, SLOT(rescanConfiguration()) );
            kDebug(1201) << "done";
            return;
        }
    }
}

void KonqSidebarTree::slotFilesChanged( const QStringList & urls )
{
    //kDebug(1201) << "KonqSidebarTree::slotFilesChanged";
    // not same signal, but same implementation
    slotFilesRemoved( urls );
}

void KonqSidebarTree::scanDir( KonqSidebarTreeItem *parent, const QString &path, bool isRoot )
{
    QDir dir( path );

    if ( !dir.isReadable() )
        return;

    kDebug(1201) << "scanDir" << path;

    QStringList entries = dir.entryList( QDir::Files );
    QStringList dirEntries = dir.entryList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);

    if ( isRoot )
    {
        bool copyConfig = (entries.isEmpty() && dirEntries.isEmpty());
        if (!copyConfig)
        {
            // Check version number
            // Version 1 was the dirtree of KDE 2.0.x (no versioning at that time, so default)
            // Version 2 includes the history
            // Version 3 includes the bookmarks
            // Version 4 includes lan.desktop and floppy.desktop, Alex
            // Version 5 includes the audiocd browser
            // Version 6 includes the printmanager and lan browser
            const int currentVersion = 6;
            QString key = QString::fromLatin1("X-KDE-DirTreeVersionNumber");
            KConfig versionCfg( path + "/.directory", KConfig::SimpleConfig);
            KConfigGroup generalGroup( &versionCfg, "General" );
            int versionNumber = generalGroup.readEntry( key, 1 );
            kDebug(1201) << "found version " << versionNumber;
            if ( versionNumber < currentVersion ) {
                generalGroup.writeEntry( key, currentVersion );
                versionCfg.sync();
                copyConfig = true;
            }
        }
        if (copyConfig) {
            // We will copy over the configuration for the dirtree, from the global directory
            const QStringList dirtree_dirs = KGlobal::dirs()->findDirs("data","konqsidebartng/virtual_folders/"+m_dirtreeDir.relDir+'/');


//            QString dirtree_dir = KGlobal::dirs()->findDirs("data","konqsidebartng/virtual_folders/"+m_dirtreeDir.relDir+"/").last();  // most global
//            kDebug(1201) << "dirtree_dir=" << dirtree_dir;

            /*
            // debug code

            const QStringList blah = m_sidebarModule->getInterfaces->componentData()->dirs()->dirs()->findDirs( "data", "konqueror/dirtree" );
            QStringList::ConstIterator eIt = blah.constBegin();
            QStringList::ConstIterator eEnd = blah.constEnd();
            for (; eIt != eEnd; ++eIt )
            kDebug(1201) << "findDirs got me " << *eIt;
            // end debug code
            */

            for (QStringList::const_iterator ddit=dirtree_dirs.constBegin();ddit!=dirtree_dirs.constEnd();++ddit) {
                QString dirtree_dir=*ddit;
                if (dirtree_dir==path) continue;
                //    if ( !dirtree_dir.isEmpty() && dirtree_dir != path )
                {
                    QDir globalDir( dirtree_dir );
                    Q_ASSERT( globalDir.isReadable() );
                    // Only copy the entries that don't exist yet in the local dir
                    const QStringList globalDirEntries = globalDir.entryList();
                    QStringList::ConstIterator eIt = globalDirEntries.constBegin();
                    QStringList::ConstIterator eEnd = globalDirEntries.constEnd();
                    for (; eIt != eEnd; ++eIt )
                    {
                        //kDebug(1201) << "dirtree_dir contains " << *eIt;
                        if ( *eIt != "." && *eIt != ".."
                             && !entries.contains( *eIt ) && !dirEntries.contains( *eIt ) )
                        { // we don't have that one yet -> copy it.
                            QString cp("cp -R -- ");
                            cp += KShell::quoteArg(dirtree_dir + *eIt);
                            cp += ' ';
                            cp += KShell::quoteArg(path);
                            kDebug(1201) << "executing " << cp;
                            ::system( QFile::encodeName(cp) );
                        }
                    }
                }
            }
            // hack to make QDir refresh the lists
            dir.setPath(path);
            entries = dir.entryList( QDir::Files );
            dirEntries = dir.entryList( QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot );
        }
    }


    // TODO: currently the filename order is used. Implement SortOrder? #69667

    QStringList::ConstIterator eIt = entries.constBegin();
    QStringList::ConstIterator eEnd = entries.constEnd();
    for (; eIt != eEnd; ++eIt ) {
        const QString filePath = path + *eIt;
        if (KDesktopFile::isDesktopFile(filePath)) {
            loadTopLevelItem(parent, filePath);
        }
    }

    eIt = dirEntries.constBegin();
    eEnd = dirEntries.constEnd();

    for (; eIt != eEnd; eIt++ )
    {
        const QString newPath = QString( path ).append( *eIt ).append( QLatin1Char( '/' ) );
        loadTopLevelGroup( parent, newPath );
    }
}

void KonqSidebarTree::loadTopLevelGroup( KonqSidebarTreeItem *parent, const QString &path )
{
    QDir dir( path );
    QString name = dir.dirName();
    QString icon = "folder";
    bool    open = false;

    kDebug(1201) << "Scanning " << path;

    QString dotDirectoryFile = QString( path ).append( "/.directory" );

    if ( QFile::exists( dotDirectoryFile ) )
    {
        kDebug(1201) << "Reading the .directory";
        KDesktopFile cfg(  dotDirectoryFile );
        const KConfigGroup group = cfg.desktopGroup();
        name = group.readEntry( "Name", name );
        icon = group.readEntry( "Icon", icon );
        //stripIcon( icon );
        open = group.readEntry( "Open", open);
    }

    KonqSidebarTreeTopLevelItem *item;
    if ( parent )
    {
        kDebug(1201) << "Inserting new group under parent ";
        item = new KonqSidebarTreeTopLevelItem( parent, 0 /* no module */, path );
    }
    else
        item = new KonqSidebarTreeTopLevelItem( this, 0 /* no module */, path );
    item->setText( 0, name );
    item->setPixmap( 0, SmallIcon( icon ) );
    item->setListable( false );
    item->setClickable( false );
    item->setTopLevelGroup( true );
    item->setOpen( open );

    m_topLevelItems.append( item );

    kDebug(1201) << "Inserting group " << name << "   " << path;

    scanDir( item, path );

    if ( item->childCount() == 0 )
        item->setExpandable( false );
}

void KonqSidebarTree::loadTopLevelItem(KonqSidebarTreeItem *parent, const QString &path)
{
    KDesktopFile cfg( path );
    KConfigGroup desktopGroup = cfg.desktopGroup();
    const QString name = cfg.readName();

    // Here's where we need to create the right module...
    // ### TODO: make this KTrader/KLibrary based.
    const QString moduleName = desktopGroup.readPathEntry( "X-KDE-TreeModule", QString("Directory") );
    const QString showHidden = desktopGroup.readEntry("X-KDE-TreeModule-ShowHidden");

    kDebug(1201) << "##### Loading module: " << moduleName << " file: " << path;

    KonqSidebarTreeModule * module = NULL;
    getModule func = getPluginFactory(moduleName);
    if (func) {
        kDebug(1201)<<"showHidden: "<<showHidden;
        module=func(this,showHidden.toUpper()=="TRUE");
    }

    if (!module) { kDebug() << "No Module loaded for" << moduleName; return; }

    KonqSidebarTreeTopLevelItem *item;
    if ( parent )
        item = new KonqSidebarTreeTopLevelItem( parent, module, path );
    else
        item = new KonqSidebarTreeTopLevelItem( this, module, path );

    item->setText( 0, name );
    item->setPixmap( 0, SmallIcon( cfg.readIcon() ));

    module->addTopLevelItem( item );

    m_topLevelItems.append( item );
    m_lstModules.append( module );

    bool open = desktopGroup.readEntry( "Open", false);
    if ( open && item->isExpandable() )
        item->setOpen( true );
}

void KonqSidebarTree::slotAnimation()
{
    MapCurrentOpeningFolders::Iterator it = m_mapCurrentOpeningFolders.begin();
    MapCurrentOpeningFolders::Iterator end = m_mapCurrentOpeningFolders.end();
    for (; it != end; ++it )
    {
        uint & iconNumber = it.value().iconNumber;
        QString icon = QString::fromLatin1( it.value().iconBaseName ).append( QString::number( iconNumber ) );
        it.key()->setPixmap( 0, SmallIcon( icon));

        iconNumber++;
        if ( iconNumber > it.value().iconCount )
            iconNumber = 1;
    }
}


void KonqSidebarTree::startAnimation( KonqSidebarTreeItem * item, const char * iconBaseName, uint iconCount, const QPixmap * originalPixmap )
{
    const QPixmap *pix = originalPixmap ? originalPixmap : item->pixmap(0);
    if (pix)
    {
        m_mapCurrentOpeningFolders.insert( item, AnimationInfo( iconBaseName, iconCount, *pix ) );
        if ( !m_animationTimer->isActive() )
            m_animationTimer->start( 50 );
    }
}

void KonqSidebarTree::stopAnimation( KonqSidebarTreeItem * item )
{
    MapCurrentOpeningFolders::Iterator it = m_mapCurrentOpeningFolders.find(item);
    if ( it != m_mapCurrentOpeningFolders.end() )
    {
        item->setPixmap( 0, it.value().originalPixmap );
        m_mapCurrentOpeningFolders.remove( item );

        if (m_mapCurrentOpeningFolders.isEmpty())
            m_animationTimer->stop();
    }
}

KonqSidebarTreeItem * KonqSidebarTree::currentItem() const
{
    return static_cast<KonqSidebarTreeItem *>( selectedItem() );
}

void KonqSidebarTree::setContentsPos( int x, int y )
{
    if ( !m_scrollingLocked )
        K3ListView::setContentsPos( x, y );
}

void KonqSidebarTree::slotItemRenamed(Q3ListViewItem* item, const QString &name, int col)
{
    Q_ASSERT(col==0);
    if (col != 0) return;
    Q_ASSERT(item);
    KonqSidebarTreeItem * treeItem = static_cast<KonqSidebarTreeItem *>(item);
    treeItem->rename( name );
}


void KonqSidebarTree::enableActions(bool copy, bool cut, bool paste)
{
    kDebug() << copy << cut << paste;
    m_sidebarModule->enableCopy(copy);
    m_sidebarModule->enableCut(cut);
    m_sidebarModule->enablePaste(paste);
}

void KonqSidebarTree::showToplevelContextMenu()
{
    KonqSidebarTreeTopLevelItem *item = 0;
    KonqSidebarTreeItem *treeItem = currentItem();
    if (treeItem && treeItem->isTopLevelItem())
        item = static_cast<KonqSidebarTreeTopLevelItem *>(treeItem);

    QMenu *menu = new QMenu;

    if (item) {
        if (item->isTopLevelGroup()) {
            menu->addAction( m_collection->action("rename") );
            menu->addAction( m_collection->action("delete") );
            menu->addSeparator();
            menu->addAction( m_collection->action("create_folder") );
        } else {
            menu->addAction( m_collection->action("open_tab") );
            menu->addAction( m_collection->action("open_window") );
            menu->addAction( m_collection->action("copy_location") );
            menu->addSeparator();
            menu->addAction( m_collection->action("rename") );
            menu->addAction( m_collection->action("delete_link") );
        }
        menu->addSeparator();
        menu->addAction( m_collection->action("item_properties") );
    } else {
        menu->addAction( m_collection->action("create_folder") );
    }

    m_currentTopLevelItem = item;

    menu->exec( QCursor::pos() );
    delete menu;

    m_currentTopLevelItem = 0;
}

void KonqSidebarTree::slotCreateFolder()
{
    QString path;
    QString name = i18n("New Folder");

    while(true)
    {
        name = KInputDialog::getText(i18nc("@title:window", "Create New Folder"),
                                     i18n("Enter folder name:"), name);
        if (name.isEmpty())
            return;

        if (m_currentTopLevelItem)
            path = m_currentTopLevelItem->path();
        else
            path = m_dirtreeDir.dir.path();

        if (!path.endsWith('/'))
            path += '/';

        path = path + name;

        if (!QFile::exists(path))
            break;

        name = name + "-2";
   }

   KGlobal::dirs()->makeDir(path);

   loadTopLevelGroup(m_currentTopLevelItem, path);
}

void KonqSidebarTree::slotDelete()
{
    if (currentItem())
        currentItem()->del();
}

void KonqSidebarTree::slotTrash()
{
    if (currentItem())
        currentItem()->trash();
}

void KonqSidebarTree::slotRename()
{
    if (currentItem())
        currentItem()->rename();
}

void KonqSidebarTree::slotProperties()
{
    if (!m_currentTopLevelItem) return;

    KUrl url(m_currentTopLevelItem->path());

    QPointer<KPropertiesDialog> dlg(new KPropertiesDialog(url, this));
    dlg->setFileNameReadOnly(true);
    dlg->exec();
    delete dlg;
}

void KonqSidebarTree::slotOpenNewWindow()
{
    if (!m_currentTopLevelItem) return;
    emit createNewWindow( m_currentTopLevelItem->externalURL() );
}

void KonqSidebarTree::slotOpenTab()
{
    if (!m_currentTopLevelItem) return;
    KParts::BrowserArguments browserArgs;
    browserArgs.setNewTab(true);
    emit createNewWindow(m_currentTopLevelItem->externalURL(),
                         KParts::OpenUrlArguments(),
                         browserArgs);
}

static QMimeData* mimeDataFor(const KUrl& url)
{
  QMimeData* data = new QMimeData();
  QList<QUrl> urlList;
  urlList.append(QUrl(url));
  data->setUrls(urlList);
  return data;
}

void KonqSidebarTree::slotCopyLocation()
{
    if (!m_currentTopLevelItem) return;
    KUrl url = m_currentTopLevelItem->externalURL();
    qApp->clipboard()->setMimeData( mimeDataFor(url), QClipboard::Selection );
    qApp->clipboard()->setMimeData( mimeDataFor(url), QClipboard::Clipboard );
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
/*
#ifdef __GNUC__
#warning KonqSidebarTreeToolTip removed, must implemented in event() function
#endif
void KonqSidebarTreeToolTip::maybeTip( const QPoint &point )
{
    Q3ListViewItem *item = m_view->itemAt( point );
    if ( item ) {
        QString text = static_cast<KonqSidebarTreeItem*>( item )->toolTipText();
        if ( !text.isEmpty() )
            tip ( m_view->itemRect( item ), text );
    }
}
*/

bool KonqSidebarTree::overrideShortcut(const QKeyEvent* e)
{
    const int key = e->key() | e->modifiers();
    if (key == Qt::Key_F2) {
        slotRename();
        return true;
    } else if (key == Qt::Key_Delete) {
        kDebug() << "delete key -> trash";
        slotTrash();
        return true;
    } else if (key == (Qt::SHIFT | Qt::Key_Delete)) {
        kDebug() << "shift+delete -> delete";
        slotDelete();
        return true;
    } else if (KStandardShortcut::copy().contains(key)) {
        kDebug() << "copy";
        emit copy();
        return true;
    } else if (KStandardShortcut::cut().contains(key)) {
        kDebug() << "cut";
        emit cut();
        return true;
    } else if (KStandardShortcut::paste().contains(key)) {
        kDebug() << "paste";
        emit paste();
        return true;
    }
    return false;
}

// For F2 and other shortcuts to work we can't use a KAction; it would conflict with the
// KAction from the active dolphinpart. So we have to use ShortcutOverride.
// Many users requested keyboard shortcuts to work in the sidebar, so it's worth the ugliness (#80584)
bool KonqSidebarTree::eventFilter(QObject* obj, QEvent* ev)
{
    if (ev->type() == QEvent::ShortcutOverride) {
        QKeyEvent *e = static_cast<QKeyEvent *>( ev );
        if (overrideShortcut(e)) {
            e->accept();
            return true;
        }
    }
    return K3ListView::eventFilter(obj, ev);
}

#include "konq_sidebartree.moc"
