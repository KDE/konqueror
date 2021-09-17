/* This file is part of KDE
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <konqcloseditem.h>
#include <qtest_gui.h>
#include <QSignalSpy>
#include <konqundomanager.h>
#include <konqsessionmanager.h>
#include <konqclosedwindowsmanager.h>

class UndoManagerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testAddClosedTabItem();
    void testUndoLastClosedTab();

private:
    KonqClosedWindowsManager m_cwManager;
};

QTEST_MAIN(UndoManagerTest)

void UndoManagerTest::initTestCase()
{
    // Make sure we start clean
    QStandardPaths::setTestModeEnabled(true);
    KonqSessionManager::self()->disableAutosave();
    KonqUndoManager manager(&m_cwManager, nullptr);
    QSignalSpy spyUndoAvailable(&manager, SIGNAL(undoAvailable(bool)));
    QVERIFY(spyUndoAvailable.isValid());
    manager.clearClosedItemsList();
    QCOMPARE(spyUndoAvailable.count(), 1);
    QCOMPARE(spyUndoAvailable[0][0], QVariant(false));
    QVERIFY(!manager.undoAvailable());
}

void UndoManagerTest::testAddClosedTabItem()
{
    KonqUndoManager manager(&m_cwManager, nullptr);
    QVERIFY(!manager.undoAvailable());
    KonqClosedTabItem *item = new KonqClosedTabItem(QStringLiteral("url"), m_cwManager.memoryStore(),
                                                    QStringLiteral("title"), 0, manager.newCommandSerialNumber());
    QCOMPARE(item->url(), QString("url"));
    QCOMPARE(item->serialNumber(), quint64(1001));
    QCOMPARE(item->pos(), 0);
    KConfigGroup configGroup = item->configGroup();
    QVERIFY(!configGroup.exists());

    QSignalSpy spyUndoAvailable(&manager, SIGNAL(undoAvailable(bool)));
    QVERIFY(spyUndoAvailable.isValid());
    QSignalSpy spyTextChanged(&manager, SIGNAL(undoTextChanged(QString)));
    QVERIFY(spyTextChanged.isValid());
    manager.addClosedTabItem(item);

    QVERIFY(manager.undoAvailable());
    QCOMPARE(spyUndoAvailable.count(), 1);
    QCOMPARE(spyTextChanged.count(), 1);
    QCOMPARE(manager.closedItemsList().count(), 1);
    configGroup.writeEntry("RootItem", "test");
    QVERIFY(!configGroup.keyList().isEmpty());

    // This requires a default constructor...
    //qRegisterMetaType<KonqClosedTabItem>("KonqClosedTabItem");
    QSignalSpy spyOpenClosedTab(&manager, SIGNAL(openClosedTab(KonqClosedTabItem)));
    manager.undo();
    QCOMPARE(spyUndoAvailable.count(), 2);
    QVERIFY(!manager.undoAvailable());
    QCOMPARE(spyOpenClosedTab.count(), 1);
    QCOMPARE(manager.closedItemsList().count(), 0);
    QVERIFY(configGroup.keyList().isEmpty()); // was deleted by the KonqClosedTabItem destructor
}

void UndoManagerTest::testUndoLastClosedTab()
{

}

#include "undomanagertest.moc"
