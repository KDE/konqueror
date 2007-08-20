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
#include "kpropertiesdialog.h"
#include "knewmenu.h"
#include "konq_operations.h"

#include <klocale.h>
#include <kapplication.h>
#include <kbookmarkmanager.h>
#include <kdebug.h>
#include <krun.h>
#include <kprotocolmanager.h>
#include <kicon.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <kglobalsettings.h>
#include <kstandarddirs.h>
#include <kxmlguifactory.h>
#include <kxmlguibuilder.h>
#include <kconfiggroup.h>
#include <kdesktopfile.h>
#include <kparts/componentfactory.h>
#include <kfileshare.h>
#include <kauthorized.h>
#include <kglobal.h>

#include <QtDBus/QtDBus>
#include <QDir>
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
  kdesktop folder
  trash link on desktop
  trashed file or directory
  application .desktop file
 Then the same after uninstalling kdeaddons/konq-plugins (kuick and arkplugin in particular)
*/

class KonqPopupMenuGUIBuilder : public KXMLGUIBuilder
{
public:
  KonqPopupMenuGUIBuilder( QMenu *menu )
  : KXMLGUIBuilder( 0 )
  {
    m_menu = menu;
  }
  virtual ~KonqPopupMenuGUIBuilder()
  {
  }

  virtual QWidget *createContainer( QWidget *parent, int index,
          const QDomElement &element,
          int &id )
  {
    if ( !parent && element.attribute( "name" ) == "popupmenu" )
      return m_menu;

    return KXMLGUIBuilder::createContainer( parent, index, element, id );
  }

  QMenu *m_menu;
};

class KonqPopupMenu::KonqPopupMenuPrivate
{
public:
  KonqPopupMenuPrivate() : m_parentWidget( 0 ),
                           m_itemFlags( KParts::BrowserExtension::DefaultPopupItems )
  {
  }
  QString m_urlTitle;
  QWidget *m_parentWidget;
  KParts::BrowserExtension::PopupFlags m_itemFlags;
};

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

// This helper class stores the .desktop-file actions and the servicemenus
// in order to support X-KDE-Priority and X-KDE-Submenu.
class PopupServices
{
public:
    ServiceList* selectList( const QString& priority, const QString& submenuName );

    ServiceList builtin;
    ServiceList user, userToplevel, userPriority;
    QMap<QString, ServiceList> userSubmenus, userToplevelSubmenus, userPrioritySubmenus;
};

ServiceList* PopupServices::selectList( const QString& priority, const QString& submenuName )
{
    // we use the categories .desktop entry to define submenus
    // if none is defined, we just pop it in the main menu
    if (submenuName.isEmpty())
    {
        if (priority == "TopLevel")
        {
            return &userToplevel;
        }
        else if (priority == "Important")
        {
            return &userPriority;
        }
    }
    else if (priority == "TopLevel")
    {
        return &(userToplevelSubmenus[submenuName]);
    }
    else if (priority == "Important")
    {
        return &(userPrioritySubmenus[submenuName]);
    }
    else
    {
        return &(userSubmenus[submenuName]);
    }
    return &user;
}

//////////////////

KonqPopupMenu::KonqPopupMenu( KBookmarkManager *mgr, const KFileItemList &items,
                              const KUrl& viewURL,
                              KActionCollection & actions,
                              KNewMenu * newMenu,
                              QWidget * parentWidget,
                              KonqPopupFlags kpf,
                              KParts::BrowserExtension::PopupFlags flags )
  : QMenu( parentWidget ),
    m_actions( actions ),
    m_ownActions( static_cast<QWidget *>( 0 ) ),
    m_pMenuNew( newMenu ),
    m_sViewURL(viewURL),
    m_lstItems(items),
    m_pManager(mgr)
{
    init(parentWidget, kpf, flags);
}

void KonqPopupMenu::init (QWidget * parentWidget, KonqPopupFlags kpf, KParts::BrowserExtension::PopupFlags flags)
{
    m_ownActions.setObjectName("KonqPopupMenu::m_ownActions");
    d = new KonqPopupMenuPrivate;
    d->m_parentWidget = parentWidget;
    d->m_itemFlags = flags;
    setup(kpf);
}

int KonqPopupMenu::insertServicesSubmenus(const QMap<QString, ServiceList>& submenus,
                                          QDomElement& menu,
                                          bool isBuiltin)
{
    int count = 0;
    QMap<QString, ServiceList>::ConstIterator it;

    for (it = submenus.begin(); it != submenus.end(); ++it)
    {
        if (it.value().isEmpty())
        {
            //avoid empty sub-menus
            continue;
        }

        QDomElement actionSubmenu = domDocument().createElement( "menu" );
        actionSubmenu.setAttribute( "name", "actions " + it.key() );
        menu.appendChild( actionSubmenu );
        QDomElement subtext = domDocument().createElement( "text" );
        actionSubmenu.appendChild( subtext );
        subtext.appendChild( domDocument().createTextNode( it.key() ) );
        count += insertServices(it.value(), actionSubmenu, isBuiltin);
    }

    return count;
}

int KonqPopupMenu::insertServices(const ServiceList& list,
                                  QDomElement& menu,
                                  bool isBuiltin)
{
    static int id = 1000;
    int count = 0;

    ServiceList::const_iterator it = list.begin();
    for( ; it != list.end(); ++it )
    {
        if ((*it).isEmpty())
        {
            if (!menu.firstChild().isNull() &&
                menu.lastChild().toElement().tagName().toLower() != "separator")
            {
                QDomElement separator = domDocument().createElement( "separator" );
                menu.appendChild(separator);
            }
            continue;
        }

        if (isBuiltin || (*it).m_display == true)
        {
            QString name;
            name.setNum( id );
            name.prepend( isBuiltin ? "builtinservice_" : "userservice_" );
            QAction* act = m_ownActions.addAction( name.toLatin1() );
            act->setText( QString((*it).m_strName).replace('&',"&&") );
            connect(act, SIGNAL(triggered()), this, SLOT(slotRunService()));

            if ( !(*it).m_strIcon.isEmpty() )
            {
                act->setIcon( KIcon((*it).m_strIcon) );
            }

            KonqXMLGUIClient::addAction( name, menu ); // Add to toplevel menu

            m_mapPopupServices[ id++ ] = *it;
            ++count;
        }
    }

    return count;
}

bool KonqPopupMenu::KIOSKAuthorizedAction(const KConfigGroup& cfg)
{
    if ( !cfg.hasKey( "X-KDE-AuthorizeAction") )
    {
        return true;
    }

    QStringList list = cfg.readEntry("X-KDE-AuthorizeAction", QStringList() );
    if (kapp && !list.isEmpty())
    {
        for(QStringList::ConstIterator it = list.begin();
            it != list.end();
            ++it)
        {
            if (!KAuthorized::authorize((*it).trimmed()))
            {
                return false;
            }
        }
    }

    return true;
}


void KonqPopupMenu::setup(KonqPopupFlags kpf)
{
    Q_ASSERT( m_lstItems.count() >= 1 );

    m_ownActions.setAssociatedWidget( this );

    const bool bIsLink  = (kpf & IsLink);
    bool currentDir     = false;
    bool sReading       = true;
    bool sDeleting      = ( d->m_itemFlags & KParts::BrowserExtension::NoDeletion ) == 0;
    bool sMoving        = sDeleting;
    bool sWriting       = sDeleting && m_lstItems.first()->isWritable();
    m_sMimeType         = m_lstItems.first()->mimetype();
    QString mimeGroup   = m_sMimeType.left(m_sMimeType.indexOf('/'));
    mode_t mode         = m_lstItems.first()->mode();
    bool isDirectory    = S_ISDIR(mode);
    bool bTrashIncluded = false;
    bool mediaFiles     = false;
    bool isLocal        = m_lstItems.first()->isLocalFile()
                       || m_lstItems.first()->url().protocol()=="media"
                       || m_lstItems.first()->url().protocol()=="system";
    bool isTrashLink     = false;
    m_lstPopupURLs.clear();
    int id = 0;
    setFont(KGlobalSettings::menuFont());

    attrName = QLatin1String( "name" );

    prepareXMLGUIStuff();
    m_builder = new KonqPopupMenuGUIBuilder( this );
    m_factory = new KXMLGUIFactory( m_builder );

    KUrl url;
    KFileItemList::const_iterator it = m_lstItems.begin();
    const KFileItemList::const_iterator kend = m_lstItems.end();
    QStringList mimeTypeList;
    // Check whether all URLs are correct
    for ( ; it != kend; ++it )
    {
        url = (*it)->url();

        // Build the list of URLs
        m_lstPopupURLs.append( url );

        // Determine if common mode among all URLs
        if ( mode != (*it)->mode() )
            mode = 0; // modes are different => reset to 0

        // Determine if common mimetype among all URLs
        if ( m_sMimeType != (*it)->mimetype() )
        {
            m_sMimeType.clear(); // mimetypes are different => null

            if ( mimeGroup != (*it)->mimetype().left((*it)->mimetype().indexOf('/')))
                mimeGroup.clear(); // mimetype groups are different as well!
        }

        if ( mimeTypeList.indexOf( (*it)->mimetype() ) == -1 )
            mimeTypeList << (*it)->mimetype();

        if ( isLocal && !url.isLocalFile() && url.protocol() != "media" && url.protocol() != "system" )
            isLocal = false;

        if ( !bTrashIncluded && (
             ( url.protocol() == "trash" && url.path().length() <= 1 )
             || url.url() == "system:/trash" || url.url() == "system:/trash/" ) ) {
            bTrashIncluded = true;
            isLocal = false;
        }

        if ( sReading )
            sReading = KProtocolManager::supportsReading( url );

        if ( sWriting )
            sWriting = KProtocolManager::supportsWriting( url ) && (*it)->isWritable();

        if ( sDeleting )
            sDeleting = KProtocolManager::supportsDeleting( url );

        if ( sMoving )
            sMoving = KProtocolManager::supportsMoving( url );
        if ( (*it)->mimetype().startsWith("media/") )
            mediaFiles = true;
    }
    url = m_sViewURL;
    url.cleanPath();

    //check if url is current directory
    if ( m_lstItems.count() == 1 )
    {
        KUrl firstPopupURL( m_lstItems.first()->url() );
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
    const bool isSingleLocal = m_lstItems.count() == 1 && isLocal;

    m_info.m_Reading = sReading;
    m_info.m_Writing = sWriting;
    m_info.m_Deleting = sDeleting;
    m_info.m_Moving = sMoving;
    m_info.m_TrashIncluded = bTrashIncluded;

    // isCurrentTrash: popup on trash:/ itself, or on the trash.desktop link
    bool isCurrentTrash = ( m_lstItems.count() == 1 && bTrashIncluded ) || isTrashLink;
    bool isIntoTrash = ( url.protocol() == "trash" || url.url().startsWith( "system:/trash" ) ) && !isCurrentTrash; // trashed file, not trash:/ itself
    //kDebug() << "isLocal=" << isLocal << " url=" << url << " isCurrentTrash=" << isCurrentTrash << " isIntoTrash=" << isIntoTrash << " bTrashIncluded=" << bTrashIncluded;
    bool isSingleMedium = m_lstItems.count() == 1 && mediaFiles;
    clear();

    //////////////////////////////////////////////////////////////////////////

    QAction * act;

    if (!isCurrentTrash)
        addMerge( "konqueror" );

    bool isKDesktop = QByteArray( kapp->objectName().toUtf8() ) == "kdesktop";
    QAction *actNewWindow = 0;

    if (( kpf & ShowProperties ) && isKDesktop &&
        !KAuthorized::authorizeKAction("editable_desktop_icons"))
    {
        kpf &= ~ShowProperties; // remove flag
    }

    // Either 'newview' is in the actions we're given (probably in the tabhandling group)
    // or we need to insert it ourselves (e.g. for kdesktop). In the first case, actNewWindow must remain 0.
    if ( ((kpf & ShowNewWindow) != 0) && sReading )
    {
        QString openStr = isKDesktop ? i18n( "&Open" ) : i18n( "Open in New &Window" );
        actNewWindow = m_ownActions.addAction( "newview" );
        actNewWindow->setIcon( KIcon("window-new") );
        actNewWindow->setText( openStr );
        connect(actNewWindow, SIGNAL(triggered()), this, SLOT(slotPopupNewView()));
    }

    if ( actNewWindow && !isKDesktop )
    {
        if (isCurrentTrash)
            actNewWindow->setToolTip( i18n( "Open the trash in a new window" ) );
        else if (isSingleMedium)
            actNewWindow->setToolTip( i18n( "Open the medium in a new window") );
        else
            actNewWindow->setToolTip( i18n( "Open the document in a new window" ) );
    }

    if ( S_ISDIR(mode) && sWriting && !isCurrentTrash ) // A dir, and we can create things into it
    {
        if ( currentDir && m_pMenuNew ) // Current dir -> add the "new" menu
        {
            // As requested by KNewMenu :
            m_pMenuNew->slotCheckUpToDate();
            m_pMenuNew->setPopupFiles( m_lstPopupURLs );

            KonqXMLGUIClient::addAction( m_pMenuNew->objectName() );

            KonqXMLGUIClient::addSeparator();
        }
        else
        {
            if (d->m_itemFlags & KParts::BrowserExtension::ShowCreateDirectory)
            {
                QAction *actNewDir = m_ownActions.addAction( "newdir" );
                actNewDir->setIcon( KIcon("folder-new") );
                actNewDir->setText( i18n( "Create &Folder..." ) );
                connect(actNewDir, SIGNAL(triggered()), this, SLOT(slotPopupNewDir()));
                KonqXMLGUIClient::addAction( "newdir" );
                KonqXMLGUIClient::addSeparator();
            }
        }
    } else if ( isIntoTrash ) {
        // Trashed item, offer restoring
        act = m_ownActions.addAction( "restore" );
        act->setText( i18n( "&Restore" ) );
        connect(act, SIGNAL(triggered()), this, SLOT(slotPopupRestoreTrashedItems()));
        KonqXMLGUIClient::addAction( "restore" );
    }

    if (d->m_itemFlags & KParts::BrowserExtension::ShowNavigationItems)
    {
        if (d->m_itemFlags & KParts::BrowserExtension::ShowUp)
            KonqXMLGUIClient::addAction( "up" );
        KonqXMLGUIClient::addAction( "back" );
        KonqXMLGUIClient::addAction( "forward" );
        if (d->m_itemFlags & KParts::BrowserExtension::ShowReload)
            KonqXMLGUIClient::addAction( "reload" );
        KonqXMLGUIClient::addSeparator();
    }

    // "open in new window" is either provided by us, or by the tabhandling group
    if (actNewWindow)
    {
        KonqXMLGUIClient::addAction( "newview" );
        KonqXMLGUIClient::addSeparator();
    }
    KonqXMLGUIClient::addGroup( "tabhandling" ); // includes a separator

    if ( !bIsLink )
    {
        if ( !currentDir && sReading ) {
            if ( sDeleting ) {
                KonqXMLGUIClient::addAction( "cut" );
            }
            KonqXMLGUIClient::addAction( "copy" );
        }

        if ( S_ISDIR(mode) && sWriting ) {
            if ( currentDir )
                KonqXMLGUIClient::addAction( "paste" );
            else
                KonqXMLGUIClient::addAction( "pasteto" );
        }
        if ( !currentDir )
        {
            if ( m_lstItems.count() == 1 && sMoving )
                KonqXMLGUIClient::addAction( "rename" );

            bool addTrash = false;
            bool addDel = false;

            if ( sMoving && !isIntoTrash && !isTrashLink )
                addTrash = true;

            if ( sDeleting ) {
                if ( !isLocal )
                    addDel = true;
                else if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
                    addTrash = false;
                    addDel = true;
                }
                else {
                    KConfigGroup configGroup( KGlobal::config(), "KDE" );
                    if ( configGroup.readEntry( "ShowDeleteCommand", false) )
                        addDel = true;
                }
            }

            if ( addTrash )
                KonqXMLGUIClient::addAction( "trash" );
            if ( addDel )
                KonqXMLGUIClient::addAction( "del" );
        }
    }
    if ( isCurrentTrash )
    {
        act = m_ownActions.addAction( "emptytrash" );
        act->setIcon( KIcon("emptytrash") );
        act->setText( i18n( "&Empty Trash Bin" ) );
        KConfig trashConfig( "trashrc", KConfig::OnlyLocal);
        act->setEnabled( !trashConfig.group("Status").readEntry( "Empty", true ) );
        connect(act, SIGNAL(triggered()), this, SLOT(slotPopupEmptyTrashBin()));
        KonqXMLGUIClient::addAction( "emptytrash" );
    }
    KonqXMLGUIClient::addGroup( "editactions" );

    if (d->m_itemFlags & KParts::BrowserExtension::ShowTextSelectionItems) {
      KonqXMLGUIClient::addMerge( 0 );
      m_factory->addClient( this );
      return;
    }

    if ( !isCurrentTrash && !isIntoTrash && (d->m_itemFlags & KParts::BrowserExtension::ShowBookmark))
    {
        KonqXMLGUIClient::addSeparator();
        QString caption;
        if (currentDir)
        {
           bool httpPage = (m_sViewURL.protocol().indexOf("http", 0, Qt::CaseInsensitive) == 0);
           if (httpPage)
              caption = i18n("&Bookmark This Page");
           else
              caption = i18n("&Bookmark This Location");
        }
        else if (S_ISDIR(mode))
           caption = i18n("&Bookmark This Folder");
        else if (bIsLink)
           caption = i18n("&Bookmark This Link");
        else
           caption = i18n("&Bookmark This File");

        act = m_ownActions.addAction( "bookmark_add" );
        act->setIcon( KIcon("bookmark-new") );
        act->setText( caption );
        connect(act, SIGNAL(triggered()), this, SLOT(slotPopupAddToBookmark()));
        if (m_lstItems.count() > 1)
            act->setEnabled(false);
        if (KAuthorized::authorizeKAction("bookmarks"))
            KonqXMLGUIClient::addAction( "bookmark_add" );
        if (bIsLink)
            KonqXMLGUIClient::addGroup( "linkactions" );
    }

    //////////////////////////////////////////////////////

    PopupServices s;
    KUrl urlForServiceMenu( m_lstItems.first()->url() );

    // 1 - Look for builtin and user-defined services
    if ( m_sMimeType == "application/x-desktop" && isSingleLocal ) // .desktop file
    {
        // get builtin services, like mount/unmount
        s.builtin = KDesktopFileActions::builtinServices( m_lstItems.first()->url() );
        const QString path = m_lstItems.first()->url().path();
        KDesktopFile desktopFile( path );
        KConfigGroup cfg = desktopFile.desktopGroup();
        const QString priority = cfg.readEntry("X-KDE-Priority");
        const QString submenuName = cfg.readEntry( "X-KDE-Submenu" );
        if ( cfg.readEntry("Type") == "Link" ) {
           urlForServiceMenu = cfg.readEntry("URL");
           // TODO: Do we want to make all the actions apply on the target
           // of the .desktop file instead of the .desktop file itself?
        }
        ServiceList* list = s.selectList( priority, submenuName );
        (*list) = KDesktopFileActions::userDefinedServices( path, desktopFile, url.isLocalFile() );
    }

    if ( sReading )
    {

        // 2 - Look for "servicesmenus" bindings (konqueror-specific user-defined services)

        // first check the .directory if this is a directory
        if (isDirectory && isSingleLocal)
        {
            QString dotDirectoryFile = m_lstItems.first()->url().path( KUrl::AddTrailingSlash ).append(".directory");
            KDesktopFile desktopFile(  dotDirectoryFile );
            const KConfigGroup cfg = desktopFile.desktopGroup();

            if (KIOSKAuthorizedAction(cfg))
            {
                const QString priority = cfg.readEntry("X-KDE-Priority");
                const QString submenuName = cfg.readEntry( "X-KDE-Submenu" );
                ServiceList* list = s.selectList( priority, submenuName );
                (*list) += KDesktopFileActions::userDefinedServices( dotDirectoryFile, desktopFile, true );
            }
        }

        // findAllResources() also removes duplicates
        const QStringList entries = KGlobal::dirs()->findAllResources( "data",
                                                                       "konqueror/servicemenus/*.desktop",
                                                                       KStandardDirs::NoDuplicates );
        QStringList::ConstIterator eIt = entries.begin();
        const QStringList::ConstIterator eEnd = entries.end();
        for (; eIt != eEnd; ++eIt )
        {
            KDesktopFile desktopFile( *eIt );
            const KConfigGroup cfg = desktopFile.desktopGroup();

            if (!KIOSKAuthorizedAction(cfg))
            {
                continue;
            }

            if ( cfg.hasKey( "X-KDE-ShowIfRunning" ) )
            {
                const QString app = cfg.readEntry( "X-KDE-ShowIfRunning" );
                if ( QDBusConnection::sessionBus().interface()->isServiceRegistered( app ) )
                    continue;
            }
            if ( cfg.hasKey( "X-KDE-ShowIfDBusCall" ) )
            {
                QString calldata = cfg.readEntry( "X-KDE-ShowIfDBusCall" );
                QStringList parts = calldata.split(' ');
                const QString &app = parts.at(0);
                const QString &obj = parts.at(1);
                QString interface = parts.at(2);
                QString method;
                int pos = interface.lastIndexOf( QLatin1Char( '.' ) );
                if ( pos != -1 ) {
                    method = interface.mid(pos + 1);
                    interface.truncate(pos);
                }

                //if ( !QDBus::sessionBus().busService()->nameHasOwner( app ) )
                //    continue; //app does not exist so cannot send call

                QDBusMessage reply = QDBusInterface( app, obj, interface ).
                                     call( method, m_lstPopupURLs.toStringList() );
                if ( reply.arguments().count() < 1 || reply.arguments().at(0).type() != QVariant::Bool || !reply.arguments().at(0).toBool() )
                    continue;

            }
            if ( cfg.hasKey( "X-KDE-Protocol" ) )
            {
                const QString protocol = cfg.readEntry( "X-KDE-Protocol" );
                if ( protocol != urlForServiceMenu.protocol() )
                    continue;
            }
            else if ( cfg.hasKey( "X-KDE-Protocols" ) )
            {
                const QStringList protocols = cfg.readEntry( "X-KDE-Protocols" ).split( ',' );
                if ( !protocols.contains( urlForServiceMenu.protocol() ) )
                    continue;
            }
            else if ( urlForServiceMenu.protocol() == "trash" || urlForServiceMenu.url().startsWith( "system:/trash" ) )
            {
                // Require servicemenus for the trash to ask for protocol=trash explicitly.
                // Trashed files aren't supposed to be available for actions.
                // One might want a servicemenu for trash.desktop itself though.
                continue;
            }

            if ( cfg.hasKey( "X-KDE-Require" ) )
            {
                const QStringList capabilities = cfg.readEntry( "X-KDE-Require" , QStringList() );
                if ( capabilities.contains( "Write" ) && !sWriting )
                    continue;
            }
            if ( (cfg.hasKey( "Actions" ) || cfg.hasKey( "X-KDE-GetActionMenu") ) && cfg.hasKey( "ServiceTypes" ) )
            {
                const QStringList types = cfg.readEntry( "ServiceTypes" , QStringList() );
                const QStringList excludeTypes = cfg.readEntry( "ExcludeServiceTypes" , QStringList() );
                bool ok = false;

                // check for exact matches or a typeglob'd mimetype if we have a mimetype
                for (QStringList::ConstIterator it = types.begin();
                     it != types.end() && !ok;
                     ++it)
                {
                    // first check if we have an all mimetype
                    bool checkTheMimetypes = false;
                    if (*it == "all/all" ||
                        *it == "allfiles" /*compat with KDE up to 3.0.3*/)
                    {
                        checkTheMimetypes = true;
                    }

                    // next, do we match all files?
                    if (!ok &&
                        !isDirectory &&
                        *it == "all/allfiles")
                    {
                        checkTheMimetypes = true;
                    }

                    // if we have a mimetype, see if we have an exact or a type globbed match
                    if (!ok &&
                        (!m_sMimeType.isEmpty() &&
                         *it == m_sMimeType) ||
                        (!mimeGroup.isEmpty() &&
                         ((*it).right(1) == "*" &&
                          (*it).left((*it).indexOf('/')) == mimeGroup)))
                    {
                        checkTheMimetypes = true;
                    }

                    if (checkTheMimetypes)
                    {
                        ok = true;
                        for (QStringList::ConstIterator itex = excludeTypes.begin(); itex != excludeTypes.end(); ++itex)
                        {
                            if( ((*itex).right(1) == "*" && (*itex).left((*itex).indexOf('/')) == mimeGroup) ||
                                ((*itex) == m_sMimeType) )
                            {
                                ok = false;
                                break;
                            }
                        }
                    }
                }

                if ( ok )
                {
                    const QString priority = cfg.readEntry("X-KDE-Priority");
                    const QString submenuName = cfg.readEntry( "X-KDE-Submenu" );

                    ServiceList* list = s.selectList( priority, submenuName );
                    (*list) += KDesktopFileActions::userDefinedServices( *eIt, desktopFile, url.isLocalFile(), m_lstPopupURLs );
                }
            }
        }

        KService::List offers;

        if (KAuthorized::authorizeKAction("openwith"))
        {
            QString constraint = "DesktopEntryName != 'kfmclient' and DesktopEntryName != 'kfmclient_dir' and DesktopEntryName != 'kfmclient_html'";
            QString subConstraint = " and '%1' in ServiceTypes";

            QStringList::ConstIterator it = mimeTypeList.begin();
            QStringList::ConstIterator end = mimeTypeList.end();
            Q_ASSERT( it != end );
            QString first = *it;
            ++it;
            while ( it != end ) {
                constraint += subConstraint.arg( *it );
                ++it;
            }

            offers = KMimeTypeTrader::self()->query( first, "Application", constraint );
        }

        //// Ok, we have everything, now insert

        m_mapPopup.clear();
        m_mapPopupServices.clear();
        // "Open With..." for folders is really not very useful, especially for remote folders.
        // (media:/something, or trash:/, or ftp://...)
        if ( !isDirectory || isLocal )
        {
            if ( hasAction() )
                KonqXMLGUIClient::addSeparator();

            if ( !offers.isEmpty() )
            {
                // First block, app and preview offers
                id = 1;

                QDomElement menu = domElement();

                if ( offers.count() > 1 ) // submenu 'open with'
                {
                    menu = domDocument().createElement( "menu" );
                    menu.setAttribute( "name", "openwith submenu" );
                    domElement().appendChild( menu );
                    QDomElement text = domDocument().createElement( "text" );
                    menu.appendChild( text );
                    text.appendChild( domDocument().createTextNode( i18n("&Open With") ) );
                }

                KService::List::ConstIterator it = offers.begin();
                for( ; it != offers.end(); it++ )
                {
                    KService::Ptr service = (*it);

                    // Skip OnlyShowIn=Foo and NotShowIn=KDE entries,
                    // but still offer NoDisplay=true entries, that's the
                    // whole point of such desktop files. This is why we don't
                    // use service->noDisplay() here.
                    const QString onlyShowIn = service->property("OnlyShowIn", QVariant::String).toString();
                    if ( !onlyShowIn.isEmpty() ) {
                        const QStringList aList = onlyShowIn.split(';', QString::SkipEmptyParts);
                        if (!aList.contains("KDE"))
                            continue;
                    }
                    const QString notShowIn = service->property("NotShowIn", QVariant::String).toString();
                    if ( !notShowIn.isEmpty() ) {
                        const QStringList aList = notShowIn.split(';', QString::SkipEmptyParts);
                        if (aList.contains("KDE"))
                            continue;
                    }


                    QByteArray nam;
                    nam.setNum( id );

                    QString actionName( service->name().replace( "&", "&&" ) );
                    if ( menu == domElement() ) // no submenu -> prefix single offer
                        actionName = i18n( "Open with %1" ,  actionName );

                    act = m_ownActions.addAction( nam.prepend( "appservice_" ) );
                    act->setIcon( KIcon( service->icon() ) );
                    act->setText( actionName );
                    connect(act, SIGNAL(triggered()), this, SLOT(slotRunService()));
                    KonqXMLGUIClient::addAction( nam.prepend( "appservice_" ).constData(), menu );

                    m_mapPopup[ id++ ] = *it;
                }

                QString openWithActionName;
                if ( menu != domElement() ) // submenu
                {
                    KonqXMLGUIClient::addSeparator( menu );
                    openWithActionName = i18n( "&Other..." );
                }
                else
                {
                    openWithActionName = i18n( "&Open With..." );
                }
                QAction *openWithAct = m_ownActions.addAction( "openwith" );
                openWithAct->setText( openWithActionName );
                connect(openWithAct, SIGNAL(triggered()), this, SLOT(slotPopupOpenWith()));
                KonqXMLGUIClient::addAction( "openwith", menu );
            }
            else // no app offers -> Open With...
            {
                act = m_ownActions.addAction( "openwith" );
                act->setText( i18n( "&Open With..." ) );
                connect(act, SIGNAL(triggered()), this, SLOT(slotPopupOpenWith()));
                KonqXMLGUIClient::addAction( "openwith" );
            }

        }
        KonqXMLGUIClient::addGroup( "preview" );
    }

    // Second block, builtin + user
    QDomElement actionMenu = domElement();
    int userItemCount = 0;
    if (s.user.count() + s.userSubmenus.count() +
        s.userPriority.count() + s.userPrioritySubmenus.count() > 1)
    {
        // we have more than one item, so let's make a submenu
        actionMenu = domDocument().createElement( "menu" );
        actionMenu.setAttribute( "name", "actions submenu" );
        domElement().appendChild( actionMenu );
        QDomElement text = domDocument().createElement( "text" );
        actionMenu.appendChild( text );
        text.appendChild( domDocument().createTextNode( i18n("Ac&tions") ) );
    }

    userItemCount += insertServicesSubmenus(s.userPrioritySubmenus, actionMenu, false);
    userItemCount += insertServices(s.userPriority, actionMenu, false);

    // see if we need to put a separator between our priority items and our regular items
    if (userItemCount > 0 &&
        (s.user.count() > 0 ||
         s.userSubmenus.count() > 0 ||
         s.builtin.count() > 0) &&
         actionMenu.lastChild().toElement().tagName().toLower() != "separator")
    {
        QDomElement separator = domDocument().createElement( "separator" );
        actionMenu.appendChild(separator);
    }
	QDomElement element = domElement( );
    userItemCount += insertServicesSubmenus(s.userSubmenus, actionMenu, false);
    userItemCount += insertServices(s.user, actionMenu, false);
    userItemCount += insertServices(s.builtin, element, true);

    userItemCount += insertServicesSubmenus(s.userToplevelSubmenus, element, false);
    userItemCount += insertServices(s.userToplevel, element, false);

    if ( userItemCount > 0 )
    {
        addPendingSeparator();
    }

    if ( !isCurrentTrash && !isIntoTrash && !mediaFiles && sReading )
        addPlugins(); // now it's time to add plugins

    if ( KPropertiesDialog::canDisplay( m_lstItems ) && (kpf & ShowProperties) )
    {
        act = m_ownActions.addAction( "properties" );
        act->setText( i18n( "&Properties" ) );
        connect(act, SIGNAL(triggered()), this, SLOT(slotPopupProperties()));
        KonqXMLGUIClient::addAction( "properties" );
    }

    while ( !domElement().lastChild().isNull() &&
            domElement().lastChild().toElement().tagName().toLower() == "separator" )
        domElement().removeChild( domElement().lastChild() );

    if ( isDirectory && isLocal )
    {
        if ( KFileShare::authorization() == KFileShare::Authorized )
        {
            KonqXMLGUIClient::addSeparator();
            act = m_ownActions.addAction( "sharefile" );
            act->setText( i18n("Share") );
            connect(act, SIGNAL(triggered()), this, SLOT(slotOpenShareFileDialog()));
            KonqXMLGUIClient::addAction( "sharefile" );
        }
    }

    KonqXMLGUIClient::addMerge( 0 );
    //kDebug() << domDocument().toString();

    m_factory->addClient( this );
}

void KonqPopupMenu::slotOpenShareFileDialog()
{
    KPropertiesDialog* dlg = new KPropertiesDialog( m_lstItems, d->m_parentWidget );
    dlg->showFileSharingPage();
    dlg->exec();
}

KonqPopupMenu::~KonqPopupMenu()
{
  delete m_factory;
  delete m_builder;
  delete d;
  //kDebug(1203) << "~KonqPopupMenu leave";
}

void KonqPopupMenu::setURLTitle( const QString& urlTitle )
{
    d->m_urlTitle = urlTitle;
}

void KonqPopupMenu::slotPopupNewView()
{
  KUrl::List::ConstIterator it = m_lstPopupURLs.begin();
  for ( ; it != m_lstPopupURLs.end(); it++ )
    (void) new KRun(*it,this);
}

void KonqPopupMenu::slotPopupNewDir()
{
  if (m_lstPopupURLs.empty())
    return;

  KonqOperations::newDir(d->m_parentWidget, m_lstPopupURLs.first());
}

void KonqPopupMenu::slotPopupEmptyTrashBin()
{
  KonqOperations::emptyTrash( d->m_parentWidget );
}

void KonqPopupMenu::slotPopupRestoreTrashedItems()
{
  KonqOperations::restoreTrashedItems( m_lstPopupURLs, d->m_parentWidget );
}

void KonqPopupMenu::slotPopupOpenWith()
{
  KRun::displayOpenWithDialog( m_lstPopupURLs, d->m_parentWidget );
}

void KonqPopupMenu::slotPopupAddToBookmark()
{
  KBookmarkGroup root;
  if ( m_lstPopupURLs.count() == 1 ) {
    KUrl url = m_lstPopupURLs.first();
    QString title = d->m_urlTitle.isEmpty() ? url.prettyUrl() : d->m_urlTitle;
    root = m_pManager->addBookmarkDialog( url.prettyUrl(), title );
  }
  else
  {
    root = m_pManager->root();
    KUrl::List::ConstIterator it = m_lstPopupURLs.begin();
    for ( ; it != m_lstPopupURLs.end(); it++ )
      root.addBookmark( (*it).prettyUrl(), (*it) );
  }
  m_pManager->emitChanged( root );
}

void KonqPopupMenu::slotRunService()
{
  QByteArray senderName = sender()->objectName().toUtf8();
  int id = senderName.mid( senderName.indexOf( '_' ) + 1 ).toInt();

  // Is it a usual service (application)
  QMap<int,KService::Ptr>::Iterator it = m_mapPopup.find( id );
  if ( it != m_mapPopup.end() )
  {
    KRun::run( **it, m_lstPopupURLs, topLevelWidget() );
    return;
  }

  // Is it a service specific to desktop entry files ?
  QMap<int,KDesktopFileActions::Service>::Iterator it2 = m_mapPopupServices.find( id );
  if ( it2 != m_mapPopupServices.end() )
  {
      KDesktopFileActions::executeService( m_lstPopupURLs, it2.value() );
  }

  return;
}

void KonqPopupMenu::slotPopupMimeType()
{
    KonqOperations::editMimeType( m_sMimeType, d->m_parentWidget );
}

void KonqPopupMenu::slotPopupProperties()
{
    KPropertiesDialog::showDialog( m_lstItems, d->m_parentWidget );
}

QAction *KonqPopupMenu::action( const QDomElement &element ) const
{
  QByteArray name = element.attribute( attrName ).toLatin1();

  QAction *res = m_ownActions.action( name.data() );

  if ( !res )
    res = m_actions.action( name.data() );

  if ( !res && m_pMenuNew && strcmp( name.data(), m_pMenuNew->objectName().toUtf8() ) == 0 )
    return m_pMenuNew;

  return res;
}

KActionCollection *KonqPopupMenu::actionCollection() const
{
    return const_cast<KActionCollection *>( &m_ownActions );
}

QString KonqPopupMenu::mimeType() const
{
    return m_sMimeType;
}

KonqPopupMenu::ProtocolInfo KonqPopupMenu::protocolInfo() const
{
    return m_info;
}

void KonqPopupMenu::addPlugins()
{
    // search for Konq_PopupMenuPlugins inspired by simons kpropsdlg
    //search for a plugin with the right protocol
    KService::List plugin_offers;
    unsigned int pluginCount = 0;
    plugin_offers = KMimeTypeTrader::self()->query( m_sMimeType.isNull() ? QLatin1String( "all/all" ) : m_sMimeType, "KonqPopupMenu/Plugin" );
    if ( plugin_offers.isEmpty() )
        return; // no plugins installed do not bother about it

    KService::List::ConstIterator iterator = plugin_offers.begin();
    KService::List::ConstIterator end = plugin_offers.end();

    addGroup( "plugins" );
    // travers the offerlist
    for(; iterator != end; ++iterator, ++pluginCount ) {
        //kDebug() << (*iterator)->library();
        KonqPopupMenuPlugin *plugin =
            KLibLoader::createInstance<KonqPopupMenuPlugin>( QFile::encodeName( (*iterator)->library() ),
                                                            this );
        if ( !plugin )
            continue;
        plugin->setObjectName( (*iterator)->name() );
        QString pluginClientName = QString::fromLatin1( "Plugin%1" ).arg( pluginCount );
        addMerge( pluginClientName );
        plugin->domDocument().documentElement().setAttribute( "name", pluginClientName );
        insertChildClient( plugin );
    }

    // ## Where is this used?
    addMerge( "plugins" );
}

KUrl KonqPopupMenu::url() const // ### should be viewURL()
{
  return m_sViewURL;
}

KFileItemList KonqPopupMenu::fileItemList() const
{
  return m_lstItems;
}

KUrl::List KonqPopupMenu::popupURLList() const
{
  return m_lstPopupURLs;
}

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

#include "konq_popupmenu.moc"
