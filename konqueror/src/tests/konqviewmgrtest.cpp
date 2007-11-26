/* This file is part of the KDE project
   Copyright (C) 2006 David Faure <faure@kde.org>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <qtest_kde.h>
#include "konqviewmgrtest.h"

#include <konqmainwindow.h>
#include <konqviewmanager.h>
#include <konqview.h>
#include <konqtabs.h>
#include <konqframevisitor.h>
#include <kstandarddirs.h>
#include <QLayout>

QTEST_KDEMAIN_WITH_COMPONENTNAME( ViewMgrTest, GUI, "konqueror" )

#if 0
// could be used to load dummy parts; or to check that the right parts are being loaded
// (and to detect the case where a part is loaded and then replacd with another one for no good reason)
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

static void sendAllPendingResizeEvents( QWidget* mainWindow )
{
    bool foundOne = true;
    while ( foundOne ) {
        foundOne = false;
        QList<QWidget *> allChildWidgets = mainWindow->findChildren<QWidget *>();
        allChildWidgets.prepend( mainWindow );
        foreach( QWidget* w, allChildWidgets ) {
            if (w->testAttribute(Qt::WA_PendingResizeEvent)) {
                //qDebug() << "Resizing" << w << " to " << w->size() << endl;
                QResizeEvent e(w->size(), QSize());
                QApplication::sendEvent(w, &e);
                w->setAttribute(Qt::WA_PendingResizeEvent, false);
                // hack: make QTabWidget think it's visible; no layout otherwise
                w->setAttribute(Qt::WA_WState_Visible, true);
                foundOne = true;
            }
        }
        // Process LayoutRequest events, in particular
        qApp->sendPostedEvents();
        //qDebug() << "Loop done, checking again";
    }
}

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
    KonqViewManager* viewMgr = mainWindow.viewManager();
    KonqView* view = viewMgr->createFirstView( "KonqAboutPage", "konq_aboutpage" );
    QVERIFY( view );
    QVERIFY( viewMgr->tabContainer() );

    // Use DebugFrameVisitor to find out the structure of the frame hierarchy
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].") ); // mainWindow, one tab, one frame

    // Check widget parents:  part's widget -> frame -> [tabwidget's stack] -> tabwidget -> mainWindow
    QWidget* partWidget = view->part()->widget();
    QCOMPARE( partWidget->topLevelWidget(), &mainWindow );
    QWidget* frame = view->frame()->asQWidget();
    QCOMPARE( partWidget->parentWidget(), frame );
    QWidget* tabWidget = viewMgr->tabContainer()->asQWidget();
    QCOMPARE( frame->parentWidget()->parentWidget(), tabWidget );

    // Check frame geometry, to check that all layouts are there
    // (the mainWindow is resized to 700x480 in its constructor)
    // But pending resize events are only sent by show(), and we don't want to see
    // widgets from unit tests.
    // So we iterate over all widgets and ensure the pending resize events are sent.
    sendAllPendingResizeEvents( &mainWindow );
    //for ( QWidget* w = partWidget; w; w = w->parentWidget() )
    //    qDebug() << w << w->geometry();
    QVERIFY( frame->width() > 680 );
    QVERIFY( frame->height() > 300 ); // 325
    //qDebug() << "partWidget geom:" << partWidget->geometry();
    QVERIFY( partWidget->width() > 680 );
    QVERIFY( partWidget->height() > 290 ); // 295 (325 - statusbar)
    //qDebug() << "tabWidget geom: " << tabWidget->geometry();
    QVERIFY( tabWidget->width() > 680 );
    QVERIFY( tabWidget->height() > 300 ); // 329
}

void ViewMgrTest::testRemoveFirstView()
{
    KonqMainWindow mainWindow;
    KonqViewManager* viewMgr = mainWindow.viewManager();
    KonqView* view = viewMgr->createFirstView( "KonqAboutPage", "konq_aboutpage" );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].") ); // mainWindow, tab widget, one frame
    viewMgr->removeView( view );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].") ); // removing not allowed
    // real test for removeView is part of testSplitView
}

void ViewMgrTest::testSplitView()
{
    KonqMainWindow mainWindow;
    KonqViewManager* viewMgr = mainWindow.viewManager();
    KonqView* view = viewMgr->createFirstView( "KonqAboutPage", "konq_aboutpage" );

    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].") ); // mainWindow, tab widget, one frame
    KonqView* view2 = viewMgr->splitView( view, Qt::Horizontal );
    QVERIFY( view2 );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)].") ); // mainWindow, tab widget, one splitter, two frames

    // Check widget parents
    //mainWindow.dumpObjectTree();
    QWidget* partWidget = view->part()->widget();
    QCOMPARE( partWidget->topLevelWidget(), &mainWindow );
    QWidget* frame = view->frame()->asQWidget();
    QCOMPARE( partWidget->parentWidget(), frame );
    QVERIFY(!frame->isHidden());

    QWidget* part2Widget = view2->part()->widget();
    QCOMPARE( part2Widget->topLevelWidget(), &mainWindow );
    QWidget* frame2 = view2->frame()->asQWidget();
    QCOMPARE( part2Widget->parentWidget(), frame2 );
    QVERIFY(!frame2->isHidden());

    // Check container
    QVERIFY(view->frame()->parentContainer()->frameType() == "Container");
    KonqFrameContainer* container = static_cast<KonqFrameContainer *>(view->frame()->parentContainer());
    QVERIFY(container);
    QCOMPARE(container->count(), 2);
    QCOMPARE(container, view2->frame()->parentContainer());
    QCOMPARE(container->firstChild(), view->frame());
    QCOMPARE(container->secondChild(), view2->frame());
    QCOMPARE(container->widget(0), view->frame()->asQWidget());
    QCOMPARE(container->widget(1), view2->frame()->asQWidget());

    // Check frame geometries
    sendAllPendingResizeEvents( &mainWindow );
    //for ( QWidget* w = partWidget; w; w = w->parentWidget() )
    //    qDebug() << w << w->geometry() << "visible:" << w->isVisible();

    //qDebug() << "view geom:" << frame->geometry();
    QVERIFY( frame->width() > 300 && frame->width() < 400 ); // horiz split, so half the mainWindow width
    QVERIFY( frame->height() > 300 );
    //qDebug() << "view2 geom:" << frame2->geometry();
    QVERIFY( frame2->width() > 300 && frame2->width() < 400 ); // horiz split, so half the mainWindow width
    QVERIFY( frame2->height() > 300 );
    QCOMPARE( frame->size(), frame2->size() );
    //qDebug() << "partWidget geom:" << partWidget->geometry();
    QVERIFY( partWidget->width() > 300 && partWidget->width() < 400 ); // horiz split, so half the mainWindow width
    QVERIFY( partWidget->height() > 290 );
    QVERIFY( part2Widget->width() > 300 && part2Widget->width() < 400 ); // horiz split, so half the mainWindow width
    QVERIFY( part2Widget->height() > 290 );

    //KonqFrameContainerBase* container = view->frame()->parentContainer();
    //QVERIFY( container );
    //qDebug() << "container geom: " << container->asQWidget()->geometry();


    // Split again
    KonqView* view3 = viewMgr->splitView( view, Qt::Horizontal );
    QVERIFY( view3 );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(C(FF)F)].") );

    // Now test removing the first view
    viewMgr->removeView( view );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)].") ); // mainWindow, tab widget, one splitter, two frames

    // Now test removing the last view
    viewMgr->removeView( view3 );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].") ); // mainWindow, tab widget, one frame
}

void ViewMgrTest::testSplitMainContainer()
{
    KonqMainWindow mainWindow;
    KonqViewManager* viewMgr = mainWindow.viewManager();
    KonqView* view = viewMgr->createFirstView( "KonqAboutPage", "konq_aboutpage" );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].") ); // mainWindow, tab widget, one frame
    KonqFrameContainerBase* tabContainer = view->frame()->parentContainer();
    KonqView* view2 = viewMgr->splitMainContainer( view, Qt::Horizontal, "KonqAboutPage", "konq_aboutpage", true );
    QVERIFY( view2 );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MC(FT[F]).") ); // mainWindow, splitter, frame, tab widget, one frame

    // Check widget parents
    QWidget* partWidget = view->part()->widget();
    QCOMPARE( partWidget->topLevelWidget(), &mainWindow );
    QWidget* frame = view->frame()->asQWidget();
    QCOMPARE( partWidget->parentWidget(), frame );
    QVERIFY(!frame->isHidden());

    QWidget* part2Widget = view2->part()->widget();
    QCOMPARE( part2Widget->topLevelWidget(), &mainWindow );
    QWidget* frame2 = view2->frame()->asQWidget();
    QCOMPARE( part2Widget->parentWidget(), frame2 );
    QVERIFY(!frame2->isHidden());

    // Check container
    QVERIFY(view->frame()->parentContainer()->frameType() == "Tabs");
    QVERIFY(view2->frame()->parentContainer()->frameType() == "Container");
    KonqFrameContainer* container = static_cast<KonqFrameContainer *>(view2->frame()->parentContainer());
    QVERIFY(container);
    QCOMPARE(container->count(), 2);
    QCOMPARE(container, view2->frame()->parentContainer());
    QCOMPARE(container->firstChild(), view2->frame());
    QCOMPARE(container->secondChild(), tabContainer);
    QCOMPARE(container->widget(0), view2->frame()->asQWidget());
    QCOMPARE(container->widget(1), tabContainer->asQWidget());

    // Now test removing the view we added last
    viewMgr->removeView( view2 );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].") ); // mainWindow, tab widget, one frame
}

void ViewMgrTest::testAddTab()
{
    KonqMainWindow mainWindow;
    KonqViewManager* viewMgr = mainWindow.viewManager();
    KonqView* view = viewMgr->createFirstView( "KonqAboutPage", "konq_aboutpage" );
    QVERIFY( view );
    KonqView* viewTab2 = viewMgr->addTab("text/html");
    QVERIFY( viewTab2 );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[FF].") ); // mainWindow, tab widget, two tabs

}

void ViewMgrTest::testDuplicateTab()
{
    KonqMainWindow mainWindow;
    KonqViewManager* viewMgr = mainWindow.viewManager();
    KonqView* view = viewMgr->createFirstView( "KonqAboutPage", "konq_aboutpage" );
    viewMgr->duplicateTab(view->frame()); // should return a KonqFrameBase?

    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[FF].") ); // mainWindow, tab widget, two tabs
    // TODO check serviceType and serviceName of the new view
}

void ViewMgrTest::testDuplicateSplittedTab()
{
    KonqMainWindow mainWindow;
    KonqViewManager* viewMgr = mainWindow.viewManager();
    KonqView* view = viewMgr->createFirstView( "KonqAboutPage", "konq_aboutpage" );
    KonqView* view2 = viewMgr->splitView( view, Qt::Vertical );
    QVERIFY( view2 );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)].") ); // mainWindow, tab widget, one splitter, two frames

    KonqFrameContainer* container = static_cast<KonqFrameContainer *>(view->frame()->parentContainer());
    QVERIFY( container );
    QVERIFY( container->parentContainer()->frameType() == "Tabs" ); // TODO enum instead

    viewMgr->duplicateTab(container); // TODO shouldn't it return a KonqFrameBase?
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)C(FF)].") ); // mainWindow, tab widget, two tabs

    viewMgr->removeTab(container);
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)].") ); // mainWindow, tab widget, one tab
}

void ViewMgrTest::testLoadProfile()
{
    // TODO
    // TODO check view->locationBarUrl() and mainWindow.locationBarUrl() too
}

#include "konqviewmgrtest.moc"
