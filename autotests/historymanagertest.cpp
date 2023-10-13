/* This file is part of KDE
    SPDX-FileCopyrightText: 2006 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QTest>
#include <QSignalSpy>
#include <konqhistorymanager.h>

#include <QObject>
#include <QStandardPaths>

class HistoryManagerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testGetSetMaxCount();
    void testGetSetMaxAge();
    void testAddHistoryEntry();
};

QTEST_MAIN(HistoryManagerTest)

void HistoryManagerTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void HistoryManagerTest::testGetSetMaxCount()
{
    KonqHistoryManager mgr(nullptr);
    const int oldMaxCount = mgr.maxCount();
    qDebug("oldMaxCount=%d", oldMaxCount);
    mgr.emitSetMaxCount(4242);
    QTest::qWait(100);   // ### fragile. We have no signal to wait for, so we must just wait a little bit...
    // Yes this is just a set+get test, but given that it goes via DBUS before changing the member variable
    // we do test quite a lot with it. We can't really instantiate two KonqHistoryManagers (same dbus path),
    // so we'd need two processes to test the dbus signal 'for real', if the setter changed the member var...
    QCOMPARE(mgr.maxCount(), 4242);
    mgr.emitSetMaxCount(oldMaxCount);
    QTest::qWait(100);   // ### fragile. Wait again otherwise the change will be lost
    QCOMPARE(mgr.maxCount(), oldMaxCount);
}

void HistoryManagerTest::testGetSetMaxAge()
{
    KonqHistoryManager mgr(nullptr);
    const int oldMaxAge = mgr.maxAge();
    qDebug("oldMaxAge=%d", oldMaxAge);
    mgr.emitSetMaxAge(4242);
    QTest::qWait(100);   // ### fragile. We have no signal to wait for, so we must just wait a little bit...
    QCOMPARE(mgr.maxAge(), 4242);
    mgr.emitSetMaxAge(oldMaxAge);
    QTest::qWait(100);   // ### fragile. Wait again otherwise the change will be lost
    QCOMPARE(mgr.maxAge(), oldMaxAge);
}

static void waitForAddedSignal(KonqHistoryManager *mgr)
{
    QEventLoop eventLoop;
    QObject::connect(mgr, &KonqHistoryManager::entryAdded, &eventLoop, &QEventLoop::quit);
    eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
}

static void waitForRemovedSignal(KonqHistoryManager *mgr)
{
    QEventLoop eventLoop;
    QObject::connect(mgr, &KonqHistoryManager::entryRemoved, &eventLoop, &QEventLoop::quit);
    eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
}

void HistoryManagerTest::testAddHistoryEntry()
{
    KonqHistoryManager mgr(nullptr);
    qRegisterMetaType<KonqHistoryEntry>("KonqHistoryEntry");
    QSignalSpy addedSpy(&mgr, &KonqHistoryManager::entryAdded);
    QSignalSpy removedSpy(&mgr,  &KonqHistoryManager::entryRemoved);
    const QUrl url(QStringLiteral("http://user@historymgrtest.org/"));
    const QString typedUrl = QStringLiteral("http://www.example.net");
    const QString title = QStringLiteral("The Title");
    mgr.addPending(url, typedUrl, title);

    waitForAddedSignal(&mgr);

    QCOMPARE(addedSpy.count(), 1);
    QCOMPARE(removedSpy.count(), 0);
    QList<QVariant> args = addedSpy[0];
    QCOMPARE(args.count(), 1);
    KonqHistoryEntry entry = qvariant_cast<KonqHistoryEntry>(args[0]);
    QCOMPARE(entry.url.url(), url.url());
    QCOMPARE(entry.typedUrl, typedUrl);
    QCOMPARE(entry.title, QString());   // not set yet, still pending
    QCOMPARE(int(entry.numberOfTimesVisited), 1);

    // Now confirm it
    mgr.confirmPending(url, typedUrl, title);
    // ## alternate code path: mgr.removePending()

    waitForAddedSignal(&mgr);

    QCOMPARE(addedSpy.count(), 2);
    QCOMPARE(removedSpy.count(), 0);
    args = addedSpy[1];
    QCOMPARE(args.count(), 1);
    entry = qvariant_cast<KonqHistoryEntry>(args[0]);
    QCOMPARE(entry.url.url(), url.url());
    QCOMPARE(entry.typedUrl, typedUrl);
    QCOMPARE(entry.title, title);   // now it's there
    QCOMPARE(int(entry.numberOfTimesVisited), 1);

    // Now clean it up

    mgr.emitRemoveFromHistory(url);

    waitForRemovedSignal(&mgr);

    QCOMPARE(removedSpy.count(), 1);
    QCOMPARE(addedSpy.count(), 2);   // unchanged
    args = removedSpy[0];
    QCOMPARE(args.count(), 1);
    entry = qvariant_cast<KonqHistoryEntry>(args[0]);
    QCOMPARE(entry.url.url(), url.url());
    QCOMPARE(entry.typedUrl, typedUrl);
    QCOMPARE(entry.title, title);
    QCOMPARE(int(entry.numberOfTimesVisited), 1);
}

#include "historymanagertest.moc"
