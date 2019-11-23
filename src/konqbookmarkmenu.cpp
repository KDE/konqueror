/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright (C) 2006 Daniel Teske <teske@squorn.de>

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

#include "konqbookmarkmenu.h"
#include "kbookmarkowner.h"
#include "kbookmarkaction.h"
#include "kbookmarkcontextmenu.h"

#include <QMenu>
#include <QFile>

#include <kconfig.h>
#include <ksharedconfig.h>
#include <kconfiggroup.h>
#include <kactioncollection.h>
#include <kstringhandler.h>

#include "kbookmarkimporter.h"
#include "kbookmarkimporter_opera.h"
#include "kbookmarkimporter_ie.h"
#include "kbookmarkmanager.h"
#include "konqbookmarkmenu_p.h"

using namespace Konqueror;

KImportedBookmarkMenu::KImportedBookmarkMenu(KBookmarkManager *mgr,
        KBookmarkOwner *owner, QMenu *parentMenu,
        const QString &type, const QString &location)
    : KBookmarkMenu(mgr, owner, parentMenu, QString()), m_type(type), m_location(location)
{
    connect(parentMenu, &QMenu::aboutToShow, this, &KImportedBookmarkMenu::slotNSLoad);
}

KImportedBookmarkMenu::KImportedBookmarkMenu(KBookmarkManager *mgr,
        KBookmarkOwner *owner, QMenu *parentMenu)
    : KBookmarkMenu(mgr, owner, parentMenu, QString()), m_type(QString()), m_location(QString())
{
}

KImportedBookmarkMenu::~KImportedBookmarkMenu()
{
}

void KImportedBookmarkMenu::refill()
{
}

void KImportedBookmarkMenu::clear()
{
}

void KImportedBookmarkMenu::slotNSLoad()
{
    // qCDebug(KBOOKMARKS_LOG)<<"**** slotNSLoad  ****"<<m_type<<"  "<<m_location;
    // only fill menu once
    disconnect(parentMenu(), &QMenu::aboutToShow, nullptr, nullptr);

    // not NSImporter, but kept old name for BC reasons
    KBookmarkMenuImporter importer(manager(), this);
    importer.openBookmarks(m_location, m_type);
}

/********************************************************************/

void KBookmarkMenuImporter::openBookmarks(const QString &location, const QString &type)
{
    mstack.push(m_menu);

    KBookmarkImporterBase *importer = KBookmarkImporterBase::factory(type);
    if (!importer) {
        return;
    }
    importer->setFilename(location);
    connectToImporter(*importer);
    importer->parse();

    delete importer;
}

void KBookmarkMenuImporter::connectToImporter(const QObject &importer)
{
    connect(&importer, SIGNAL(newBookmark(QString,QString,QString)),
            SLOT(newBookmark(QString,QString,QString)));
    connect(&importer, SIGNAL(newFolder(QString,bool,QString)),
            SLOT(newFolder(QString,bool,QString)));
    connect(&importer, SIGNAL(newSeparator()), SLOT(newSeparator()));
    connect(&importer, SIGNAL(endFolder()), SLOT(endFolder()));
}

void KBookmarkMenuImporter::newBookmark(const QString &text, const QString &url, const QString &)
{
    KBookmark bm = KBookmark::standaloneBookmark(text, QUrl(url), QStringLiteral("html"));
    QAction *action = new KBookmarkAction(bm, mstack.top()->owner(), this);
    mstack.top()->parentMenu()->addAction(action);
    mstack.top()->m_actions.append(action);
}

void KBookmarkMenuImporter::newFolder(const QString &text, bool, const QString &)
{
    QString _text = KStringHandler::csqueeze(text).replace(QLatin1Char('&'), QLatin1String("&&"));
    KActionMenu *actionMenu = new KImportedBookmarkActionMenu(QIcon::fromTheme(QStringLiteral("folder")), _text, this);
    mstack.top()->parentMenu()->addAction(actionMenu);
    mstack.top()->m_actions.append(actionMenu);
    KImportedBookmarkMenu *subMenu = new KImportedBookmarkMenu(m_pManager, m_menu->owner(), actionMenu->menu());
    mstack.top()->m_lstSubMenus.append(subMenu);

    mstack.push(subMenu);
}

void KBookmarkMenuImporter::newSeparator()
{
    mstack.top()->parentMenu()->addSeparator();
}

void KBookmarkMenuImporter::endFolder()
{
    mstack.pop();
}

/********************************************************************/

KonqBookmarkContextMenu::KonqBookmarkContextMenu(const KBookmark &bm, KBookmarkManager *mgr, KBookmarkOwner *owner)
    : KBookmarkContextMenu(bm, mgr, owner)
{
}

KonqBookmarkContextMenu::~KonqBookmarkContextMenu()
{
}

void KonqBookmarkContextMenu::addActions()
{
    KConfigGroup config = KSharedConfig::openConfig(QStringLiteral("kbookmarkrc"), KConfig::NoGlobals)->group("Bookmarks");
    bool filteredToolbar = config.readEntry("FilteredToolbar", false);

    if (bookmark().isGroup()) {
        addOpenFolderInTabs();
        addBookmark();

        if (filteredToolbar) {
            QString text = bookmark().showInToolbar() ? tr("Hide in toolbar") : tr("Show in toolbar");
            addAction(text, this, &KonqBookmarkContextMenu::toggleShowInToolbar);
        }

        addFolderActions();
    } else {
        if (owner()) {
            addAction(QIcon::fromTheme(QStringLiteral("window-new")), tr("Open in New Window"), this, &KonqBookmarkContextMenu::openInNewWindow);
            addAction(QIcon::fromTheme(QStringLiteral("tab-new")), tr("Open in New Tab"), this, &KonqBookmarkContextMenu::openInNewTab);
        }
        addBookmark();

        if (filteredToolbar) {
            QString text = bookmark().showInToolbar() ? tr("Hide in toolbar") : tr("Show in toolbar");
            addAction(text, this, &KonqBookmarkContextMenu::toggleShowInToolbar);
        }

        addBookmarkActions();
    }
}

void KonqBookmarkContextMenu::toggleShowInToolbar()
{
    bookmark().setShowInToolbar(!bookmark().showInToolbar());
    manager()->emitChanged(bookmark().parentGroup());
}

void KonqBookmarkContextMenu::openInNewTab()
{
    owner()->openInNewTab(bookmark());
}

void KonqBookmarkContextMenu::openInNewWindow()
{
    owner()->openInNewWindow(bookmark());
}

/******************************/
/******************************/
/******************************/

void KonqBookmarkMenu::fillDynamicBookmarks()
{
    if (isDirty()
            && KBookmarkManager::userBookmarksManager()->path() == manager()->path()) {
        bool haveSep = false;

        const QStringList keys = KonqBookmarkMenu::dynamicBookmarksList();
        for (QStringList::const_iterator it = keys.begin(); it != keys.end(); ++it) {
            DynMenuInfo info;
            info = showDynamicBookmarks((*it));

            if (!info.show || !QFile::exists(info.location)) {
                continue;
            }

            if (!haveSep) {
                parentMenu()->addSeparator();
                haveSep = true;
            }

            KActionMenu *actionMenu;
            actionMenu = new KActionMenu(QIcon::fromTheme(info.type), info.name, this);
            m_actionCollection->addAction(QStringLiteral("kbookmarkmenu"), actionMenu);

            parentMenu()->addAction(actionMenu);
            m_actions.append(actionMenu);

            KImportedBookmarkMenu *subMenu =
                new KImportedBookmarkMenu(manager(), owner(), actionMenu->menu(),
                                          info.type, info.location);
            m_lstSubMenus.append(subMenu);
        }
    }
}

void KonqBookmarkMenu::refill()
{
    if (isRoot()) {
        addActions();
    }
    fillDynamicBookmarks();
    fillBookmarks();
    if (!isRoot()) {
        addActions();
    }
}

QAction *KonqBookmarkMenu::actionForBookmark(const KBookmark &bm)
{
    if (bm.isGroup()) {
        // qCDebug(KBOOKMARKS_LOG) << "Creating Konq bookmark submenu named " << bm.text();
        KBookmarkActionMenu *actionMenu = new KBookmarkActionMenu(bm, this);
        m_actionCollection->addAction(QStringLiteral("kbookmarkmenu"), actionMenu);
        m_actions.append(actionMenu);

        KBookmarkMenu *subMenu = new KonqBookmarkMenu(manager(), owner(), actionMenu, bm.address());

        m_lstSubMenus.append(subMenu);
        return actionMenu;
    } else if (bm.isSeparator()) {
        return KBookmarkMenu::actionForBookmark(bm);
    } else {
        // qCDebug(KBOOKMARKS_LOG) << "Creating Konq bookmark action named " << bm.text();
        KBookmarkAction *action = new KBookmarkAction(bm, owner(), this);
        m_actionCollection->addAction(action->objectName(), action);
        m_actions.append(action);
        return action;
    }
}

KonqBookmarkMenu::DynMenuInfo KonqBookmarkMenu::showDynamicBookmarks(const QString &id)
{
    KConfig bookmarkrc(QStringLiteral("kbookmarkrc"), KConfig::NoGlobals);
    KConfigGroup config(&bookmarkrc, "Bookmarks");

    DynMenuInfo info;
    info.show = false;
    info.d = nullptr;

    if (!config.hasKey("DynamicMenus")) {
        const QString dynamicMenuGroupId = QLatin1String("DynamicMenu-") + id;
        if (bookmarkrc.hasGroup(dynamicMenuGroupId)) {
            KConfigGroup dynGroup(&bookmarkrc, dynamicMenuGroupId);
            info.show = dynGroup.readEntry("Show", false);
            info.location = dynGroup.readPathEntry("Location", QString());
            info.type = dynGroup.readEntry("Type");
            info.name = dynGroup.readEntry("Name");
        }
    }
    return info;
}

QStringList KonqBookmarkMenu::dynamicBookmarksList()
{
    KConfigGroup config = KSharedConfig::openConfig(QStringLiteral("kbookmarkrc"), KConfig::NoGlobals)->group("Bookmarks");

    QStringList mlist;
    if (config.hasKey("DynamicMenus")) {
        mlist = config.readEntry("DynamicMenus", QStringList());
    }

    return mlist;
}

void KonqBookmarkMenu::setDynamicBookmarks(const QString &id, const DynMenuInfo &newMenu)
{
    KSharedConfig::Ptr kbookmarkrc = KSharedConfig::openConfig(QStringLiteral("kbookmarkrc"), KConfig::NoGlobals);
    KConfigGroup dynConfig = kbookmarkrc->group(QLatin1String("DynamicMenu-") + id);

    // add group unconditionally
    dynConfig.writeEntry("Show", newMenu.show);
    dynConfig.writePathEntry("Location", newMenu.location);
    dynConfig.writeEntry("Type", newMenu.type);
    dynConfig.writeEntry("Name", newMenu.name);

    QStringList elist;
    KConfigGroup config = kbookmarkrc->group("Bookmarks");
    if (config.hasKey("DynamicMenus")) {
        elist = config.readEntry("DynamicMenus", QStringList());
    }

    // make sure list includes type
    if (!elist.contains(id)) {
        elist << id;
        config.writeEntry("DynamicMenus", elist);
    }

    config.sync();
}

QMenu *KonqBookmarkMenu::contextMenu(QAction *action)
{
    KBookmarkActionInterface *act = dynamic_cast<KBookmarkActionInterface *>(action);
    if (!act) {
        return nullptr;
    }
    return new KonqBookmarkContextMenu(act->bookmark(), manager(), owner());
}

#include "moc_konqbookmarkmenu.cpp"
