/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2006 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    Boston, MA 02110-1301, USA.
*/

#ifndef KONQVIEWMGRTEST_H
#define KONQVIEWMGRTEST_H

#include <QObject>

class ViewMgrTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void testCloseOtherTabs();
    void testCloseTabsFast();
    void testCreateFirstView();
    void testEmptyWindow();
    void testRemoveFirstView();
    void testSplitView();
    void testSplitMainContainer();
    void testLinkedViews();

    void testPopupNewTab();
    void testPopupNewWindow();
    void testCtrlClickOnLink();
    void sameTestsWithNewTabsInFront();
    void sameTestsWithMmbOpenTabsFalse();

    void testAddTabs();
    void testDuplicateTab();
    void testDuplicateSplittedTab();
    void testDeletePartInTab();
    void testSaveProfile();

    void testDuplicateWindow();

    void testBrowserArgumentsNewTab();

    void testBreakOffTab();
    void moveTabLeft();

    static void sendAllPendingResizeEvents(QWidget *);
};

#endif
