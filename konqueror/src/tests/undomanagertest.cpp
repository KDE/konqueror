/* This file is part of KDE
    Copyright 2007 David Faure <faure@kde.org>

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

#include <konqcloseditem.h>
#include <qtest_kde.h>
#include <konqundomanager.h>

class UndoManagerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testAddClosedTabItem();
    void testUndoLastClosedTab();
};


QTEST_KDEMAIN( UndoManagerTest, GUI )

void UndoManagerTest::testAddClosedTabItem()
{
    KonqUndoManager manager(0);
    QVERIFY(!manager.undoAvailable());
    KonqClosedTabItem* item = new KonqClosedTabItem("url", "title", 0, manager.newCommandSerialNumber());
    QCOMPARE(item->url(), QString("url"));
    QCOMPARE(item->serialNumber(), (quint64)1);
    QCOMPARE(item->pos(), 0);
    KConfigGroup configGroup = item->configGroup();
    QVERIFY(!configGroup.exists());

    QSignalSpy spyUndoAvailable(&manager, SIGNAL(undoAvailable(bool)) );
    QVERIFY(spyUndoAvailable.isValid());
    QSignalSpy spyTextChanged(&manager, SIGNAL(undoTextChanged(QString)) );
    QVERIFY( spyTextChanged.isValid() );
    manager.addClosedTabItem(item);

    QVERIFY(manager.undoAvailable());
    QCOMPARE(spyUndoAvailable.count(), 1);
    QCOMPARE(spyTextChanged.count(), 1);
    QCOMPARE(manager.closedItemsList().count(), 1);
    configGroup.writeEntry( "RootItem", "test" );
    QVERIFY(!configGroup.keyList().isEmpty());

    // This requires a default constructor...
    //qRegisterMetaType<KonqClosedTabItem>("KonqClosedTabItem");
    QSignalSpy spyOpenClosedTab(&manager, SIGNAL(openClosedTab(KonqClosedTabItem)) );
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
