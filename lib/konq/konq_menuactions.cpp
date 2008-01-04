/* This file is part of the KDE project
   Copyright (C) 1998-2007 David Faure <faure@kde.org>

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

#include "konq_menuactions.h"
#include "konq_menuactions_p.h"
#include <kdesktopfileactions.h>
#include <kmenu.h>
#include <klocale.h>
#include <kauthorized.h>
#include <kconfiggroup.h>
#include <kdesktopfile.h>
#include <kglobal.h>
#include <kicon.h>
#include <kstandarddirs.h>
#include <KService>
#include <KServiceTypeTrader>
#include <QFile>

#include <QtDBus/QtDBus>

static bool KIOSKAuthorizedAction(const KConfigGroup& cfg)
{
    if ( !cfg.hasKey( "X-KDE-AuthorizeAction") ) {
        return true;
    }
    const QStringList list = cfg.readEntry("X-KDE-AuthorizeAction", QStringList() );
    for(QStringList::ConstIterator it = list.begin();
        it != list.end(); ++it) {
        if (!KAuthorized::authorize((*it).trimmed())) {
            return false;
        }
    }
    return true;
}

// This helper class stores the .desktop-file actions and the servicemenus
// in order to support X-KDE-Priority and X-KDE-Submenu.
class PopupServices
{
public:
    ServiceList& selectList( const QString& priority, const QString& submenuName );

    ServiceList builtin;
    ServiceList user, userToplevel, userPriority;
    QMap<QString, ServiceList> userSubmenus, userToplevelSubmenus, userPrioritySubmenus;
};

ServiceList& PopupServices::selectList( const QString& priority, const QString& submenuName )
{
    // we use the categories .desktop entry to define submenus
    // if none is defined, we just pop it in the main menu
    if (submenuName.isEmpty()) {
        if (priority == "TopLevel") {
            return userToplevel;
        } else if (priority == "Important") {
            return userPriority;
        }
    } else if (priority == "TopLevel") {
        return userToplevelSubmenus[submenuName];
    } else if (priority == "Important") {
        return userPrioritySubmenus[submenuName];
    } else {
        return userSubmenus[submenuName];
    }
    return user;
}

////

KonqMenuActionsPrivate::KonqMenuActionsPrivate()
    : QObject(),
      m_isDirectory(false),
      m_readOnly(false),
      m_executeServiceActionGroup(static_cast<QWidget *>(0)),
      m_ownActions(static_cast<QWidget *>(0))
{
    QObject::connect(&m_executeServiceActionGroup, SIGNAL(triggered(QAction*)),
                     this, SLOT(slotExecuteService(QAction*)));
}

int KonqMenuActionsPrivate::insertServicesSubmenus(const QMap<QString, ServiceList>& submenus,
                                                   QMenu* menu,
                                                   bool isBuiltin)
{
    int count = 0;
    QMap<QString, ServiceList>::ConstIterator it;
    for (it = submenus.begin(); it != submenus.end(); ++it) {
        if (it.value().isEmpty()) {
            //avoid empty sub-menus
            continue;
        }

        QMenu* actionSubmenu = new KMenu(menu);
        menu->menuAction()->setObjectName("services_submenu"); // for the unittest
        menu->addMenu(actionSubmenu);
        count += insertServices(it.value(), actionSubmenu, isBuiltin);
    }

    return count;
}

int KonqMenuActionsPrivate::insertServices(const ServiceList& list,
                                           QMenu* menu,
                                           bool isBuiltin)
{
    int count = 0;
    ServiceList::const_iterator it = list.begin();
    for( ; it != list.end(); ++it ) {
        if ((*it).isSeparator()) {
            const QList<QAction*> actions = menu->actions();
            if (!actions.isEmpty() && !actions.last()->isSeparator()) {
                menu->addSeparator();
            }
            continue;
        }

        if (isBuiltin || !(*it).noDisplay()) {
            QAction* act = new QAction(&m_ownActions);
            QString text = (*it).text();
            text.replace('&',"&&");
            act->setText( text );
            if ( !(*it).icon().isEmpty() ) {
                act->setIcon( KIcon((*it).icon()) );
            }
            // act->setData(...);
            m_executeServiceActionGroup.addAction(act);

            menu->addAction(act); // Add to toplevel menu

            m_mapPopupServices.insert(act, *it);
            ++count;
        }
    }

    return count;
}

void KonqMenuActionsPrivate::slotExecuteService(QAction* act)
{
    QMap<QAction *,KServiceAction>::Iterator it = m_mapPopupServices.find(act);
    Q_ASSERT(it != m_mapPopupServices.end());
    if (it != m_mapPopupServices.end()) {
        KDesktopFileActions::executeService(m_urlList, it.value());
    }
}

////

KonqMenuActions::KonqMenuActions()
    : d(new KonqMenuActionsPrivate)
{
}

void KonqMenuActions::setItems(const KFileItemList& items)
{
    Q_ASSERT(!items.isEmpty());
    d->m_items = items;
    d->m_mimeType = items.first().mimetype();
    d->m_mimeGroup = d->m_mimeType.left(d->m_mimeType.indexOf('/'));
    d->m_isDirectory = items.first().isDir();
    d->m_urlList = items.urlList();
    if (items.count() > 1) {
        KFileItemList::const_iterator kit = items.begin();
        const KFileItemList::const_iterator kend = items.end();
        for ( ; kit != kend; ++kit ) {
            const QString itemMimeType = (*kit).mimetype();
            if (d->m_mimeType != itemMimeType) {
                d->m_mimeType.clear();
                if (d->m_mimeGroup != itemMimeType.left(itemMimeType.indexOf('/')))
                    d->m_mimeGroup.clear(); // mimetype groups are different as well!
            }
            if (d->m_isDirectory && !(*kit).isDir())
                d->m_isDirectory = false;
        }
    }
    if (d->m_url.isEmpty())
        d->m_url = d->m_urlList.first();
}

void KonqMenuActions::setUrl(const KUrl& url)
{
    Q_ASSERT(!url.isEmpty());
    d->m_url = url;
}

void KonqMenuActions::setReadOnly(bool ro)
{
    d->m_readOnly = ro;
}

int KonqMenuActions::addActionsTo(QMenu* mainMenu)
{
    d->m_mapPopupServices.clear();
    const bool isLocal = d->m_url.isLocalFile();
    const bool isSingleLocal = d->m_urlList.count() == 1 && isLocal;

    PopupServices s;

    // 1 - Look for builtin and user-defined services
    if (isSingleLocal && d->m_mimeType == "application/x-desktop") // .desktop file
    {
        // get builtin services, like mount/unmount
        s.builtin = KDesktopFileActions::builtinServices( d->m_url );
        const QString path = d->m_url.path();
        KDesktopFile desktopFile(path);
        KConfigGroup cfg = desktopFile.desktopGroup();
        const QString priority = cfg.readEntry("X-KDE-Priority");
        const QString submenuName = cfg.readEntry( "X-KDE-Submenu" );
        if ( cfg.readEntry("Type") == "Link" ) {
           d->m_url = cfg.readEntry("URL");
           // TODO: Do we want to make all the actions apply on the target
           // of the .desktop file instead of the .desktop file itself?
        }
        ServiceList& list = s.selectList( priority, submenuName );
        list = KDesktopFileActions::userDefinedServices( path, desktopFile, d->m_url.isLocalFile() );
    }

    // 2 - Look for "servicesmenus" bindings (konqueror-specific user-defined services)

    // first check the .directory if this is a directory
    if (d->m_isDirectory && isSingleLocal) {
        QString dotDirectoryFile = d->m_url.path( KUrl::AddTrailingSlash ).append(".directory");
        if (QFile::exists(dotDirectoryFile)) {
            KDesktopFile desktopFile(  dotDirectoryFile );
            const KConfigGroup cfg = desktopFile.desktopGroup();

            if (KIOSKAuthorizedAction(cfg)) {
                const QString priority = cfg.readEntry("X-KDE-Priority");
                const QString submenuName = cfg.readEntry( "X-KDE-Submenu" );
                ServiceList& list = s.selectList( priority, submenuName );
                list += KDesktopFileActions::userDefinedServices( dotDirectoryFile, desktopFile, true );
            }
        }
    }

    const KService::List entries = KServiceTypeTrader::self()->query( "KonqPopupMenu/Plugin");
    KService::List::const_iterator eEnd = entries.end();
    for (KService::List::const_iterator it2 = entries.begin(); it2 != eEnd; it2++ ) {
        QString file = KStandardDirs::locate("services", (*it2)->entryPath());
        KDesktopFile desktopFile( file );
        const KConfigGroup cfg = desktopFile.desktopGroup();

        if (!KIOSKAuthorizedAction(cfg)) {
            continue;
        }

        if ( cfg.hasKey( "X-KDE-ShowIfRunning" ) ) {
            const QString app = cfg.readEntry( "X-KDE-ShowIfRunning" );
            if ( QDBusConnection::sessionBus().interface()->isServiceRegistered( app ) )
                continue;
        }
        if ( cfg.hasKey( "X-KDE-ShowIfDBusCall" ) ) {
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
                                 call( method, d->m_urlList.toStringList() );
            if ( reply.arguments().count() < 1 || reply.arguments().at(0).type() != QVariant::Bool || !reply.arguments().at(0).toBool() )
                continue;

        }
        if ( cfg.hasKey( "X-KDE-Protocol" ) ) {
            const QString protocol = cfg.readEntry( "X-KDE-Protocol" );
            if (protocol.startsWith('!')) {
                const QString excludedProtocol = protocol.mid(1);
                if (excludedProtocol == d->m_url.protocol())
                    continue;
            } else if (protocol != d->m_url.protocol())
                continue;
        }
        else if ( cfg.hasKey( "X-KDE-Protocols" ) ) {
            const QStringList protocols = cfg.readEntry( "X-KDE-Protocols", QStringList() );
            if ( !protocols.contains( d->m_url.protocol() ) )
                continue;
        }
        else if ( d->m_url.protocol() == "trash" || d->m_url.url().startsWith( "system:/trash" ) ) {
            // Require servicemenus for the trash to ask for protocol=trash explicitly.
            // Trashed files aren't supposed to be available for actions.
            // One might want a servicemenu for trash.desktop itself though.
            continue;
        }

        if ( cfg.hasKey( "X-KDE-Require" ) ) {
            const QStringList capabilities = cfg.readEntry( "X-KDE-Require" , QStringList() );
            if ( capabilities.contains( "Write" ) && d->m_readOnly )
                continue;
        }
        if ( (cfg.hasKey( "Actions" ) || cfg.hasKey( "X-KDE-GetActionMenu") ) && cfg.hasKey( "ServiceTypes" ) ) {
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
                    *it == "allfiles" /*compat with KDE up to 3.0.3*/) {
                    checkTheMimetypes = true;
                }

                // next, do we match all files?
                if (!ok &&
                    !d->m_isDirectory &&
                    *it == "all/allfiles") {
                    checkTheMimetypes = true;
                }

                // if we have a mimetype, see if we have an exact or a type globbed match
                if (!ok &&
                    (!d->m_mimeType.isEmpty() &&
                     *it == d->m_mimeType) ||
                    (!d->m_mimeGroup.isEmpty() &&
                     ((*it).right(1) == "*" &&
                      (*it).left((*it).indexOf('/')) == d->m_mimeGroup))) {
                    checkTheMimetypes = true;
                }

                if (checkTheMimetypes) {
                    ok = true;
                    for (QStringList::ConstIterator itex = excludeTypes.begin(); itex != excludeTypes.end(); ++itex)
                    {
                        if( ((*itex).endsWith('*') && (*itex).left((*itex).indexOf('/')) == d->m_mimeGroup) ||
                            ((*itex) == d->m_mimeType) ) {
                            ok = false;
                            break;
                        }
                    }
                }
            }

            if ( ok ) {
                const QString priority = cfg.readEntry("X-KDE-Priority");
                const QString submenuName = cfg.readEntry( "X-KDE-Submenu" );

                ServiceList& list = s.selectList( priority, submenuName );
                list += KDesktopFileActions::userDefinedServices( *(*it2), d->m_url.isLocalFile(), d->m_urlList );
            }
        }
    }



    QMenu* actionMenu = mainMenu;
    int userItemCount = 0;
    if (s.user.count() + s.userSubmenus.count() +
        s.userPriority.count() + s.userPrioritySubmenus.count() > 1)
    {
        // we have more than one item, so let's make a submenu
        actionMenu = new KMenu(i18nc("@title:menu", "Actions"), mainMenu);
        actionMenu->menuAction()->setObjectName("actions_submenu"); // for the unittest
        mainMenu->addMenu(actionMenu);
    }

    userItemCount += d->insertServicesSubmenus(s.userPrioritySubmenus, actionMenu, false);
    userItemCount += d->insertServices(s.userPriority, actionMenu, false);

    // see if we need to put a separator between our priority items and our regular items
    if (userItemCount > 0 &&
        (s.user.count() > 0 ||
         s.userSubmenus.count() > 0 ||
         s.builtin.count() > 0) &&
        !actionMenu->actions().last()->isSeparator()) {
        actionMenu->addSeparator();
    }
    userItemCount += d->insertServicesSubmenus(s.userSubmenus, actionMenu, false);
    userItemCount += d->insertServices(s.user, actionMenu, false);
    userItemCount += d->insertServices(s.builtin, mainMenu, true);
    userItemCount += d->insertServicesSubmenus(s.userToplevelSubmenus, mainMenu, false);
    userItemCount += d->insertServices(s.userToplevel, mainMenu, false);
    return userItemCount;
}
