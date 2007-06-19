/* This file is part of the KDE project
   Copyright (C) 2006 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <qtest_kde.h>
#include "konqviewmgrtest.h"

#include <konq_mainwindow.h>
#include <konq_viewmgr.h>
#include <konq_view.h>
#include <kstandarddirs.h>
#include <konq_framevisitor.h>

QTEST_KDEMAIN_WITH_COMPONENTNAME( ViewMgrTest, GUI, "konqueror" )

#if 0
class KonqTestFactory : public KonqAbstractFactory
{
public:
    virtual KonqViewFactory createView( const QString &serviceType,
                                        const QString &serviceName = QString(),
                                        KService::Ptr *serviceImpl = 0,
                                        KService::List *partServiceOffers = 0,
                                        KService::List *appServiceOffers = 0,
                                        bool forceAutoEmbed = false );

};
#endif

class DebugFrameVisitor : public KonqFrameVisitor
{
public:
    DebugFrameVisitor() {}
    QString output() const { return m_output; }
    virtual bool visit(KonqFrame*) { m_output += 'F'; return true; }
    virtual bool visit(KonqFrameContainer*) { m_output += "C("; return true; }
    virtual bool visit(KonqFrameTabs*) { m_output += "T["; return true; }
    virtual bool visit(KonqMainWindow*) { m_output += 'M'; return true; }
    virtual bool endVisit(KonqFrameContainer*) { m_output += ')'; return true; }
    virtual bool endVisit(KonqFrameTabs*) { m_output += ']'; return true; }
    virtual bool endVisit(KonqMainWindow*) { m_output += '.'; return true; }

    static QString inspect(KonqMainWindow* mainWindow) {
        DebugFrameVisitor dfv;
        bool ok = mainWindow->accept( &dfv );
        if ( !ok )
            return QString("ERROR: visitor returned false");
        return dfv.output();
    }

private:
    QString m_output;
};

void ViewMgrTest::initTestCase()
{
    QVERIFY( KGlobal::mainComponent().componentName() == "konqueror" );
}

void ViewMgrTest::testCreateFirstView()
{
    KonqMainWindow mainWindow;
    KonqViewManager viewMgr( &mainWindow );
    KonqView* view = viewMgr.createFirstView( "KonqAboutPage", "konq_aboutpage" );
    QVERIFY( view );
    QVERIFY( viewMgr.tabContainer() );

    // Use DebugFrameVisitor to find out the structure of the frame hierarchy
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].") ); // mainWindow, one tab, one frame
}

void ViewMgrTest::testRemoveFirstView()
{
    KonqMainWindow mainWindow;
    KonqViewManager viewMgr( &mainWindow );
    KonqView* view = viewMgr.createFirstView( "KonqAboutPage", "konq_aboutpage" );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].") ); // mainWindow, tab widget, one frame
    viewMgr.removeView( view );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].") ); // removing not allowed
    // real test for removeView is part of testSplitView
}

void ViewMgrTest::testSplitView()
{
    KonqMainWindow mainWindow;
    KonqViewManager viewMgr( &mainWindow );
    KonqView* view = viewMgr.createFirstView( "KonqAboutPage", "konq_aboutpage" );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].") ); // mainWindow, tab widget, one frame
    // TODO also test newOneFirst after improving the visitor... using 'F' + frame->objectName()[0]?
    // Or registring views to the visitor... or to a registry used by the visitor, rather.
    KonqView* view2 = viewMgr.splitView( view, Qt::Vertical );
    QVERIFY( view2 );
    QCOMPARE( view->frame()->parentContainer(), view2->frame()->parentContainer() );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)].") ); // mainWindow, tab widget, one splitter, two frames

    // Split again
    KonqView* view3 = viewMgr.splitView( view, Qt::Horizontal );
    QVERIFY( view3 );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(C(FF)F)].") );

    // Now test removing the first view
    viewMgr.removeView( view );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)].") ); // mainWindow, tab widget, one splitter, two frames

    // Now test removing the last view
    viewMgr.removeView( view3 );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].") ); // mainWindow, tab widget, one frame
}

void ViewMgrTest::testAddTab()
{
    KonqMainWindow mainWindow;
    KonqViewManager viewMgr( &mainWindow );
    KonqView* view = viewMgr.createFirstView( "KonqAboutPage", "konq_aboutpage" );
    QVERIFY( view );
    KonqView* viewTab2 = viewMgr.addTab("text/html");
    QVERIFY( viewTab2 );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[FF].") ); // mainWindow, tab widget, two tabs

}

void ViewMgrTest::testDuplicateTab()
{
    KonqMainWindow mainWindow;
    KonqViewManager viewMgr( &mainWindow );
    KonqView* view = viewMgr.createFirstView( "KonqAboutPage", "konq_aboutpage" );
    viewMgr.duplicateTab(view->frame()); // should return a KonqFrameBase?

    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[FF].") ); // mainWindow, tab widget, two tabs
    // TODO check serviceType and serviceName of the new view
}

void ViewMgrTest::testDuplicateSplittedTab()
{
    KonqMainWindow mainWindow;
    KonqViewManager viewMgr( &mainWindow );
    KonqView* view = viewMgr.createFirstView( "KonqAboutPage", "konq_aboutpage" );
    KonqView* view2 = viewMgr.splitView( view, Qt::Vertical );
    QVERIFY( view2 );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)].") ); // mainWindow, tab widget, one splitter, two frames

    KonqFrameContainer* container = static_cast<KonqFrameContainer *>(view->frame()->parentContainer());
    QVERIFY( container );
    QVERIFY( container->parentContainer()->frameType() == "Tabs" ); // TODO enum instead

    viewMgr.duplicateTab(container); // TODO shouldn't it return a KonqFrameBase?
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)C(FF)].") ); // mainWindow, tab widget, two tabs

    viewMgr.removeTab(container);
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)].") ); // mainWindow, tab widget, one tab
}

#include "konqviewmgrtest.moc"
