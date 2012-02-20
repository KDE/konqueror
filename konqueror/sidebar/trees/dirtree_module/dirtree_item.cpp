/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2003 Waldo Bastian <bastian@kde.org>

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

#include "dirtree_item.h"

#include "dirtree_module.h"
#include "konqsidebar_oldtreemodule.h"

#include <kfileitemlistproperties.h>
#include <kactioncollection.h>
#include <konq_operations.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kmimetypetrader.h>
#include <kio/paste.h>
#include <kconfiggroup.h>
#include <kiconloader.h>
#include <konqmimedata.h>
#include <kde_file.h>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtGui/QPainter>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QCursor>

#define MYMODULE static_cast<KonqSidebarDirTreeModule*>(module())

KonqSidebarDirTreeItem::KonqSidebarDirTreeItem( KonqSidebarTreeItem *parentItem, KonqSidebarTreeTopLevelItem *topLevelItem, const KFileItem &fileItem )
    : KonqSidebarTreeItem( parentItem, topLevelItem ), m_fileItem( fileItem )
{
    if ( m_topLevelItem )
        MYMODULE->addSubDir( this );
    reset();
}

KonqSidebarDirTreeItem::KonqSidebarDirTreeItem( KonqSidebarTree *parent, KonqSidebarTreeTopLevelItem *topLevelItem, const KFileItem &fileItem )
    : KonqSidebarTreeItem( parent, topLevelItem ), m_fileItem( fileItem )
{
    if ( m_topLevelItem )
        MYMODULE->addSubDir( this );
    reset();
}

KonqSidebarDirTreeItem::~KonqSidebarDirTreeItem()
{
}

void KonqSidebarDirTreeItem::reset()
{
    bool expandable = true;
    // For local dirs, find out if they have no children, to remove the "+"
    if ( m_fileItem.isDir() )
    {
        KUrl url = m_fileItem.url();
        if ( url.isLocalFile() )
        {
            struct stat buff;
            if ( KDE::stat( url.toLocalFile(), &buff ) != -1 )
            {
                //kDebug() << "KonqSidebarDirTreeItem::init " << path << " : " << buff.st_nlink;
                // The link count for a directory is generally subdir_count + 2.
                // One exception is if there are hard links to the directory, in this case
                // the link count can be > 2 even if no subdirs exist.
                // The other exception are smb (and maybe netware) mounted directories
                // of which the link count is always 1. Therefore, we only set the item
                // as non-expandable if it's exactly 2 (one link from the parent dir,
                // plus one from the '.' entry).
                if ( buff.st_nlink == 2 )
                    expandable = false;
            }
        }
    }
    setExpandable( expandable );
    id = m_fileItem.url().url( KUrl::RemoveTrailingSlash );
}

void KonqSidebarDirTreeItem::setOpen( bool open )
{
    kDebug(1201) << "KonqSidebarDirTreeItem::setOpen " << open;
    if ( open && !childCount() && m_bListable )
        MYMODULE->openSubFolder( this );
    else if ( hasStandardIcon() )
    {
        int size = KIconLoader::global()->currentSize( KIconLoader::Small );
        if ( open )
            setPixmap( 0, DesktopIcon( "folder-open", size ) );
        else
            setPixmap( 0, m_fileItem.pixmap( size ) );
    }
    KonqSidebarTreeItem::setOpen( open );
}

bool KonqSidebarDirTreeItem::hasStandardIcon()
{
    // The reason why we can't use KFileItem::iconName() is that it doesn't
    // take custom icons in .directory files into account
    return m_fileItem.determineMimeType()->iconName( m_fileItem.url()/*, m_fileItem->isLocalFile()*/ ) == "folder";
}

void KonqSidebarDirTreeItem::paintCell( QPainter *_painter, const QColorGroup & _cg, int _column, int _width, int _alignment )
{
    if (m_fileItem.isLink())
    {
        QFont f( _painter->font() );
        f.setItalic( true );
        _painter->setFont( f );
    }
    Q3ListViewItem::paintCell( _painter, _cg, _column, _width, _alignment );
}

KUrl KonqSidebarDirTreeItem::externalURL() const
{
    return m_fileItem.url();
}

QString KonqSidebarDirTreeItem::externalMimeType() const
{
    if (m_fileItem.isMimeTypeKnown())
        return m_fileItem.mimetype();
    else
        return QString();
}

bool KonqSidebarDirTreeItem::acceptsDrops( const Q3StrList & formats )
{
    if ( formats.contains("text/uri-list") ) {
        // A directory ?
        if ( S_ISDIR( m_fileItem.mode() ) ) {
            return m_fileItem.isWritable();
        }

        // But only local .desktop files and executables
        if ( !m_fileItem.isLocalFile() )
            return false;

        if ( m_fileItem.mimetype() == "application/x-desktop")
            return true;

        // Executable, shell script ... ?
        if ( QFileInfo(m_fileItem.url().toLocalFile()).isExecutable() )
            return true;

        return false;
    }
    return false;
}

void KonqSidebarDirTreeItem::drop( QDropEvent * ev )
{
    KonqOperations::doDrop( m_fileItem, externalURL(), ev, tree() );
}

bool KonqSidebarDirTreeItem::populateMimeData( QMimeData* mimeData, bool move )
{
    KUrl::List lst;
    lst.append( m_fileItem.url() );
    kDebug() << lst;

    KonqMimeData::populateMimeData( mimeData, lst, KUrl::List(), move );
    return true;
}

void KonqSidebarDirTreeItem::itemSelected()
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    const KUrl::List urls = KUrl::List::fromMimeData(mimeData);
    const bool paste = !urls.isEmpty();
    tree()->enableActions(true, true, paste);
}

void KonqSidebarDirTreeItem::middleButtonClicked()
{
    // Optimization to avoid KRun to call kfmclient that then tells us
    // to open a window :-)
    KService::Ptr offer = KMimeTypeTrader::self()->preferredService(m_fileItem.mimetype(), "Application");
    if (offer) kDebug(1201) << "KonqDirPart::mmbClicked: got service " << offer->desktopEntryName();
    if ( offer && offer->desktopEntryName().startsWith("kfmclient") )
    {
        kDebug(1201)<<"Emitting createNewWindow";
        KParts::OpenUrlArguments args;
        args.setMimeType( m_fileItem.mimetype() );
        emit tree()->createNewWindow( m_fileItem.url(), args );
    }
    else
        m_fileItem.run();
}

void KonqSidebarDirTreeItem::rightButtonPressed()
{
    KParts::BrowserExtension::PopupFlags popupFlags = KParts::BrowserExtension::DefaultPopupItems
                                                      | KParts::BrowserExtension::ShowProperties
                                                      | KParts::BrowserExtension::ShowUrlOperations;
    KParts::BrowserExtension::ActionGroupMap actionGroups;
    QList<QAction *> editActions;
    KActionCollection* actionCollection = tree()->actionCollection();

    KFileItemList items;
    items.append(m_fileItem);
    KFileItemListProperties capabilities(items);

    // ###### most of this is duplicated from DolphinPart::slotOpenContextMenu :(

    bool supportsDeleting = capabilities.supportsDeleting();
    bool supportsMoving = capabilities.supportsMoving();
    if (!supportsDeleting) {
        popupFlags |= KParts::BrowserExtension::NoDeletion;
    }

    Q_ASSERT(actionCollection->action("rename"));
    Q_ASSERT(actionCollection->action("trash"));
    Q_ASSERT(actionCollection->action("delete"));

    if (supportsMoving) {
        editActions.append(actionCollection->action("rename"));
    }

    bool addTrash = capabilities.isLocal() && supportsMoving;
    bool addDel = false;

    if (supportsDeleting) {
        if (!m_fileItem.isLocalFile())
            addDel = true;
        else if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
            addTrash = false;
            addDel = true;
        }
        else {
            KSharedConfig::Ptr globalConfig = KSharedConfig::openConfig("kdeglobals", KConfig::IncludeGlobals);
            KConfigGroup configGroup(globalConfig, "KDE");
            addDel = configGroup.readEntry("ShowDeleteCommand", false);
        }
    }

    if (addTrash)
        editActions.append(actionCollection->action("trash"));
    if (addDel)
        editActions.append(actionCollection->action("delete"));
    // Normally KonqPopupMenu only shows the "Create new" submenu in the current view
    // since otherwise the created file would not be visible.
    // But in treeview mode we should allow it.
    popupFlags |= KParts::BrowserExtension::ShowCreateDirectory;

    actionGroups.insert("editactions", editActions);

    emit tree()->sidebarModule()->showPopupMenu(QCursor::pos(), items,
                           KParts::OpenUrlArguments(), KParts::BrowserArguments(),
                           popupFlags, actionGroups);
}

void KonqSidebarDirTreeItem::paste()
{
    // move or not move ?
    bool move = false;
    const QMimeData *data = QApplication::clipboard()->mimeData();
    if ( data->hasFormat( "application/x-kde-cutselection" ) ) {
        move = KonqMimeData::decodeIsCutSelection( data );
        kDebug(1201) << "move (from clipboard data) = " << move;
    }

    KIO::pasteClipboard( m_fileItem.url(), listView(), move );
}

void KonqSidebarDirTreeItem::trash()
{
    delOperation( KonqOperations::TRASH );
}

void KonqSidebarDirTreeItem::del()
{
    delOperation( KonqOperations::DEL );
}

void KonqSidebarDirTreeItem::delOperation( KonqOperations::Operation method )
{
    KUrl::List lst;
    lst.append(m_fileItem.url());

    KonqOperations::del(tree(), method, lst);
}

QString KonqSidebarDirTreeItem::toolTipText() const
{
    return m_fileItem.url().pathOrUrl();
}

void KonqSidebarDirTreeItem::rename()
{
    tree()->rename( this, 0 );
}

void KonqSidebarDirTreeItem::rename( const QString & name )
{
    KonqOperations::rename( tree(), m_fileItem.url(), name );
}
