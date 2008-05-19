/* This file is part of the KDE project
   Copyright (C) 1998-2006 David Faure <faure@kde.org>
   Copyright (C) 2001 Holger Freyther <freyther@yahoo.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_popupmenu.h"
#include "konq_popupmenuinformation.h"
#include "konq_copytomenu.h"
#include "konq_menuactions.h"
#include "kpropertiesdialog.h"
#include "knewmenu.h"
#include "konq_operations.h"

#include <klocale.h>
#include <kbookmarkmanager.h>
#include <kbookmarkdialog.h>
#include <kdebug.h>
#include <krun.h>
#include <kprotocolmanager.h>
#include <kicon.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <kglobalsettings.h>
#include <kmimetypetrader.h>
#include <kstandarddirs.h>
#include <kconfiggroup.h>
#include <kdesktopfile.h>
#include <kfileshare.h>
#include <kauthorized.h>
#include <kglobal.h>

#include <QDir>
#include <QApplication>
#include <QPixmap>

/*
 Test cases:
  iconview file: background
  iconview file: file (with and without servicemenus)
  iconview file: directory
  iconview remote protocol (e.g. ftp: or fish:)
  iconview trash:/
  sidebar directory tree
  sidebar Devices / Hard Disc
  khtml background
  khtml link
  khtml image (www.kde.org RMB on K logo)
  khtmlimage (same as above, then choose View image, then RMB)
  selected text in khtml
  embedded katepart
  folder on the desktop
  trash link on the desktop
  trashed file or directory
  application .desktop file
 Then the same after uninstalling kdeaddons/konq-plugins (kuick and arkplugin in particular)
*/

class KonqPopupMenuPrivate
{
public:
    KonqPopupMenuPrivate(KonqPopupMenu* qq, KActionCollection & actions)
        : q(qq),
          m_parentWidget(0),
          m_itemFlags(KParts::BrowserExtension::DefaultPopupItems),
          m_actions(actions),
          m_ownActions(static_cast<QWidget *>(0))
    {
    }
    void addNamedAction(const QString& name);
    void addGroup(const QString& name);
    void addPlugins();
    void setup(KonqPopupMenu::Flags kpf);

    void slotPopupNewDir();
    void slotPopupNewView();
    void slotPopupEmptyTrashBin();
    void slotPopupRestoreTrashedItems();
    void slotPopupAddToBookmark();
    void slotPopupMimeType();
    void slotPopupProperties();
    void slotOpenShareFileDialog();

    KonqPopupMenu* q;
    QString m_urlTitle;
    QWidget *m_parentWidget;
    KParts::BrowserExtension::PopupFlags m_itemFlags;
    KNewMenu *m_pMenuNew;
    KUrl m_sViewURL;
    QString m_sMimeType;
    KFileItemList m_lstItems;
    KUrl::List m_lstPopupURLs;
    KonqMenuActions m_menuActions;
    KonqCopyToMenu m_copyToMenu;
    bool m_bHandleEditOperations;
    QString m_attrName;
//    KonqPopupMenu::ProtocolInfo m_info;
    KBookmarkManager* m_bookmarkManager;
    KActionCollection &m_actions;
    KActionCollection m_ownActions; // TODO connect to statusbar for help on actions
    KParts::BrowserExtension::ActionGroupMap m_actionGroups;
};

#if 0
KonqPopupMenu::ProtocolInfo::ProtocolInfo()
{
  m_Reading = false;
  m_Writing = false;
  m_Deleting = false;
  m_Moving = false;
  m_TrashIncluded = false;
}

bool KonqPopupMenu::ProtocolInfo::supportsReading() const
{
  return m_Reading;
}

bool KonqPopupMenu::ProtocolInfo::supportsWriting() const
{
  return m_Writing;
}

bool KonqPopupMenu::ProtocolInfo::supportsDeleting() const
{
  return m_Deleting;
}

bool KonqPopupMenu::ProtocolInfo::supportsMoving() const
{
  return m_Moving;
}

bool KonqPopupMenu::ProtocolInfo::trashIncluded() const
{
  return m_TrashIncluded;
}
#endif

//////////////////

KonqPopupMenu::KonqPopupMenu(const KFileItemList &items,
                             const KUrl& viewURL,
                             KActionCollection & actions,
                             KNewMenu * newMenu,
                             Flags kpf,
                             KParts::BrowserExtension::PopupFlags flags,
                             QWidget * parentWidget,
                             KBookmarkManager *mgr,
                             const KParts::BrowserExtension::ActionGroupMap& actionGroups)
  : QMenu( parentWidget ),
    d(new KonqPopupMenuPrivate(this, actions))
{
    d->m_actionGroups = actionGroups;
    d->m_pMenuNew = newMenu;
    d->m_sViewURL = viewURL;
    d->m_lstItems = items;
    d->m_bookmarkManager = mgr;
    init(parentWidget, kpf, flags);
}

void KonqPopupMenu::init (QWidget * parentWidget, Flags kpf, KParts::BrowserExtension::PopupFlags flags)
{
    d->m_ownActions.setObjectName("KonqPopupMenu::m_ownActions");
    d->m_parentWidget = parentWidget;
    d->m_itemFlags = flags;
    setFont(KGlobalSettings::menuFont());
    d->setup(kpf);
}

void KonqPopupMenuPrivate::addNamedAction(const QString& name)
{
    QAction* act = m_actions.action(name);
    if (act)
        q->addAction(act);
}

void KonqPopupMenuPrivate::setup(KonqPopupMenu::Flags kpf)
{
    Q_ASSERT( m_lstItems.count() >= 1 );

    const bool bIsLink  = (m_itemFlags & KParts::BrowserExtension::IsLink);
    bool currentDir     = false;
    bool sReading       = true;
    bool sDeleting      = (m_itemFlags & KParts::BrowserExtension::NoDeletion) == 0;
    bool sMoving        = sDeleting;
    bool sWriting       = sDeleting && m_lstItems.first().isWritable();
    bool bTrashIncluded = false;
    bool isLocal        = m_lstItems.first().isLocalFile()
                       || m_lstItems.first().url().protocol()=="media"
                       || m_lstItems.first().url().protocol()=="system";
    bool isTrashLink     = false;
    m_lstPopupURLs.clear();

    m_attrName = QLatin1String( "name" );

    QFileInfo parentDirInfo;

    KUrl url;
    KFileItemList::const_iterator it = m_lstItems.begin();
    const KFileItemList::const_iterator kend = m_lstItems.end();
    for ( ; it != kend; ++it )
    {
        url = (*it).url();

        if ( isLocal && !url.isLocalFile())
            isLocal = false;

        if ( !bTrashIncluded && (
             ( url.protocol() == "trash" && url.path().length() <= 1 ) ) ) {
            bTrashIncluded = true;
            isLocal = false;
        }

        // TODO use FileItemCapabilities from Dolphin (in KonqPopupMenuInformation)
        if ( sReading )
            sReading = KProtocolManager::supportsReading( url );

        if ( sWriting )
            sWriting = KProtocolManager::supportsWriting( url ) && (*it).isWritable();

        if ( sDeleting )
            sDeleting = KProtocolManager::supportsDeleting( url );

        if ( sMoving )
            sMoving = KProtocolManager::supportsMoving( url );

        // For local files we can do better: check if we have write permission in parent directory
        if (url.isLocalFile() && (sDeleting || sMoving)) {
            const QString directory = url.directory();
            if (parentDirInfo.filePath() != directory) {
                parentDirInfo.setFile(directory);
            }
            if (!parentDirInfo.isWritable()) {
                // kDebug(1203) << "not writable:" << directory;
                sDeleting = false;
                sMoving = false;
            }
        }
    }

    KonqPopupMenuInformation popupInfo;
    popupInfo.setItems(m_lstItems);
    popupInfo.setReadOnly(sWriting == false);
    popupInfo.setParentWidget(m_parentWidget);

    m_lstPopupURLs = popupInfo.urlList();
    m_sMimeType    = popupInfo.mimeType();
    const bool isDirectory = popupInfo.isDirectory();

    url = m_sViewURL;
    url.cleanPath();

    //check if url is current directory
    if ( m_lstItems.count() == 1 )
    {
        KUrl firstPopupURL( m_lstItems.first().url() );
        firstPopupURL.cleanPath();
        //kDebug(1203) << "View path is " << url.url();
        //kDebug(1203) << "First popup path is " << firstPopupURL.url();
        currentDir = firstPopupURL.equals( url, KUrl::CompareWithoutTrailingSlash );
        if ( isLocal && m_sMimeType == "application/x-desktop" ) {
            KDesktopFile desktopFile( firstPopupURL.path() );
            const KConfigGroup cfg = desktopFile.desktopGroup();
            isTrashLink = ( cfg.readEntry("Type") == "Link" && cfg.readEntry("URL") == "trash:/" );
        }

        if ( isTrashLink ) {
            sDeleting = false;
        }
    }

#if 0
    m_info.m_Reading = sReading;
    m_info.m_Writing = sWriting;
    m_info.m_Deleting = sDeleting;
    m_info.m_Moving = sMoving;
    m_info.m_TrashIncluded = bTrashIncluded;
#endif

    // isCurrentTrash: popup on trash:/ itself, or on the trash.desktop link
    bool isCurrentTrash = ( m_lstItems.count() == 1 && bTrashIncluded ) || isTrashLink;
    bool isIntoTrash = ( url.protocol() == "trash" || url.url().startsWith( "system:/trash" ) ) && !isCurrentTrash; // trashed file, not trash:/ itself
    //kDebug() << "isLocal=" << isLocal << " url=" << url << " isCurrentTrash=" << isCurrentTrash << " isIntoTrash=" << isIntoTrash << " bTrashIncluded=" << bTrashIncluded;

    const bool isSingleMedium = false; // was: m_lstItems.count() == 1 && mediaFiles, not available anymore

    //////////////////////////////////////////////////////////////////////////

    addGroup( "topactions" ); // used e.g. for ShowMenuBar. includes a separator at the end

    QAction * act;

    bool isOnDesktop = false; // ### remove, or re-introduced if needed for desktop icons
    QAction *actNewWindow = 0;

#if 0
    if (( flags & KParts::BrowserExtension::ShowProperties ) && isOnDesktop &&
        !KAuthorized::authorizeKAction("editable_desktop_icons"))
    {
        flags &= ~KParts::BrowserExtension::ShowProperties; // remove flag
    }
#endif

    // Either 'newview' is in the actions we're given (probably in the tabhandling group)
    // or we need to insert it ourselves (e.g. for kdesktop). In the first case, actNewWindow must remain 0.
    if ( ((kpf & KonqPopupMenu::ShowNewWindow) != 0) && sReading )
    {
        QString openStr = isOnDesktop ? i18n( "&Open" ) : i18n( "Open in New &Window" );
        actNewWindow = m_ownActions.addAction( "newview" );
        actNewWindow->setIcon( KIcon("window-new") );
        actNewWindow->setText( openStr );
        QObject::connect(actNewWindow, SIGNAL(triggered()), q, SLOT(slotPopupNewView()));
    }

    if ( actNewWindow && !isOnDesktop )
    {
        if (isCurrentTrash)
            actNewWindow->setToolTip( i18n( "Open the trash in a new window" ) );
        else if (isSingleMedium)
            actNewWindow->setToolTip( i18n( "Open the medium in a new window") );
        else
            actNewWindow->setToolTip( i18n( "Open the document in a new window" ) );
    }

    if ( isDirectory && sWriting && !isCurrentTrash ) // A dir, and we can create things into it
    {
        if ( currentDir && m_pMenuNew ) // Current dir -> add the "new" menu
        {
            // As requested by KNewMenu :
            m_pMenuNew->slotCheckUpToDate();
            m_pMenuNew->setPopupFiles( m_lstPopupURLs );

            q->addAction( m_pMenuNew );
            q->addSeparator();
        }
        else
        {
            if (m_itemFlags & KParts::BrowserExtension::ShowCreateDirectory)
            {
                QAction *actNewDir = m_ownActions.addAction( "newdir" );
                actNewDir->setIcon( KIcon("folder-new") );
                actNewDir->setText( i18n( "Create &Folder..." ) );
                QObject::connect(actNewDir, SIGNAL(triggered()), q, SLOT(slotPopupNewDir()));
                q->addAction( actNewDir );
                q->addSeparator();
            }
        }
    } else if ( isIntoTrash ) {
        // Trashed item, offer restoring
        act = m_ownActions.addAction( "restore" );
        act->setText( i18n( "&Restore" ) );
        QObject::connect(act, SIGNAL(triggered()), q, SLOT(slotPopupRestoreTrashedItems()));
        q->addAction(act);
    }

    if (m_itemFlags & KParts::BrowserExtension::ShowNavigationItems)
    {
        if (m_itemFlags & KParts::BrowserExtension::ShowUp)
            addNamedAction( "go_up" );
        addNamedAction( "go_back" );
        addNamedAction( "go_forward" );
        if (m_itemFlags & KParts::BrowserExtension::ShowReload)
            addNamedAction( "reload" );
        q->addSeparator();
    }

    // "open in new window" is either provided by us, or by the tabhandling group
    if (actNewWindow) {
        q->addAction(actNewWindow);
        q->addSeparator();
    }
    addGroup( "tabhandling" ); // includes a separator at the end

    if (m_itemFlags & KParts::BrowserExtension::ShowUrlOperations) {
        if ( !currentDir && sReading ) {
            if ( sDeleting ) {
                addNamedAction( "cut" );
            }
            addNamedAction( "copy" );
        }

        if ( isDirectory && sWriting ) {
            if ( currentDir )
                addNamedAction( "paste" );
            else
                addNamedAction( "pasteto" );
        }
    }
    if ( isCurrentTrash )
    {
        act = m_ownActions.addAction( "emptytrash" );
        act->setIcon( KIcon("trash-empty") );
        act->setText( i18n( "&Empty Trash Bin" ) );
        KConfig trashConfig( "trashrc", KConfig::SimpleConfig);
        act->setEnabled( !trashConfig.group("Status").readEntry( "Empty", true ) );
        QObject::connect(act, SIGNAL(triggered()), q, SLOT(slotPopupEmptyTrashBin()));
        q->addAction(act);
    }

    // This is used by KHTML, see khtml_popupmenu.rc (copy, selectAll, searchProvider etc.)
    // and by DolphinPart (rename, trash, delete)
    addGroup( "editactions" );

    if (m_itemFlags & KParts::BrowserExtension::ShowTextSelectionItems) {
        // OK, we have to stop here.

        // Anything else that is provided by the part
        addGroup( "partactions" );
        return;
    }

    if ( !isCurrentTrash && !isIntoTrash && (m_itemFlags & KParts::BrowserExtension::ShowBookmark))
    {
        QString caption;
        if (currentDir)
        {
           const bool httpPage = m_sViewURL.protocol().startsWith("http", Qt::CaseInsensitive);
           if (httpPage)
              caption = i18n("&Bookmark This Page");
           else
              caption = i18n("&Bookmark This Location");
        }
        else if (isDirectory)
           caption = i18n("&Bookmark This Folder");
        else if (bIsLink)
           caption = i18n("&Bookmark This Link");
        else
           caption = i18n("&Bookmark This File");

        act = m_ownActions.addAction( "bookmark_add" );
        act->setIcon( KIcon("bookmark-new") );
        act->setText( caption );
        QObject::connect(act, SIGNAL(triggered()), q, SLOT(slotPopupAddToBookmark()));
        if (m_lstItems.count() > 1)
            act->setEnabled(false);
        if (KAuthorized::authorizeKAction("bookmarks"))
            q->addAction( act );
        if (bIsLink)
            addGroup( "linkactions" ); // see khtml
    }

    // "Open With" actions

    m_menuActions.setPopupMenuInfo(popupInfo);

    if ( sReading ) {
        m_menuActions.addOpenWithActionsTo(q, "DesktopEntryName != 'kfmclient' and DesktopEntryName != 'kfmclient_dir' and DesktopEntryName != 'kfmclient_html'");

        QList<QAction *> previewActions = m_actionGroups.value("preview");
        if (!previewActions.isEmpty()) {
            if (previewActions.count() == 1) {
                q->addAction(previewActions.first());
            } else {
                QMenu* subMenu = new QMenu(i18n("Preview In"), q);
                subMenu->menuAction()->setObjectName("preview_submenu"); // for the unittest
                q->addMenu(subMenu);
                subMenu->addActions(previewActions);
            }
        }
    }

    // Second block, builtin + user
    m_menuActions.addActionsTo(q);

    q->addSeparator();

    // CopyTo/MoveTo menus
    if (m_itemFlags & KParts::BrowserExtension::ShowUrlOperations) {
        m_copyToMenu.setItems(m_lstItems);
        m_copyToMenu.setReadOnly(sMoving == false);
        m_copyToMenu.addActionsTo(q);
        q->addSeparator();
    }

    if ( !isCurrentTrash && !isIntoTrash && sReading )
        addPlugins(); // now it's time to add plugins

    if ( (m_itemFlags & KParts::BrowserExtension::ShowProperties) && KPropertiesDialog::canDisplay( m_lstItems ) ) {
        act = m_ownActions.addAction( "properties" );
        act->setText( i18n( "&Properties" ) );
        QObject::connect(act, SIGNAL(triggered()), q, SLOT(slotPopupProperties()));
        q->addAction(act);
    }

    while ( !q->actions().isEmpty() &&
            q->actions().last()->isSeparator() )
        delete q->actions().last();

    if ( isDirectory && isLocal ) {
        if ( KFileShare::authorization() == KFileShare::Authorized ) {
            q->addSeparator();
            act = m_ownActions.addAction( "sharefile" );
            act->setText( i18n("Share") );
            QObject::connect(act, SIGNAL(triggered()), q, SLOT(slotOpenShareFileDialog()));
            q->addAction(act);
        }
    }

    // Anything else that is provided by the part
    addGroup( "partactions" );
}

void KonqPopupMenuPrivate::slotOpenShareFileDialog()
{
    KPropertiesDialog* dlg = new KPropertiesDialog( m_lstItems, m_parentWidget );
    dlg->showFileSharingPage();
    dlg->exec();
}

KonqPopupMenu::~KonqPopupMenu()
{
  delete d;
  //kDebug(1203) << "~KonqPopupMenu leave";
}

void KonqPopupMenu::setURLTitle( const QString& urlTitle )
{
    d->m_urlTitle = urlTitle;
}

void KonqPopupMenuPrivate::slotPopupNewView()
{
  KUrl::List::ConstIterator it = m_lstPopupURLs.begin();
  for ( ; it != m_lstPopupURLs.end(); it++ )
    (void) new KRun(*it, m_parentWidget);
}

void KonqPopupMenuPrivate::slotPopupNewDir()
{
  if (m_lstPopupURLs.empty())
    return;

  KonqOperations::newDir(m_parentWidget, m_lstPopupURLs.first());
}

void KonqPopupMenuPrivate::slotPopupEmptyTrashBin()
{
  KonqOperations::emptyTrash( m_parentWidget );
}

void KonqPopupMenuPrivate::slotPopupRestoreTrashedItems()
{
  KonqOperations::restoreTrashedItems( m_lstPopupURLs, m_parentWidget );
}

void KonqPopupMenuPrivate::slotPopupAddToBookmark()
{
  KBookmarkGroup root;
  if ( m_lstPopupURLs.count() == 1 ) {
    KUrl url = m_lstPopupURLs.first();
    QString title = m_urlTitle.isEmpty() ? url.prettyUrl() : m_urlTitle;
    KBookmarkDialog dlg(m_bookmarkManager, m_parentWidget);
    dlg.addBookmark(title, url.url());
  }
  else
  {
    root = m_bookmarkManager->root();
    KUrl::List::ConstIterator it = m_lstPopupURLs.begin();
    for ( ; it != m_lstPopupURLs.end(); it++ )
      root.addBookmark( (*it).prettyUrl(), (*it) );
    m_bookmarkManager->emitChanged( root );
  }
}

void KonqPopupMenuPrivate::slotPopupMimeType()
{
    KonqOperations::editMimeType( m_sMimeType, m_parentWidget );
}

void KonqPopupMenuPrivate::slotPopupProperties()
{
    KPropertiesDialog::showDialog( m_lstItems, m_parentWidget );
}

QString KonqPopupMenu::mimeType() const
{
    return d->m_sMimeType;
}

#if 0
KonqPopupMenu::ProtocolInfo KonqPopupMenu::protocolInfo() const
{
    return d->m_info;
}
#endif

void KonqPopupMenuPrivate::addGroup(const QString& name)
{
    QList<QAction *> actions = m_actionGroups.value(name);
    q->addActions(actions);
}

void KonqPopupMenuPrivate::addPlugins()
{
#if 0
    //search for a plugin with the right protocol
    KService::List plugin_offers;
    unsigned int pluginCount = 0;
    plugin_offers = KMimeTypeTrader::self()->query( m_sMimeType.isNull() ? QLatin1String( "all/all" ) : m_sMimeType, "KonqPopupMenu/Plugin" );
    if ( plugin_offers.isEmpty() )
        return; // no plugins installed do not bother about it

    KService::List::ConstIterator iterator = plugin_offers.begin();
    const KService::List::ConstIterator end = plugin_offers.end();

    //addGroup( "plugins" );
    // travers the offerlist
    for(; iterator != end; ++iterator, ++pluginCount ) {
        //kDebug() << (*iterator)->library();
        KonqPopupMenuPlugin *plugin =
            KLibLoader::createInstance<KonqPopupMenuPlugin>( QFile::encodeName( (*iterator)->library() ),
                                                            q );
        if ( !plugin )
            continue;
        plugin->setObjectName( (*iterator)->name() );
        // This made the kuick plugin insert its items at the right place
        // ### TODO replace with new mechanism (e.g. addAction(QAction *) in the plugin code),
        // if plugins are kept
        QString pluginClientName = QString::fromLatin1( "Plugin%1" ).arg( pluginCount );
        addMerge( pluginClientName );
        plugin->domDocument().documentElement().setAttribute( "name", pluginClientName );
        insertChildClient( plugin );
    }
#endif
}

KUrl KonqPopupMenu::url() const // ### should be viewURL()
{
  return d->m_sViewURL;
}

KFileItemList KonqPopupMenu::fileItemList() const
{
  return d->m_lstItems;
}

KUrl::List KonqPopupMenu::popupURLList() const
{
  return d->m_lstPopupURLs;
}

#if 0
/**
        Plugin
*/

KonqPopupMenuPlugin::KonqPopupMenuPlugin( KonqPopupMenu *parent )
    : QObject( parent )
{
}

KonqPopupMenuPlugin::~KonqPopupMenuPlugin()
{
}
#endif

#include "konq_popupmenu.moc"
