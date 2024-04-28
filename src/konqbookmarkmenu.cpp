/* This file is part of the KDE project
SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>
SPDX-FileCopyrightText: 2006 Daniel Teske <teske@squorn.de>

SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "konqbookmarkmenu.h"
#include "kbookmarkowner.h"
#include "kbookmarkaction.h"

#include <QMenu>
#include <QFile>

#include <kconfig.h>
#include <ksharedconfig.h>
#include <kconfiggroup.h>
#include <kactioncollection.h>
#include <kstringhandler.h>

#include "kbookmarkmanager.h"
#include "konqpixmapprovider.h"
#include "libkonq_utils.h"

using namespace Konqueror;

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

void KonqBookmarkMenu::refill()
{
    if (isRoot()) {
        addActions();
    }
    fillBookmarks();
    if (!isRoot()) {
        addActions();
    }
    startFillingFavicons();
}

void Konqueror::KonqBookmarkMenu::startFillingFavicons()
{
    QList<QUrl> urls;
    for (auto a : m_actions) {
        if (!a->isSeparator() && !a->menu()) {
            urls.append(a->data().toUrl());
        }
    }
    connect(KonqPixmapProvider::self(), &KonqPixmapProvider::changed, this, &Konqueror::KonqBookmarkMenu::fillFavicons);
    KonqPixmapProvider::self()->downloadHostIcons(urls);
}

void Konqueror::KonqBookmarkMenu::fillFavicons()
{
    disconnect(KonqPixmapProvider::self(), &KonqPixmapProvider::changed, this, &Konqueror::KonqBookmarkMenu::fillFavicons);
    for (auto a : m_actions) {
        if (!a->isSeparator() && !a->menu()) {
            QUrl u = a->data().toUrl();
            a->data().clear(); //Don't waste memory, as we won't need the URLs any longer
            QString host = u.host();
            a->setIcon(KonqPixmapProvider::self()->iconForUrl(host));
            connect(KonqPixmapProvider::self(), &KonqPixmapProvider::changed, a, [host, a]() {
                a->setIcon(KonqPixmapProvider::self()->iconForUrl(host));
            });
        }
    }
}

QAction *KonqBookmarkMenu::actionForBookmark(const KBookmark &_bm)
{
    //We need a non-const copy of it
    KBookmark bm(_bm);
    if (bm.isGroup()) {
        // qCDebug(KBOOKMARKS_LOG) << "Creating Konq bookmark submenu named " << bm.text();
        KBookmarkActionMenu *actionMenu = new KBookmarkActionMenu(bm, this);
        m_actions.append(actionMenu);

        KBookmarkMenu *subMenu = new KonqBookmarkMenu(manager(), owner(), actionMenu, bm.address());

        m_lstSubMenus.append(subMenu);
        return actionMenu;
    } else if (bm.isSeparator()) {
        return KBookmarkMenu::actionForBookmark(bm);
    } else {
        // qCDebug(KBOOKMARKS_LOG) << "Creating Konq bookmark action named " << bm.text();
        QUrl host = bm.url().adjusted(QUrl::RemovePath | QUrl::RemoveQuery);
            bm.setIcon(KonqPixmapProvider::self()->iconNameFor(host));
        KBookmarkAction *action = new KBookmarkAction(bm, owner(), this);
        action->setData(bm.url());
        m_actions.append(action);
        return action;
    }
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
