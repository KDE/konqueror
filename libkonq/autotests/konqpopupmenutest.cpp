/* This file is part of KDE
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#undef QT_NO_CAST_FROM_ASCII

#include "konqpopupmenutest.h"

#include <kconfiggroup.h>
#include <kbookmarkmanager.h>
#include <KSharedConfig>
#include <knewfilemenu.h>
#include <kfileitemlistproperties.h>

#include <QTest>
#include <QDebug>
#include <QDir>
#include <QUrl>
#include <QStandardPaths>
#include <QFile>
#include <QTextStream>

QTEST_MAIN(KonqPopupMenuTest)

KonqPopupMenuTest::KonqPopupMenuTest()
    : m_actionCollection(this)
{
    m_appFlags = KonqPopupMenu::NoPlugins;
}

static QStringList extractActionNames(QMenu &menu)
{
    menu.aboutToShow(); // signals are now public in Qt5, how convenient :-)

    QString lastObjectName;
    QStringList ret;
    bool lastIsSeparator = false;
    foreach (const QAction *action, menu.actions()) {
        if (action->isSeparator()) {
            if (!lastIsSeparator) { // Qt gets rid of duplicate separators, so we should too
                ret.append(QStringLiteral("separator"));
            }
            lastIsSeparator = true;
        } else {
            lastIsSeparator = false;
            //qDebug() << action->objectName() << action->metaObject()->className() << action->text();
            const QString objectName = action->objectName();
            if (objectName.isEmpty()) {
                if (action->menu()) { // if this fails, then we have an unnamed action somewhere...
                    ret.append(QStringLiteral("submenu"));
                } else {
                    ret.append("UNNAMED " + action->text());
                }
            } else {
                if (objectName == QLatin1String("menuaction") // a single service-menu action, or a service-menu submenu: skip; too variable.
                        || objectName == QLatin1String("actions_submenu")) {
                } else if (objectName == QLatin1String("openWith_submenu")) {
                    ret.append(QStringLiteral("openwith"));
                } else if (objectName == QLatin1String("openwith_browse") && lastObjectName == QLatin1String("openwith")) {
                    // We had "open with foo" followed by openwith_browse, all is well.
                    // The expected lists only say "openwith" so that they work in both cases
                    // -> skip the browse action.
                } else {
                    ret.append(objectName);
                }
            }
        }
        lastObjectName = action->objectName();
    }
    return ret;

}

void KonqPopupMenuTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    KSharedConfig::Ptr dolphin = KSharedConfig::openConfig(QStringLiteral("dolphinrc"));
    KConfigGroup(dolphin, "General").writeEntry("ShowCopyMoveMenu", true);

    m_thisDirectoryItem = KFileItem(QUrl::fromLocalFile(QDir::currentPath()), QStringLiteral("inode/directory"), S_IFDIR + 0777);

    //Create a Makefile. It's simpler to just create a new file than to rely on the path of an existing file
    const QString makefileDir = QDir::tempPath() + QStringLiteral("/konqueror-tests/");
    if (!QDir().exists(makefileDir)) {
        QDir().mkpath(makefileDir);
        m_deleteMakefileDir = true;
    }
    m_makefilePath = makefileDir + "/Makefile";
    // qDebug() << "Creating" << m_makefilePath;
    QFile mf(m_makefilePath);
    mf.open(QFile::WriteOnly);
    QTextStream ts(&mf);
    ts << "main.o : main.c\n\tcc -c main.c\n";
    mf.close();
    QVERIFY2(QFile::exists(m_makefilePath), qPrintable(m_makefilePath));

    m_fileItem = KFileItem(QUrl::fromLocalFile(m_makefilePath), QStringLiteral("text/x-makefile"), S_IFREG + 0660);
    m_linkItem = KFileItem(QUrl::fromLocalFile(QStringLiteral("http://www.kde.org/foo")), QStringLiteral("text/html"), S_IFREG + 0660);
    m_subDirItem = KFileItem(QUrl::fromLocalFile(QDir::currentPath() + "/CMakeFiles"), QStringLiteral("inode/directory"), S_IFDIR + 0755);
    m_cut = KStandardAction::cut(nullptr, nullptr, this);
    m_actionCollection.addAction(QStringLiteral("cut"), m_cut);
    m_copy = KStandardAction::copy(nullptr, nullptr, this);
    m_actionCollection.addAction(QStringLiteral("copy"), m_copy);
    m_paste = KStandardAction::paste(nullptr, nullptr, this);
    m_actionCollection.addAction(QStringLiteral("paste"), m_paste);
    m_pasteTo = KStandardAction::paste(nullptr, nullptr, this);
    m_actionCollection.addAction(QStringLiteral("pasteto"), m_pasteTo);
    m_properties = new QAction(this);
    m_actionCollection.addAction(QStringLiteral("properties"), m_properties);

    m_tabHandlingActions = new QActionGroup(this);
    m_newWindow = new QAction(m_tabHandlingActions);
    m_actionCollection.addAction(QStringLiteral("openInNewWindow"), m_newWindow);
    m_newTab = new QAction(m_tabHandlingActions);
    m_actionCollection.addAction(QStringLiteral("openInNewTab"), m_newTab);
    QAction *separator = new QAction(m_tabHandlingActions);
    separator->setSeparator(true);
    QCOMPARE(m_tabHandlingActions->actions().count(), 3);

    m_previewActions = new QActionGroup(this);
    m_preview1 = new QAction(m_previewActions);
    m_actionCollection.addAction(QStringLiteral("preview1"), m_preview1);
    m_preview2 = new QAction(m_previewActions);
    m_actionCollection.addAction(QStringLiteral("preview2"), m_preview2);

    m_fileEditActions = new QActionGroup(this);
    m_rename = new QAction(m_fileEditActions);
    m_actionCollection.addAction(QStringLiteral("rename"), m_rename);
    m_trash = new QAction(m_fileEditActions);
    m_actionCollection.addAction(QStringLiteral("trash"), m_trash);

    m_htmlEditActions = new QActionGroup(this);
    // TODO use m_htmlEditActions like in khtml (see khtml_popupmenu.rc)

    m_linkActions = new QActionGroup(this);
    QAction *saveLinkAs = new QAction(m_linkActions);
    m_actionCollection.addAction(QStringLiteral("savelinkas"), saveLinkAs);
    QAction *copyLinkLocation = new QAction(m_linkActions);
    m_actionCollection.addAction(QStringLiteral("copylinklocation"), copyLinkLocation);
    // TODO there's a whole bunch of things for frames, and for images, see khtml_popupmenu.rc

    m_partActions = new QActionGroup(this);
    separator = new QAction(m_partActions);
    separator->setSeparator(true);
    m_partActions->addAction(separator); // we better start with a separator
    QAction *viewDocumentSource = new QAction(m_partActions);
    m_actionCollection.addAction(QStringLiteral("viewDocumentSource"), viewDocumentSource);

    m_newMenu = new KNewFileMenu(&m_actionCollection, QStringLiteral("newmenu"), nullptr);

    // Check if extractActionNames works
    QMenu popup;
    popup.addAction(m_rename);
    QMenu *subMenu = new QMenu(&popup);
    popup.addMenu(subMenu);
    subMenu->addAction(m_trash);
    QStringList actions = extractActionNames(popup);
    // qDebug() << actions;
    QCOMPARE(actions, QStringList({"rename", "submenu"}));
}

void KonqPopupMenuTest::cleanupTestCase()
{
    QFileInfo info(m_makefilePath);
    QDir dir = info.dir();
    QFile::remove(m_makefilePath);
    if (m_deleteMakefileDir) {
        QDir().rmdir(dir.path());
    }
}

void KonqPopupMenuTest::testFile()
{
    KFileItemList itemList;
    itemList << m_fileItem;
    QUrl viewUrl = QUrl::fromLocalFile(QDir::currentPath());
    const KonqPopupMenu::Flags flags = m_appFlags
            | KonqPopupMenu::ShowProperties
            | KonqPopupMenu::ShowUrlOperations;
    KonqPopupMenu::ActionGroupMap actionGroups;
    actionGroups.insert(KonqPopupMenu::TabHandlingActions, m_tabHandlingActions->actions());
    actionGroups.insert(KonqPopupMenu::EditActions, m_fileEditActions->actions());
    actionGroups.insert(KonqPopupMenu::PreviewActions, QList<QAction *>() << m_preview1);

    KonqPopupMenu popup(itemList, viewUrl, m_actionCollection, flags,
                        nullptr /*parent*/);
    popup.setNewFileMenu(m_newMenu);
    popup.setActionGroups(actionGroups);

    QStringList actions = extractActionNames(popup);
    actions.removeAll(QStringLiteral("services_submenu"));

    // Be tolerant with openwith, it could be there once or twice
    if (actions.count(QStringLiteral("openwith")) == 2) {
        actions.removeOne(QStringLiteral("openwith"));
    }
    // qDebug() << actions;
    QStringList expectedActions {
        QStringLiteral("openInNewWindow"),
        QStringLiteral("openInNewTab"),
        QStringLiteral("separator") ,
        QStringLiteral("cut"),
        QStringLiteral("copy"),
        QStringLiteral("rename"),
        QStringLiteral("trash") ,
        QStringLiteral("openwith") ,
        QStringLiteral("separator") ,
        QStringLiteral("preview1") ,
        QStringLiteral("separator")
    };

    // These are provided by Dolphin and depend on its configuration.
    // If actions contains copyTo_submenu, it means they exist, so add them
    if (actions.contains("copyTo_submenu")) {
        QStringList extraActions {
            QStringLiteral("copyTo_submenu"),
            QStringLiteral("moveTo_submenu"),
            QStringLiteral("separator")
        };
        expectedActions.append(extraActions);
    }

    expectedActions << QStringLiteral("properties");
    // qDebug() << "Expected:" << expectedActions;

    QCOMPARE(actions, expectedActions);
}

void KonqPopupMenuTest::testFileInReadOnlyDirectory()
{
    const KFileItem item(QUrl::fromLocalFile(QStringLiteral("/etc/passwd")));
    KFileItemList itemList;
    itemList << item;

    KFileItemListProperties capabilities(itemList);
    QVERIFY(!capabilities.supportsMoving());

    QUrl viewUrl = QUrl::fromLocalFile(QStringLiteral("/etc"));
    const KonqPopupMenu::Flags flags = m_appFlags
            | KonqPopupMenu::ShowProperties
            | KonqPopupMenu::ShowUrlOperations;
    KonqPopupMenu::ActionGroupMap actionGroups;
    actionGroups.insert(KonqPopupMenu::TabHandlingActions, m_tabHandlingActions->actions());
    // DolphinPart doesn't add rename/trash when supportsMoving is false
    // Maybe we should test dolphinpart directly :)
    //actionGroups.insert(KonqPopupMenu::EditActions, m_fileEditActions->actions());
    actionGroups.insert(KonqPopupMenu::PreviewActions, QList<QAction *>() << m_preview1);

    KonqPopupMenu popup(itemList, viewUrl, m_actionCollection, flags);
    popup.setNewFileMenu(m_newMenu);
    popup.setActionGroups(actionGroups);

    QStringList actions = extractActionNames(popup);
    actions.removeAll(QStringLiteral("services_submenu"));
    // Be tolerant with openwith, it could be there once or twice
    if (actions.count(QStringLiteral("openwith")) == 2) {
        actions.removeOne(QStringLiteral("openwith"));
    }
    // qDebug() << actions;

    QStringList expectedActions {
        QStringLiteral("openInNewWindow"),
        QStringLiteral("openInNewTab"),
        QStringLiteral("separator"),
        QStringLiteral("copy"),
        QStringLiteral("openwith"),
        QStringLiteral("separator"),
        QStringLiteral("preview1"),
        QStringLiteral("separator"),
    };
    // copyTo_submenu is provided by Dolphin and depends on its configuration.
    if (actions.contains(QStringLiteral("copyTo_submenu"))) {
        expectedActions.append({QStringLiteral("copyTo_submenu"), QStringLiteral("separator")});
    }
        expectedActions << QStringLiteral("properties");
    // qDebug() << "Expected:" << expectedActions;

    QCOMPARE(actions, expectedActions);
}

void KonqPopupMenuTest::testFilePreviewSubMenu()
{
    // Same as testFile, but this time the "preview" action group has more than one action
    KFileItemList itemList;
    itemList << m_fileItem;
    QUrl viewUrl = QUrl::fromLocalFile(QDir::currentPath());
    const KonqPopupMenu::Flags flags = m_appFlags
            | KonqPopupMenu::ShowProperties
            | KonqPopupMenu::ShowUrlOperations;
    KonqPopupMenu::ActionGroupMap actionGroups;
    actionGroups.insert(KonqPopupMenu::TabHandlingActions, m_tabHandlingActions->actions());
    actionGroups.insert(KonqPopupMenu::EditActions, m_fileEditActions->actions());
    actionGroups.insert(KonqPopupMenu::PreviewActions, m_previewActions->actions());

    KonqPopupMenu popup(itemList, viewUrl, m_actionCollection, flags);
    popup.setNewFileMenu(m_newMenu);
    popup.setActionGroups(actionGroups);

    QStringList actions = extractActionNames(popup);
    actions.removeAll(QStringLiteral("services_submenu"));
    // Be tolerant with openwith, it could be there once or twice
    if (actions.count(QStringLiteral("openwith")) == 2) {
        actions.removeOne(QStringLiteral("openwith"));
    }
    // qDebug() << actions;

    QStringList expectedActions {
        QStringLiteral("openInNewWindow"),
        QStringLiteral("openInNewTab"),
        QStringLiteral("separator"),
        QStringLiteral("cut"),
        QStringLiteral("copy"),
        QStringLiteral("rename"),
        QStringLiteral("trash"),
        QStringLiteral("openwith"),
        QStringLiteral("separator"),
        QStringLiteral("preview_submenu"),
        QStringLiteral("separator")
    };
    if (actions.contains("copyTo_submenu")) {
        QStringList extraActions {
            QStringLiteral("copyTo_submenu"),
            QStringLiteral("moveTo_submenu"),
            QStringLiteral("separator")
        };
        expectedActions.append(extraActions);
    }
        expectedActions.append({QStringLiteral("properties")});
    // qDebug() << "Expected:" << expectedActions;

    QCOMPARE(actions, expectedActions);
}

void KonqPopupMenuTest::testSubDirectory()
{
    KFileItemList itemList;
    itemList << m_subDirItem;
    QUrl viewUrl = QUrl::fromLocalFile(QDir::currentPath());

    const KonqPopupMenu::Flags flags = m_appFlags
            | KonqPopupMenu::ShowProperties
            | KonqPopupMenu::ShowUrlOperations;
    KonqPopupMenu::ActionGroupMap actionGroups;
    actionGroups.insert(KonqPopupMenu::TabHandlingActions, m_tabHandlingActions->actions());
    actionGroups.insert(KonqPopupMenu::EditActions, m_fileEditActions->actions());
    actionGroups.insert(KonqPopupMenu::PreviewActions, m_previewActions->actions());

    KonqPopupMenu popup(itemList, viewUrl, m_actionCollection, flags);
    popup.setNewFileMenu(m_newMenu);
    popup.setActionGroups(actionGroups);
    QStringList actions = extractActionNames(popup);
    actions.removeAll(QStringLiteral("services_submenu"));
    // qDebug() << actions;
    QStringList expectedActions{
        QStringLiteral("openInNewWindow"),
        QStringLiteral("openInNewTab"),
        QStringLiteral("separator"),
        QStringLiteral("cut"),
        QStringLiteral("copy"),
        QStringLiteral("rename"),
        QStringLiteral("trash"),
        QStringLiteral("openwith"),
        QStringLiteral("separator"),
        QStringLiteral("preview_submenu"),
        // It seems it has been moved to a submenu
        // QStringLiteral("open-terminal-here"),
        QStringLiteral("separator")
    };

    // These are provided by Dolphin and depend on its configuration.
    // If actions contains copyTo_submenu, it means they exist, so add them
    if (actions.contains("copyTo_submenu")) {
        QStringList extraActions {
            QStringLiteral("copyTo_submenu"),
            QStringLiteral("moveTo_submenu"),
            QStringLiteral("separator")
        };
        expectedActions.append(extraActions);
    }
    expectedActions << QStringLiteral("properties");
    // qDebug() << "Expected:" << expectedActions;
    QCOMPARE(actions, expectedActions);
}

void KonqPopupMenuTest::testViewDirectory()
{
    KFileItemList itemList;
    itemList << m_thisDirectoryItem;
    QUrl viewUrl = m_thisDirectoryItem.url();
    const KonqPopupMenu::Flags flags = m_appFlags |
        KonqPopupMenu::ShowCreateDirectory |
        KonqPopupMenu::ShowUrlOperations |
        KonqPopupMenu::ShowProperties;
    // KonqMainWindow says: doTabHandling = !openedForViewURL && ... So we don't add tabhandling here
    KonqPopupMenu::ActionGroupMap actionGroups;
    actionGroups.insert(KonqPopupMenu::PreviewActions, m_previewActions->actions());

    KonqPopupMenu popup(itemList, viewUrl, m_actionCollection, flags);
    popup.setNewFileMenu(m_newMenu);
    popup.setActionGroups(actionGroups);

    QStringList actions = extractActionNames(popup);
    actions.removeAll(QStringLiteral("services_submenu"));
    // qDebug() << actions;
    QStringList expectedActions {
        QStringLiteral("newmenu"),
        QStringLiteral("separator") ,
        QStringLiteral("paste") ,
        QStringLiteral("openwith") ,
        QStringLiteral("separator") ,
        QStringLiteral("preview_submenu"),
         // It seems it has been moved to a submenu
        // QStringLiteral("open-terminal-here"),
        QStringLiteral("separator")
    };
    // These are provided by Dolphin and depend on its configuration.
    // If actions contains copyTo_submenu, it means they exist, so add them
    if (actions.contains("copyTo_submenu")) {
        QStringList extraActions {
            QStringLiteral("copyTo_submenu"),
            QStringLiteral("moveTo_submenu"),
            QStringLiteral("separator")
        };
        expectedActions.append(extraActions);
    }
    expectedActions << QStringLiteral("properties");
    // qDebug() << "Expected:" << expectedActions;
    QCOMPARE(actions, expectedActions);
}

void KonqPopupMenuTest::testViewReadOnlyDirectory()
{
    KFileItem rootItem(QUrl::fromLocalFile(QDir::rootPath()), QStringLiteral("inode/directory"), KFileItem::Unknown);
    KFileItemList itemList;
    itemList << rootItem;
    QUrl viewUrl = rootItem.url();
    const KonqPopupMenu::Flags flags = m_appFlags |
        KonqPopupMenu::ShowCreateDirectory |
        KonqPopupMenu::ShowUrlOperations |
        KonqPopupMenu::ShowProperties;
    // KonqMainWindow says: doTabHandling = !openedForViewURL && ... So we don't add tabhandling here
    KonqPopupMenu::ActionGroupMap actionGroups;
    actionGroups.insert(KonqPopupMenu::PreviewActions, m_previewActions->actions());

    KonqPopupMenu popup(itemList, viewUrl, m_actionCollection, flags);
    popup.setNewFileMenu(m_newMenu);
    popup.setActionGroups(actionGroups);

    QStringList actions = extractActionNames(popup);
    actions.removeAll(QStringLiteral("services_submenu"));
    // qDebug() << actions;
    QStringList expectedActions {
        // "paste" // no paste since readonly
        QStringLiteral("openwith"),
        QStringLiteral("separator"),
        QStringLiteral("preview_submenu"),
        // It seems it has been moved to a submenu
        // QStringLiteral("open-terminal-here"),
        QStringLiteral("separator"),
    };

    // This is provided by Dolphin and depends on its configuration.
    // If actions contains copyTo_submenu, it means they exist, so add them
    if (actions.contains("copyTo_submenu")) {
        QStringList extraActions {
            QStringLiteral("copyTo_submenu"),
            // no moveTo_submenu, since readonly ,
            QStringLiteral("separator")
        };
        expectedActions.append(extraActions);
        expectedActions << QStringLiteral("properties");
    }
    // qDebug() << "Expected:" << expectedActions;
    QCOMPARE(actions, expectedActions);
}

void KonqPopupMenuTest::testHtmlLink()
{
    KFileItemList itemList;
    itemList << m_linkItem;
    QUrl viewUrl(QStringLiteral("http://www.kde.org"));
    const KonqPopupMenu::Flags flags = m_appFlags
            | KonqPopupMenu::ShowBookmark
            | KonqPopupMenu::IsLink;
    KonqPopupMenu::ActionGroupMap actionGroups;
    actionGroups.insert(KonqPopupMenu::TabHandlingActions, m_tabHandlingActions->actions());
    actionGroups.insert(KonqPopupMenu::EditActions, m_htmlEditActions->actions());
    actionGroups.insert(KonqPopupMenu::PreviewActions, m_previewActions->actions());
    actionGroups.insert(KonqPopupMenu::LinkActions, m_linkActions->actions());
    actionGroups.insert(KonqPopupMenu::CustomActions, m_partActions->actions());
    KonqPopupMenu popup(itemList, viewUrl, m_actionCollection, flags);
    popup.setNewFileMenu(m_newMenu);
    popup.setActionGroups(actionGroups);
    popup.setBookmarkManager(KBookmarkManager::userBookmarksManager());

    QStringList actions = extractActionNames(popup);
    // Be tolerant with openwith, it could be there once or twice
    if (actions.count(QStringLiteral("openwith")) == 2) {
        actions.removeOne(QStringLiteral("openwith"));
    }
    // qDebug() << actions;

    QStringList expectedActions {
        QStringLiteral("openInNewWindow"),
        QStringLiteral("openInNewTab"),
        QStringLiteral("separator") ,
        QStringLiteral("bookmark_add"),
        QStringLiteral("savelinkas"),
        QStringLiteral("copylinklocation"),
        QStringLiteral("openwith"),
        QStringLiteral("separator"),
        QStringLiteral("preview_submenu"),
        QStringLiteral("services_submenu"),
        QStringLiteral("separator"),
        QStringLiteral("viewDocumentSource")
    };
    // qDebug() << "Expected:" << expectedActions;

    QCOMPARE(actions, expectedActions);
}

void KonqPopupMenuTest::testHtmlPage()
{
    KFileItemList itemList;
    itemList << m_linkItem;
    QUrl viewUrl = m_linkItem.url();

    const KonqPopupMenu::Flags flags = m_appFlags
            | KonqPopupMenu::ShowBookmark;
    KonqPopupMenu::ActionGroupMap actionGroups;
    // KonqMainWindow says: doTabHandling = !openedForViewURL && ... So we don't add tabhandling here
    // TODO we could just move that logic to KonqPopupMenu...
    //actionGroups.insert(KonqPopupMenu::TabHandlingActions, m_tabHandlingActions->actions());
    actionGroups.insert(KonqPopupMenu::EditActions, m_htmlEditActions->actions());
    actionGroups.insert(KonqPopupMenu::PreviewActions, m_previewActions->actions());
    //actionGroups.insert(KonqPopupMenu::LinkActions, m_linkActions->actions());
    QAction *security = new QAction(m_partActions);
    m_actionCollection.addAction(QStringLiteral("security"), security);
    QAction *setEncoding = new QAction(m_partActions);
    m_actionCollection.addAction(QStringLiteral("setEncoding"), setEncoding);
    actionGroups.insert(KonqPopupMenu::CustomActions, m_partActions->actions());

    KonqPopupMenu popup(itemList, viewUrl, m_actionCollection, flags);
    popup.setNewFileMenu(m_newMenu);
    popup.setActionGroups(actionGroups);
    popup.setBookmarkManager(KBookmarkManager::userBookmarksManager());

    QStringList actions = extractActionNames(popup);
    // Be tolerant with openwith, it could be there once or twice
    if (actions.count(QStringLiteral("openwith")) == 2) {
        actions.removeOne(QStringLiteral("openwith"));
    }
    // qDebug() << actions;

    QStringList expectedActions {
        QStringLiteral("bookmark_add"),
        QStringLiteral("openwith"),
        QStringLiteral("separator"),
        QStringLiteral("preview_submenu"),
        QStringLiteral("services_submenu"),
        QStringLiteral("separator"),
        // TODO "stopanimations",
        QStringLiteral("viewDocumentSource"),
        QStringLiteral("security"),
        QStringLiteral("setEncoding")
    };
    // qDebug() << "Expected:" << expectedActions;

    QCOMPARE(actions, expectedActions);
}

// TODO test ShowBookmark. Probably the same logic?
// TODO separate filemanager and webbrowser bookmark managers, too (share file bookmarks with file dialog)

// TODO test text selection actions in khtml

// TODO trash:/ tests

// TODO test NoDeletion part flag
