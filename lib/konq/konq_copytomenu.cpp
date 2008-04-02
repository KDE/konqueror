/* This file is part of the KDE project

   Copyright 2008 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2 of the License or
   ( at your option ) version 3 or, at the discretion of KDE e.V.
   ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_copytomenu.h"
#include "konq_copytomenu_p.h"
#include "konq_operations.h"
#include <kfiledialog.h>
#include <kdebug.h>
#include <kaction.h>
#include <QDir>
#include <kicon.h>
#include <klocale.h>
#include <kmenu.h>

KonqCopyToMenuPrivate::KonqCopyToMenuPrivate()
    : m_urls(), m_readOnly(false)
{
}

////

KonqCopyToMenu::KonqCopyToMenu()
    : d(new KonqCopyToMenuPrivate)
{

}

KonqCopyToMenu::~KonqCopyToMenu()
{
    delete d;
}

void KonqCopyToMenu::setItems(const KFileItemList& items)
{
    // For now we lose all the information except for the urls
    // But this API is useful in case KIO can make use of this information later
    // (e.g. to avoid stat'ing the source urls)
    Q_FOREACH(const KFileItem& item, items)
        d->m_urls.append(item.url());
}

void KonqCopyToMenu::setUrls(const KUrl::List& urls)
{
    d->m_urls = urls;
}

void KonqCopyToMenu::setReadOnly(bool ro)
{
    d->m_readOnly = ro;
}

void KonqCopyToMenu::addActionsTo(QMenu* menu)
{
    KMenu* mainCopyMenu = new KonqCopyToMainMenu(menu, d, Copy);
    mainCopyMenu->setTitle(i18nc("@title:menu", "Copy To"));
    mainCopyMenu->menuAction()->setObjectName("copyTo_submenu"); // for the unittest
    menu->addMenu(mainCopyMenu);

    if (!d->m_readOnly) {
        KMenu* mainMoveMenu = new KonqCopyToMainMenu(menu, d, Move);
        mainMoveMenu->setTitle(i18nc("@title:menu", "Move To"));
        mainMoveMenu->menuAction()->setObjectName("moveTo_submenu"); // for the unittest
        menu->addMenu(mainMoveMenu);
    }
}

////

KonqCopyToMainMenu::KonqCopyToMainMenu(QMenu* parent, KonqCopyToMenuPrivate* _d, MenuType menuType)
    : KMenu(parent), m_menuType(menuType),
      m_actionGroup(static_cast<QWidget *>(0)),
      d(_d),
      m_recentDirsGroup(KGlobal::config(), m_menuType == Copy ? "kuick-copy" : "kuick-move")
{
    connect(this, SIGNAL(aboutToShow()), SLOT(slotAboutToShow()));
    connect(&m_actionGroup, SIGNAL(triggered(QAction*)), SLOT(slotTriggered(QAction*)));
}

void KonqCopyToMainMenu::slotAboutToShow()
{
    clear();
    KonqCopyToDirectoryMenu* subMenu;
    // Home Folder
    subMenu = new KonqCopyToDirectoryMenu(this, this, QDir::homePath());
    subMenu->setTitle(i18nc("@title:menu", "Home Folder"));
    subMenu->setIcon(KIcon("go-home"));
    addMenu(subMenu);

    // Root Folder
    // TODO on Windows: one submenu per drive? (Or even a Drives submenu with the drives in it?)
    subMenu = new KonqCopyToDirectoryMenu(this, this, QDir::rootPath());
    subMenu->setTitle(i18nc("@title:menu", "Root Folder"));
    subMenu->setIcon(KIcon("folder-red"));
    addMenu(subMenu);

    // Browse... action, shows a KFileDialog
    KAction* browseAction = new KAction(i18nc("@title:menu in Copy To or Move To submenu", "Browse..."), this);
    connect(browseAction, SIGNAL(triggered()), this, SLOT(slotBrowse()));
    addAction(browseAction);

    addSeparator(); // looks like Qt4 handles removing it automatically if it's last in the menu, nice.

    // Recent Destinations
    const QStringList recentDirs = m_recentDirsGroup.readPathEntry("Paths", QStringList());
    Q_FOREACH(const QString& recentDir, recentDirs) {
        KUrl url(recentDir);
        KAction* act = new KAction(url.pathOrUrl(), this);
        act->setData(url);
        m_actionGroup.addAction(act);
        addAction(act);
    }
}

void KonqCopyToMainMenu::slotBrowse()
{
    const KUrl dest = KFileDialog::getExistingDirectoryUrl(KUrl("kfiledialog:///copyto"), this);
    if (!dest.isEmpty()) {
        copyOrMoveTo(dest);
    }
}

void KonqCopyToMainMenu::slotTriggered(QAction* action)
{
    const KUrl url = action->data().value<KUrl>();
    Q_ASSERT(!url.isEmpty());
    copyOrMoveTo(url);
}

void KonqCopyToMainMenu::copyOrMoveTo(const KUrl& dest)
{
    // Insert into the recent destinations list
    QStringList recentDirs = m_recentDirsGroup.readPathEntry("Paths", QStringList());
    const QString niceDest = dest.pathOrUrl();
    if (!recentDirs.contains(niceDest)) { // don't change position if already there, moving stuff is bad usability
        recentDirs.prepend(niceDest);
        while (recentDirs.size() > 10) { // hardcoded max size
            recentDirs.removeLast();
        }
        m_recentDirsGroup.writePathEntry("Paths", recentDirs);
    }

    // And now let's do the copy or move -- with undo/redo support.
    KonqOperations::copy(this, m_menuType == Copy ? KonqOperations::COPY : KonqOperations::MOVE,
                         d->m_urls, dest);
}

////

KonqCopyToDirectoryMenu::KonqCopyToDirectoryMenu(QMenu* parent, KonqCopyToMainMenu* mainMenu, const QString& path)
    : KMenu(parent), m_mainMenu(mainMenu), m_path(path)
{
    connect(this, SIGNAL(aboutToShow()), SLOT(slotAboutToShow()));
}

void KonqCopyToDirectoryMenu::slotAboutToShow()
{
    clear();
    KAction* act = new KAction(m_mainMenu->menuType() == Copy
                               ? i18nc("@title:menu", "Copy Here")
                               : i18nc("@title:menu", "Move Here"), this);
    act->setData(KUrl(m_path));
    act->setEnabled(QFileInfo(m_path).isWritable());
    m_mainMenu->actionGroup().addAction(act);
    addAction(act);

    addSeparator(); // looks like Qt4 handles removing it automatically if it's last in the menu, nice.

    // List directory
    // All we need is sub folder names, their permissions, their icon.
    // KDirLister or KIO::listDir would fetch much more info, and would be async,
    // and we only care about local directories so we use QDir directly.
    QDir dir(m_path);
    const QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::LocaleAware);
    KMimeType::Ptr dirMime = KMimeType::mimeType("inode/directory");
    Q_FOREACH(const QString& subDir, entries) {
        const QString subPath = m_path + '/' + subDir;
        KonqCopyToDirectoryMenu* subMenu = new KonqCopyToDirectoryMenu(this, m_mainMenu, subPath);
        subMenu->setTitle(subDir);
        const QString iconName = dirMime->iconName(KUrl(subPath));
        subMenu->setIcon(KIcon(iconName));
        if (QFileInfo(subPath).isSymLink()) { // I hope this isn't too slow...
            QFont font = subMenu->menuAction()->font();
            font.setItalic(true);
            subMenu->menuAction()->setFont(font);
        }
        addMenu(subMenu);
    }
}
