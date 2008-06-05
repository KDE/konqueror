/* This file is part of the KDE project
   Copyright (C) 1998-2008 David Faure <faure@kde.org>
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
#include "konq_popupmenuplugin.h"
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

#include <QFileInfo>

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
 Then the same after uninstalling kdeaddons/konq-plugins (arkplugin in particular)
*/

class KonqPopupMenuPrivate
{
public:
    KonqPopupMenuPrivate(KonqPopupMenu* qq, KActionCollection & actions)
        : q(qq),
          m_itemFlags(KParts::BrowserExtension::DefaultPopupItems),
          m_actions(actions),
          m_ownActions(static_cast<QWidget *>(0))
    {
    }
    void addNamedAction(const QString& name);
    void addGroup(const QString& name);
    void addPlugins();
    void init(KonqPopupMenu::Flags kpf, KParts::BrowserExtension::PopupFlags itemFlags);

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
    KParts::BrowserExtension::PopupFlags m_itemFlags;
    KNewMenu *m_pMenuNew;
    KUrl m_sViewURL;
    KonqPopupMenuInformation m_popupMenuInfo;
    KonqMenuActions m_menuActions;
    KonqCopyToMenu m_copyToMenu;
    KBookmarkManager* m_bookmarkManager;
    KActionCollection &m_actions;
    KActionCollection m_ownActions; // TODO connect to statusbar for help on actions
    KParts::BrowserExtension::ActionGroupMap m_actionGroups;
};

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
  : QMenu(parentWidget),
    d(new KonqPopupMenuPrivate(this, actions))
{
    d->m_actionGroups = actionGroups;
    d->m_pMenuNew = newMenu;
    d->m_sViewURL = viewURL;
    d->m_bookmarkManager = mgr;
    d->m_popupMenuInfo.setItems(items);
    d->m_popupMenuInfo.setParentWidget(parentWidget);
    d->init(kpf, flags);
}

void KonqPopupMenuPrivate::addNamedAction(const QString& name)
{
    QAction* act = m_actions.action(name);
    if (act)
        q->addAction(act);
}

void KonqPopupMenuPrivate::init(KonqPopupMenu::Flags kpf, KParts::BrowserExtension::PopupFlags flags)
{
    m_ownActions.setObjectName("KonqPopupMenu::m_ownActions");
    m_itemFlags = flags;
    q->setFont(KGlobalSettings::menuFont());

    Q_ASSERT(m_popupMenuInfo.items().count() >= 1);

    bool bTrashIncluded = false;

    KFileItemList lstItems = m_popupMenuInfo.items();
    KFileItemList::const_iterator it = lstItems.begin();
    const KFileItemList::const_iterator kend = lstItems.end();
    for ( ; it != kend; ++it )
    {
        const KUrl url = (*it).url();
        if ( !bTrashIncluded && (
             ( url.protocol() == "trash" && url.path().length() <= 1 ) ) ) {
            bTrashIncluded = true;
        }
    }

    const bool isDirectory = m_popupMenuInfo.isDirectory();
    const bool sReading = m_popupMenuInfo.capabilities().supportsReading();
    bool sDeleting = (m_itemFlags & KParts::BrowserExtension::NoDeletion) == 0
                     && m_popupMenuInfo.capabilities().supportsDeleting();
    const bool sWriting = m_popupMenuInfo.capabilities().supportsWriting();
    const bool sMoving = sDeleting && m_popupMenuInfo.capabilities().supportsMoving();
    const bool isLocal = m_popupMenuInfo.capabilities().isLocal();

    KUrl url = m_sViewURL;
    url.cleanPath();

    bool isTrashLink     = false;
    bool isCurrentTrash = false;
    bool currentDir     = false;

    //check if url is current directory
    if ( m_popupMenuInfo.items().count() == 1 )
    {
        KUrl firstPopupURL( m_popupMenuInfo.items().first().url() );
        firstPopupURL.cleanPath();
        //kDebug(1203) << "View path is " << url.url();
        //kDebug(1203) << "First popup path is " << firstPopupURL.url();
        currentDir = firstPopupURL.equals( url, KUrl::CompareWithoutTrailingSlash );
        if ( isLocal && m_popupMenuInfo.mimeType() == "application/x-desktop" ) {
            KDesktopFile desktopFile( firstPopupURL.path() );
            const KConfigGroup cfg = desktopFile.desktopGroup();
            isTrashLink = ( cfg.readEntry("Type") == "Link" && cfg.readEntry("URL") == "trash:/" );
        }

        if (isTrashLink) {
            sDeleting = false;
        }

        // isCurrentTrash: popup on trash:/ itself, or on the trash.desktop link
        isCurrentTrash = (firstPopupURL.protocol() == "trash" && firstPopupURL.path().length() <= 1)
                         || isTrashLink;
    }

    const bool isIntoTrash = (url.protocol() == "trash") && !isCurrentTrash; // trashed file, not trash:/ itself

    const bool bIsLink  = (m_itemFlags & KParts::BrowserExtension::IsLink);

    //kDebug() << "isLocal=" << isLocal << " url=" << url << " isCurrentTrash=" << isCurrentTrash << " isIntoTrash=" << isIntoTrash << " bTrashIncluded=" << bTrashIncluded;

    //////////////////////////////////////////////////////////////////////////

    addGroup( "topactions" ); // used e.g. for ShowMenuBar. includes a separator at the end

    QAction * act;

    QAction *actNewWindow = 0;

#if 0 // TODO in the desktop code itself.
    if (( flags & KParts::BrowserExtension::ShowProperties ) && isOnDesktop &&
        !KAuthorized::authorizeKAction("editable_desktop_icons"))
    {
        flags &= ~KParts::BrowserExtension::ShowProperties; // remove flag
    }
#endif

    // Either 'newview' is in the actions we're given (probably in the tabhandling group)
    // or we need to insert it ourselves (e.g. for the desktop).
    // In the first case, actNewWindow must remain 0.
    if ( ((kpf & KonqPopupMenu::ShowNewWindow) != 0) && sReading )
    {
        const QString openStr = i18n("&Open");
        actNewWindow = m_ownActions.addAction( "newview" );
        actNewWindow->setIcon( KIcon("window-new") );
        actNewWindow->setText( openStr );
        QObject::connect(actNewWindow, SIGNAL(triggered()), q, SLOT(slotPopupNewView()));
    }

    if ( isDirectory && sWriting && !isCurrentTrash ) // A dir, and we can create things into it
    {
        if ( currentDir && m_pMenuNew ) // Current dir -> add the "new" menu
        {
            // As requested by KNewMenu :
            m_pMenuNew->slotCheckUpToDate();
            m_pMenuNew->setPopupFiles(m_popupMenuInfo.urlList());

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
        if (m_popupMenuInfo.items().count() > 1)
            act->setEnabled(false);
        if (KAuthorized::authorizeKAction("bookmarks"))
            q->addAction( act );
        if (bIsLink)
            addGroup( "linkactions" ); // see khtml
    }

    // "Open With" actions

    m_menuActions.setPopupMenuInfo(m_popupMenuInfo);

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
        m_copyToMenu.setItems(m_popupMenuInfo.items());
        m_copyToMenu.setReadOnly(sMoving == false);
        m_copyToMenu.addActionsTo(q);
        q->addSeparator();
    }

    if ( !isCurrentTrash && !isIntoTrash && sReading )
        addPlugins(); // now it's time to add plugins

    if ( (m_itemFlags & KParts::BrowserExtension::ShowProperties) && KPropertiesDialog::canDisplay( m_popupMenuInfo.items() ) ) {
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
    KPropertiesDialog* dlg = new KPropertiesDialog( m_popupMenuInfo.items(), m_popupMenuInfo.parentWidget() );
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
    Q_FOREACH(const KUrl& url, m_popupMenuInfo.urlList()) {
        (void) new KRun(url, m_popupMenuInfo.parentWidget());
    }
}

void KonqPopupMenuPrivate::slotPopupNewDir()
{
  if (m_popupMenuInfo.urlList().empty())
    return;

  KonqOperations::newDir(m_popupMenuInfo.parentWidget(), m_popupMenuInfo.urlList().first());
}

void KonqPopupMenuPrivate::slotPopupEmptyTrashBin()
{
  KonqOperations::emptyTrash(m_popupMenuInfo.parentWidget());
}

void KonqPopupMenuPrivate::slotPopupRestoreTrashedItems()
{
  KonqOperations::restoreTrashedItems(m_popupMenuInfo.urlList(), m_popupMenuInfo.parentWidget());
}

void KonqPopupMenuPrivate::slotPopupAddToBookmark()
{
    KBookmarkGroup root;
    if (m_popupMenuInfo.urlList().count() == 1) {
        const KUrl url = m_popupMenuInfo.urlList().first();
        const QString title = m_urlTitle.isEmpty() ? url.prettyUrl() : m_urlTitle;
        KBookmarkDialog dlg(m_bookmarkManager, m_popupMenuInfo.parentWidget());
        dlg.addBookmark(title, url.url());
    }
    else
    {
        root = m_bookmarkManager->root();
        Q_FOREACH(const KUrl& url, m_popupMenuInfo.urlList()) {
            root.addBookmark(url.prettyUrl(), url);
        }
        m_bookmarkManager->emitChanged(root);
    }
}

void KonqPopupMenuPrivate::slotPopupMimeType()
{
    KonqOperations::editMimeType(m_popupMenuInfo.mimeType(), m_popupMenuInfo.parentWidget());
}

void KonqPopupMenuPrivate::slotPopupProperties()
{
    KPropertiesDialog::showDialog(m_popupMenuInfo.items(), m_popupMenuInfo.parentWidget());
}

void KonqPopupMenuPrivate::addGroup(const QString& name)
{
    QList<QAction *> actions = m_actionGroups.value(name);
    q->addActions(actions);
}

void KonqPopupMenuPrivate::addPlugins()
{
    const QString commonMimeType = m_popupMenuInfo.mimeType();
    const KService::List plugin_offers = KMimeTypeTrader::self()->query(commonMimeType.isEmpty() ? QLatin1String("application/octet-stream") : commonMimeType, "KonqPopupMenu/Plugin", "exist Library");

    KService::List::ConstIterator iterator = plugin_offers.begin();
    const KService::List::ConstIterator end = plugin_offers.end();
    for(; iterator != end; ++iterator) {
        //kDebug() << (*iterator)->name() << (*iterator)->library();
        KonqPopupMenuPlugin *plugin = (*iterator)->createInstance<KonqPopupMenuPlugin>(q);
        if (!plugin)
            continue;
        plugin->setup(&m_ownActions, m_popupMenuInfo, q);
    }
}

#include "konq_popupmenu.moc"
