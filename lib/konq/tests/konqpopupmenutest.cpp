/* This file is part of KDE
    Copyright (c) 2007 David Faure <faure@kde.org>

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

#include "konqpopupmenutest.h"
#include <kbookmarkmanager.h>
#include <assert.h>
#include "qtest_kde.h"
#include <QDir>
#include <kparts/browserextension.h>
#include <knewmenu.h>
#include <kdebug.h>
#include <konq_popupmenu.h>

QTEST_KDEMAIN(KonqPopupMenuTest, GUI)

KonqPopupMenuTest::KonqPopupMenuTest()
    : m_actionCollection(this)
{
}

static QStringList extractActionNames(const QMenu& menu)
{
    QStringList ret;
    foreach (const QAction* action, menu.actions()) {
        if (action->isSeparator()) {
            ret.append("separator");
        } else if (action->objectName().isEmpty()) {
            Q_ASSERT(action->menu());
            ret.append("submenu");
        } else {
            ret.append(action->objectName());
        }
    }
    return ret;

}

void KonqPopupMenuTest::initTestCase()
{
    m_rootItem = KFileItem(QDir::currentPath(), "inode/directory", S_IFDIR + 0777);
    m_fileItem = KFileItem(QDir::currentPath() + "/Makefile", "text/x-makefile", S_IFREG + 0660);
    m_linkItem = KFileItem(QDir::currentPath() + "/cmake_install.cmake", "text/html", S_IFREG + 0660);
    m_subDirItem = KFileItem(QDir::currentPath() + "/CMakeFiles", "inode/directory", S_IFDIR + 0755);
    m_cut = KStandardAction::cut(0, 0, this);
    m_actionCollection.addAction("cut", m_cut);
    m_copy = KStandardAction::copy(0, 0, this);
    m_actionCollection.addAction("copy", m_copy);
    m_paste = KStandardAction::paste(0, 0, this);
    m_actionCollection.addAction("paste", m_paste);
    m_pasteTo = KStandardAction::paste(0, 0, this);
    m_actionCollection.addAction("pasteto", m_pasteTo);
    m_back = new QAction(this);
    m_actionCollection.addAction("back", m_back);
    m_forward = new QAction(this);
    m_actionCollection.addAction("forward", m_forward);
    m_up = new QAction(this);
    m_actionCollection.addAction("up", m_up);
    m_reload = new QAction(this);
    m_actionCollection.addAction("reload", m_reload);
    m_properties = new QAction(this);
    m_actionCollection.addAction("properties", m_properties);
    m_rename = new QAction(this);
    m_actionCollection.addAction("rename", m_rename);
    m_trash = new QAction(this);
    m_actionCollection.addAction("trash", m_trash);

    m_tabHandlingActions = new QActionGroup(this);
    m_newWindow = new QAction(m_tabHandlingActions);
    m_actionCollection.addAction("openInNewWindow", m_newWindow);
    m_newTab = new QAction(m_tabHandlingActions);
    m_actionCollection.addAction("openInNewTab", m_newTab);
    QAction* separator = new QAction(m_tabHandlingActions);
    separator->setSeparator(true);
    QCOMPARE(m_tabHandlingActions->actions().count(), 3);

    m_previewActions = new QActionGroup(this);
    m_preview1 = new QAction(m_previewActions);
    m_actionCollection.addAction("preview1", m_preview1);
    m_preview2 = new QAction(m_previewActions);
    m_actionCollection.addAction("preview2", m_preview2);

    m_editActions = new QActionGroup(this);
    // TODO use it, see khtml_popupmenu.rc

    m_linkActions = new QActionGroup(this);
    QAction* saveLinkAs = new QAction(m_linkActions);
    m_actionCollection.addAction("savelinkas", saveLinkAs);
    QAction* copyLinkLocation = new QAction(m_linkActions);
    m_actionCollection.addAction("copylinklocation", copyLinkLocation);
    // TODO there's a whole bunch of things for frames, and for images, see khtml_popupmenu.rc

    m_partActions = new QActionGroup(this);
    separator = new QAction(m_partActions);
    separator->setSeparator(true);
    m_partActions->addAction(separator); // we better start with a separator
    QAction* viewDocumentSource = new QAction(m_partActions);
    m_actionCollection.addAction("viewDocumentSource", viewDocumentSource);

    m_newMenu = new KNewMenu(&m_actionCollection, 0, "newmenu");

    // Check if extractActionNames works
    QMenu popup;
    popup.addAction(m_back);
    QMenu* subMenu = new QMenu(&popup);
    popup.addMenu(subMenu);
    subMenu->addAction(m_up);
    QStringList actions = extractActionNames(popup);
    kDebug() << actions;
    QCOMPARE(actions, QStringList() << "back" << "submenu");
}

void KonqPopupMenuTest::testFile()
{
    KFileItemList itemList;
    itemList << m_fileItem;
    KUrl viewUrl = QDir::currentPath();
    KonqPopupMenu::Flags flags = 0;
    KParts::BrowserExtension::PopupFlags beflags = KParts::BrowserExtension::ShowProperties
                                                   | KParts::BrowserExtension::ShowReload
                                                   | KParts::BrowserExtension::ShowUrlOperations;
    KParts::BrowserExtension::ActionGroupMap actionGroups;
    actionGroups.insert("tabhandling", m_tabHandlingActions->actions());
    actionGroups.insert("preview", QList<QAction *>() << m_preview1);

    KonqPopupMenu popup(itemList, viewUrl, m_actionCollection, m_newMenu, flags, beflags,
                        0 /*parent*/, 0 /*bookmark manager*/, actionGroups);

    QStringList actions = extractActionNames(popup);
    kDebug() << actions;
    QCOMPARE(actions, QStringList()
             << "openInNewWindow" << "openInNewTab" << "separator"
             << "cut" << "copy" << "rename" << "trash" << "separator"
             << "openWith_submenu"
             << "preview1"
             << "actions_submenu" << "separator"
             // (came from arkplugin) << "compress"
             // (came from kuick) << "copy_to" << "move_to"
             << "properties");
}

void KonqPopupMenuTest::testFilePreviewSubMenu()
{
    KFileItemList itemList;
    itemList << m_fileItem;
    KUrl viewUrl = QDir::currentPath();
    KonqPopupMenu::Flags flags = 0;
    KParts::BrowserExtension::PopupFlags beflags = KParts::BrowserExtension::ShowProperties
                                                   | KParts::BrowserExtension::ShowReload
                                                   | KParts::BrowserExtension::ShowUrlOperations;
    KParts::BrowserExtension::ActionGroupMap actionGroups;
    actionGroups.insert("tabhandling", m_tabHandlingActions->actions());
    actionGroups.insert("preview", m_previewActions->actions());

    KonqPopupMenu popup(itemList, viewUrl, m_actionCollection, m_newMenu, flags, beflags,
                        0 /*parent*/, 0 /*bookmark manager*/, actionGroups);

    QStringList actions = extractActionNames(popup);
    kDebug() << actions;
    QCOMPARE(actions, QStringList()
             << "openInNewWindow" << "openInNewTab" << "separator"
             << "cut" << "copy" << "rename" << "trash" << "separator"
             << "openWith_submenu"
             << "preview_submenu"
             << "actions_submenu" << "separator"
             // (came from arkplugin) << "compress"
             // (came from kuick) << "copy_to" << "move_to"
             << "properties");
}

void KonqPopupMenuTest::testSubDirectory()
{
    KFileItemList itemList;
    itemList << m_subDirItem;
    KUrl viewUrl = QDir::currentPath();
    KonqPopupMenu::Flags flags = 0;
    KParts::BrowserExtension::PopupFlags beflags = KParts::BrowserExtension::ShowProperties
                                                   | KParts::BrowserExtension::ShowUrlOperations;
    KParts::BrowserExtension::ActionGroupMap actionGroups;
    actionGroups.insert("tabhandling", m_tabHandlingActions->actions());
    actionGroups.insert("preview", m_previewActions->actions());

    KonqPopupMenu popup(itemList, viewUrl, m_actionCollection, m_newMenu, flags, beflags,
                        0 /*parent*/, 0 /*bookmark manager*/, actionGroups);
    QStringList actions = extractActionNames(popup);
    kDebug() << actions;
    // It's OK to have "openwith" instead of openWith_submenu, it depends on the number of offers
    actions.replaceInStrings("openwith", "openWith_submenu");
    QCOMPARE(actions, QStringList()
             << "openInNewWindow" << "openInNewTab" << "separator"
             << "cut" << "copy" << "pasteto" << "rename" << "trash" << "separator"
             << "openWith_submenu"
             << "preview_submenu"
             << "actions_submenu" << "separator"
             // (came from arkplugin) << "compress"
             // (came from kuick) << "copy_to" << "move_to"
             << "properties");
}

void KonqPopupMenuTest::testViewDirectory()
{
    KFileItemList itemList;
    itemList << m_rootItem;
    KUrl viewUrl = QDir::currentPath();
    KonqPopupMenu::Flags flags = 0;
    KParts::BrowserExtension::PopupFlags beflags =
        KParts::BrowserExtension::ShowNavigationItems |
        KParts::BrowserExtension::ShowUp |
        KParts::BrowserExtension::ShowCreateDirectory |
        KParts::BrowserExtension::ShowUrlOperations |
        KParts::BrowserExtension::ShowProperties;
    // KonqMainWindow says: doTabHandling = !openedForViewURL && ... So we don't add tabhandling here
    KParts::BrowserExtension::ActionGroupMap actionGroups;
    actionGroups.insert("preview", m_previewActions->actions());

    KonqPopupMenu popup(itemList, viewUrl, m_actionCollection, m_newMenu, flags, beflags,
                        0 /*parent*/, 0 /*bookmark manager*/, actionGroups);

    QStringList actions = extractActionNames(popup);
    qDebug() << actions;
    // It's OK to have "openwith" instead of openWith_submenu, it depends on the number of offers
    actions.replaceInStrings("openwith", "openWith_submenu");
    QCOMPARE(actions, QStringList()
             << "newmenu" << "separator"
             << "up" << "back" << "forward" << "separator"
             << "paste" << "separator"
             << "openWith_submenu"
             << "preview_submenu"
             << "actions_submenu" << "separator"
             // (came from arkplugin) << "compress"
             // (came from kuick) << "copy_to" << "move_to"
             << "properties");
}

void KonqPopupMenuTest::testHtmlLink()
{
    KFileItemList itemList;
    itemList << m_linkItem;
    KUrl viewUrl = m_fileItem.url();
    KonqPopupMenu::Flags flags = 0;
    KParts::BrowserExtension::PopupFlags beflags = KParts::BrowserExtension::ShowBookmark
                                                   | KParts::BrowserExtension::ShowReload
                                                   | KParts::BrowserExtension::IsLink;
    KParts::BrowserExtension::ActionGroupMap actionGroups;
    actionGroups.insert("tabhandling", m_tabHandlingActions->actions());
    actionGroups.insert("preview", m_previewActions->actions());
    actionGroups.insert("editactions", m_editActions->actions());
    actionGroups.insert("linkactions", m_linkActions->actions());
    actionGroups.insert("partactions", m_partActions->actions());

    KonqPopupMenu popup(itemList, viewUrl, m_actionCollection, m_newMenu, flags, beflags,
                        0 /*parent*/, KBookmarkManager::userBookmarksManager(), actionGroups);

    QStringList actions = extractActionNames(popup);
    kDebug() << actions;
    QCOMPARE(actions, QStringList()
             << "openInNewWindow" << "openInNewTab" << "separator"
             << "bookmark_add" << "savelinkas" << "copylinklocation"
             << "separator"
             << "openWith_submenu"
             << "preview_submenu"
             << "actions_submenu" << "separator"
             // (came from kuick) << "copy_to" << "move_to"
             << "viewDocumentSource");
}

void KonqPopupMenuTest::testHtmlPage()
{
    KFileItemList itemList;
    itemList << m_fileItem;
    KUrl viewUrl = m_fileItem.url();
    KonqPopupMenu::Flags flags = 0;
    KParts::BrowserExtension::PopupFlags beflags = KParts::BrowserExtension::ShowBookmark
                                                   | KParts::BrowserExtension::ShowReload
                                                   | KParts::BrowserExtension::ShowNavigationItems;
    KParts::BrowserExtension::ActionGroupMap actionGroups;
    // KonqMainWindow says: doTabHandling = !openedForViewURL && ... So we don't add tabhandling here
    // TODO we could just move that logic to KonqPopupMenu...
    //actionGroups.insert("tabhandling", m_tabHandlingActions->actions());
    actionGroups.insert("preview", m_previewActions->actions());
    actionGroups.insert("editactions", m_editActions->actions());
    //actionGroups.insert("linkactions", m_linkActions->actions());
    QAction* security = new QAction(m_partActions);
    m_actionCollection.addAction("security", security);
    QAction* setEncoding = new QAction(m_partActions);
    m_actionCollection.addAction("setEncoding", setEncoding);
    actionGroups.insert("partactions", m_partActions->actions());

    KonqPopupMenu popup(itemList, viewUrl, m_actionCollection, m_newMenu, flags, beflags,
                        0 /*parent*/, KBookmarkManager::userBookmarksManager(), actionGroups);

    QStringList actions = extractActionNames(popup);
    kDebug() << actions;
    QCOMPARE(actions, QStringList()
             << "back" << "forward" << "reload" << "separator"
             << "bookmark_add"
             << "separator"
             << "openWith_submenu"
             << "preview_submenu"
             << "actions_submenu" << "separator"
             // << TODO "stopanimations"
             // (came from kuick - but shouldn't be there IMHO) << "copy_to" << "move_to"
             << "viewDocumentSource" << "security" << "setEncoding"
        );
}


// TODO test ShowReload (khtml passes it, but not the file views. Maybe show it if "not a directory" or "not local")

// (because file viewers don't react on changes, and remote things don't notify) -- then get rid of ShowReload.

// TODO test ShowBookmark. Probably the same logic?
// TODO separate filemanager and webbrowser bookmark managers, too (share file bookmarks with file dialog)

// TODO click on background of html page; text selection in khtml

// TODO trash:/ tests

// TODO test NoDeletion part flag
