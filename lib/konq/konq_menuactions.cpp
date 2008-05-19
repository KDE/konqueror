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
#include <kaction.h>
#include <krun.h>
#include <kmimetypetrader.h>
#include <kdebug.h>
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
      m_executeServiceActionGroup(static_cast<QWidget *>(0)),
      m_runApplicationActionGroup(static_cast<QWidget *>(0)),
      m_ownActions(static_cast<QWidget *>(0))
{
    QObject::connect(&m_executeServiceActionGroup, SIGNAL(triggered(QAction*)),
                     this, SLOT(slotExecuteService(QAction*)));
    QObject::connect(&m_runApplicationActionGroup, SIGNAL(triggered(QAction*)),
                     this, SLOT(slotRunApplication(QAction*)));
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
        actionSubmenu->setTitle( it.key() );
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
            act->setObjectName("menuaction"); // for the unittest
            QString text = (*it).text();
            text.replace('&',"&&");
            act->setText( text );
            if ( !(*it).icon().isEmpty() ) {
                act->setIcon( KIcon((*it).icon()) );
            }
            act->setData(QVariant::fromValue(*it));
            m_executeServiceActionGroup.addAction(act);

            menu->addAction(act); // Add to toplevel menu
            ++count;
        }
    }

    return count;
}

void KonqMenuActionsPrivate::slotExecuteService(QAction* act)
{
    KServiceAction serviceAction = act->data().value<KServiceAction>();
    KDesktopFileActions::executeService(m_info.urlList(), serviceAction);
}

////

KonqMenuActions::KonqMenuActions()
    : d(new KonqMenuActionsPrivate)
{
}


KonqMenuActions::~KonqMenuActions()
{
    delete d;
}

void KonqMenuActions::setPopupMenuInfo(const KonqPopupMenuInformation& info)
{
    d->m_info = info;
}

int KonqMenuActions::addActionsTo(QMenu* mainMenu)
{
    const KFileItemList items = d->m_info.items();
    const KFileItem firstItem = items.first();
    const QString protocol = firstItem.url().protocol(); // assumed to be the same for all items
    const bool isLocal = firstItem.url().isLocalFile();
    const bool isSingleLocal = items.count() == 1 && isLocal;
    const KUrl::List urlList = d->m_info.urlList();

    PopupServices s;

    // 1 - Look for builtin and user-defined services
    if (isSingleLocal && d->m_info.mimeType() == "application/x-desktop") // .desktop file
    {
        // get builtin services, like mount/unmount
        s.builtin = KDesktopFileActions::builtinServices(firstItem.url());
        const QString path = firstItem.url().path();
        KDesktopFile desktopFile(path);
        KConfigGroup cfg = desktopFile.desktopGroup();
        const QString priority = cfg.readEntry("X-KDE-Priority");
        const QString submenuName = cfg.readEntry( "X-KDE-Submenu" );
#if 0
        if ( cfg.readEntry("Type") == "Link" ) {
           d->m_url = cfg.readEntry("URL");
           // TODO: Do we want to make all the actions apply on the target
           // of the .desktop file instead of the .desktop file itself?
        }
#endif
        ServiceList& list = s.selectList(priority, submenuName);
        list = KDesktopFileActions::userDefinedServices(path, desktopFile, true /*isLocal*/);
    }

    // 2 - Look for "servicesmenus" bindings (konqueror-specific user-defined services)

    // first check the .directory if this is a directory
    if (d->m_info.isDirectory() && isSingleLocal) {
        QString dotDirectoryFile = firstItem.url().path(KUrl::AddTrailingSlash).append(".directory");
        if (QFile::exists(dotDirectoryFile)) {
            const KDesktopFile desktopFile(  dotDirectoryFile );
            const KConfigGroup cfg = desktopFile.desktopGroup();

            if (KIOSKAuthorizedAction(cfg)) {
                const QString priority = cfg.readEntry("X-KDE-Priority");
                const QString submenuName = cfg.readEntry( "X-KDE-Submenu" );
                ServiceList& list = s.selectList( priority, submenuName );
                list += KDesktopFileActions::userDefinedServices( dotDirectoryFile, desktopFile, true );
            }
        }
    }

    const QString commonMimeType = d->m_info.mimeType();
    const QString commonMimeGroup = d->m_info.mimeGroup();
    const KMimeType::Ptr mimeTypePtr = commonMimeType.isEmpty() ? KMimeType::Ptr() : KMimeType::mimeType(commonMimeType);
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
                                 call( method, urlList.toStringList() );
            if ( reply.arguments().count() < 1 || reply.arguments().at(0).type() != QVariant::Bool || !reply.arguments().at(0).toBool() )
                continue;

        }
        if ( cfg.hasKey( "X-KDE-Protocol" ) ) {
            const QString protocol = cfg.readEntry( "X-KDE-Protocol" );
            if (protocol.startsWith('!')) {
                const QString excludedProtocol = protocol.mid(1);
                if (excludedProtocol == protocol)
                    continue;
            } else if (protocol != protocol)
                continue;
        }
        else if ( cfg.hasKey( "X-KDE-Protocols" ) ) {
            const QStringList protocols = cfg.readEntry("X-KDE-Protocols", QStringList());
            if (!protocols.contains(protocol))
                continue;
        }
        else if (protocol == "trash") {
            // Require servicemenus for the trash to ask for protocol=trash explicitly.
            // Trashed files aren't supposed to be available for actions.
            // One might want a servicemenu for trash.desktop itself though.
            continue;
        }

        if ( cfg.hasKey( "X-KDE-Require" ) ) {
            const QStringList capabilities = cfg.readEntry( "X-KDE-Require" , QStringList() );
            if ( capabilities.contains( "Write" ) && d->m_info.readOnly() )
                continue;
        }
        if ( cfg.hasKey( "Actions" ) || cfg.hasKey( "X-KDE-GetActionMenu") ) {
            // Like KService, we support ServiceTypes, X-KDE-ServiceTypes, and MimeType.
            QStringList types = cfg.readEntry("ServiceTypes", QStringList());
            types += cfg.readEntry("X-KDE-ServiceTypes", QStringList());
            types += cfg.readXdgListEntry("MimeType");
            //kDebug() << file << types;

            if (types.isEmpty())
                continue;
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
                    !d->m_info.isDirectory() &&
                    *it == "all/allfiles") {
                    checkTheMimetypes = true;
                }

                // if we have a mimetype, see if we have an exact or a type globbed match
                if (!ok &&
                    (mimeTypePtr && mimeTypePtr->is(*it)) ||
                    (!commonMimeGroup.isEmpty() &&
                     ((*it).right(1) == "*" &&
                      (*it).left((*it).indexOf('/')) == commonMimeGroup))) {
                    checkTheMimetypes = true;
                }

                if (checkTheMimetypes) {
                    ok = true;
                    for (QStringList::ConstIterator itex = excludeTypes.begin(); itex != excludeTypes.end(); ++itex)
                    {
                        if( ((*itex).endsWith('*') && (*itex).left((*itex).indexOf('/')) == commonMimeGroup) ||
                            ((*itex) == commonMimeType) ) {
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
                list += KDesktopFileActions::userDefinedServices(*(*it2), isLocal, urlList);
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

void KonqMenuActions::addOpenWithActionsTo(QMenu* topMenu, const QString& traderConstraint)
{
    if (!KAuthorized::authorizeKAction("openwith"))
        return;

    const KFileItemList items = d->m_info.items();
    QStringList mimeTypeList;
    KFileItemList::const_iterator kit = items.constBegin();
    const KFileItemList::const_iterator kend = items.constEnd();
    for ( ; kit != kend; ++kit ) {
        if (!mimeTypeList.contains((*kit).mimetype()))
            mimeTypeList << (*kit).mimetype();
    }

    QString constraint = traderConstraint;
    QString subConstraint = " and '%1' in ServiceTypes";

    QStringList::ConstIterator it = mimeTypeList.begin();
    QStringList::ConstIterator end = mimeTypeList.end();
    Q_ASSERT( it != end );
    QString firstMimeType = *it;
    ++it;
    for ( ; it != end ; ++it ) {
        constraint += subConstraint.arg( *it );
    }

    const KService::List offers = KMimeTypeTrader::self()->query( firstMimeType, "Application", constraint );

    //// Ok, we have everything, now insert

    const KFileItem firstItem = items.first();
    const bool isLocal = firstItem.url().isLocalFile();
    // "Open With..." for folders is really not very useful, especially for remote folders.
    // (media:/something, or trash:/, or ftp://...)
    if ( !d->m_info.isDirectory() || isLocal ) {
        if ( !topMenu->actions().isEmpty() )
            topMenu->addSeparator();

        if ( !offers.isEmpty() ) {
            QMenu* menu = topMenu;

            if ( offers.count() > 1 ) { // submenu 'open with'
                menu = new QMenu(i18n("&Open With"), topMenu);
                menu->menuAction()->setObjectName("openWith_submenu"); // for the unittest
                topMenu->addMenu(menu);
            }
            //kDebug() << offers.count() << "offers" << topMenu << menu;

            KService::List::ConstIterator it = offers.begin();
            for( ; it != offers.end(); it++ ) {
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

                QString actionName(service->name().replace('&', "&&"));
                if (menu == topMenu) // no submenu -> prefix single offer
                    actionName = i18n("Open with %1", actionName);

                KAction* act = d->m_ownActions.addAction("openwith");
                act->setIcon(KIcon(service->icon()));
                act->setText(actionName);
                act->setData(QVariant::fromValue(service));
                d->m_runApplicationActionGroup.addAction(act);
                menu->addAction(act);
            }

            QString openWithActionName;
            if ( menu != topMenu ) { // submenu
                menu->addSeparator();
                openWithActionName = i18n("&Other...");
            } else {
                openWithActionName = i18n("&Open With...");
            }
            QAction *openWithAct = d->m_ownActions.addAction( "openwith_browse" );
            openWithAct->setText( openWithActionName );
            QObject::connect(openWithAct, SIGNAL(triggered()), d, SLOT(slotOpenWithDialog()));
            menu->addAction(openWithAct);
        }
        else // no app offers -> Open With...
        {
            KAction* act = d->m_ownActions.addAction( "openwith_browse" );
            act->setText( i18n( "&Open With..." ) );
            QObject::connect(act, SIGNAL(triggered()), d, SLOT(slotOpenWithDialog()));
            topMenu->addAction(act);
        }

    }
}

void KonqMenuActionsPrivate::slotRunApplication(QAction* act)
{
    // Is it an application, from one of the "Open With" actions
    KService::Ptr app = act->data().value<KService::Ptr>();
    Q_ASSERT(app);
    if (app) {
        KRun::run(*app, m_info.urlList(), m_info.parentWidget());
    }
}

void KonqMenuActionsPrivate::slotOpenWithDialog()
{
    KRun::displayOpenWithDialog(m_info.urlList(), m_info.parentWidget());
}
