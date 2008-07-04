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
#include <QToolBar>

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
    KonqViewManager* viewManager = mainWindow.viewManager();
    KonqView* view = viewManager->createFirstView( "KonqAboutPage", "konq_aboutpage" );
    QVERIFY( view );
    QVERIFY( viewManager->tabContainer() );

    // Use DebugFrameVisitor to find out the structure of the frame hierarchy
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].") ); // mainWindow, one tab, one frame

    // Check widget parents:  part's widget -> frame -> [tabwidget's stack] -> tabwidget -> mainWindow
    QWidget* partWidget = view->part()->widget();
    QCOMPARE( partWidget->window(), &mainWindow );
    QWidget* frame = view->frame()->asQWidget();
    QCOMPARE( partWidget->parentWidget(), frame );
    QWidget* tabWidget = viewManager->tabContainer()->asQWidget();
    QCOMPARE( frame->parentWidget()->parentWidget(), tabWidget );

    // Check frame geometry, to check that all layouts are there
    // (the mainWindow is resized to 700x480 in its constructor)
    // But pending resize events are only sent by show(), and we don't want to see
    // widgets from unit tests.
    // So we iterate over all widgets and ensure the pending resize events are sent.
    sendAllPendingResizeEvents( &mainWindow );
    for ( QWidget* w = partWidget; w; w = w->parentWidget() )
        qDebug() << w << w->geometry();
    //const QList<QToolBar*> toolbars = mainWindow.findChildren<QToolBar *>();
    //foreach( QToolBar* toolbar, toolbars ) {
    //    if (!toolbar->isHidden())
    //        qDebug() << toolbar << toolbar->geometry();
    //}
    QVERIFY( frame->width() > 680 );
    QVERIFY( frame->height() > 240 ); // usually 325, but can be 256 with oxygen when three toolbars are shown
    //qDebug() << "partWidget geom:" << partWidget->geometry();
    QVERIFY( partWidget->width() > 680 );
    QVERIFY( partWidget->height() > frame->height() - 50 /*statusbar*/ );
    //qDebug() << "tabWidget geom: " << tabWidget->geometry();
    QVERIFY( tabWidget->width() > 680 );
    QVERIFY( tabWidget->height() > frame->height() );
}

void ViewMgrTest::testRemoveFirstView()
{
    KonqMainWindow mainWindow;
    KonqViewManager* viewManager = mainWindow.viewManager();
    KonqView* view = viewManager->createFirstView( "KonqAboutPage", "konq_aboutpage" );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].") ); // mainWindow, tab widget, one frame
    viewManager->removeView( view );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].") ); // removing not allowed
    // real test for removeView is part of testSplitView
}

void ViewMgrTest::testSplitView()
{
    KonqMainWindow mainWindow;
    KonqViewManager* viewManager = mainWindow.viewManager();
    KonqView* view = viewManager->createFirstView( "KonqAboutPage", "konq_aboutpage" );

    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].") ); // mainWindow, tab widget, one frame
    KonqView* view2 = viewManager->splitView( view, Qt::Horizontal );
    QVERIFY( view2 );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)].") ); // mainWindow, tab widget, one splitter, two frames

    // Check widget parents
    //mainWindow.dumpObjectTree();
    QWidget* partWidget = view->part()->widget();
    QCOMPARE( partWidget->window(), &mainWindow );
    QWidget* frame = view->frame()->asQWidget();
    QCOMPARE( partWidget->parentWidget(), frame );
    QVERIFY(!frame->isHidden());

    QWidget* part2Widget = view2->part()->widget();
    QCOMPARE( part2Widget->window(), &mainWindow );
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
    QVERIFY( frame->height() > 240 ); // usually 325, but can be 256 with oxygen when three toolbars are shown
    //qDebug() << "view2 geom:" << frame2->geometry();
    QVERIFY( frame2->width() > 300 && frame2->width() < 400 ); // horiz split, so half the mainWindow width
    QVERIFY( frame2->height() > 240 ); // usually 325, but can be 256 with oxygen when three toolbars are shown
    // Both frames should have the same size; well, if the width was odd then there can be an off-by-one...
    QCOMPARE( frame->height(), frame2->height() );
    QVERIFY( qAbs(frame->width() - frame2->width()) <= 1 ); // e.g. 173 and 172 are "close enough"
    //qDebug() << "partWidget geom:" << partWidget->geometry();
    QVERIFY( partWidget->width() > 300 && partWidget->width() < 400 ); // horiz split, so half the mainWindow width
    QVERIFY( partWidget->height() > 220 ); // frame minus statusbar height
    QVERIFY( part2Widget->width() > 300 && part2Widget->width() < 400 ); // horiz split, so half the mainWindow width
    QVERIFY( part2Widget->height() > 220 );

    //KonqFrameContainerBase* container = view->frame()->parentContainer();
    //QVERIFY( container );
    //qDebug() << "container geom: " << container->asQWidget()->geometry();


    // Split again
    KonqView* view3 = viewManager->splitView( view, Qt::Horizontal );
    QVERIFY( view3 );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(C(FF)F)].") );

    // Now test removing the first view
    viewManager->removeView( view );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)].") ); // mainWindow, tab widget, one splitter, two frames

    // Now test removing the last view
    viewManager->removeView( view3 );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].") ); // mainWindow, tab widget, one frame
}

void ViewMgrTest::testSplitMainContainer()
{
    KonqMainWindow mainWindow;
    KonqViewManager* viewManager = mainWindow.viewManager();
    KonqView* view = viewManager->createFirstView( "KonqAboutPage", "konq_aboutpage" );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].") ); // mainWindow, tab widget, one frame
    KonqFrameContainerBase* tabContainer = view->frame()->parentContainer();
    KonqView* view2 = viewManager->splitMainContainer( view, Qt::Horizontal, "KonqAboutPage", "konq_aboutpage", true );
    QVERIFY( view2 );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MC(FT[F]).") ); // mainWindow, splitter, frame, tab widget, one frame

    // Check widget parents
    QWidget* partWidget = view->part()->widget();
    QCOMPARE( partWidget->window(), &mainWindow );
    QWidget* frame = view->frame()->asQWidget();
    QCOMPARE( partWidget->parentWidget(), frame );
    QVERIFY(!frame->isHidden());

    QWidget* part2Widget = view2->part()->widget();
    QCOMPARE( part2Widget->window(), &mainWindow );
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
    viewManager->removeView( view2 );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].") ); // mainWindow, tab widget, one frame
}

void ViewMgrTest::testAddTab()
{
    KonqMainWindow mainWindow;
    KonqViewManager* viewManager = mainWindow.viewManager();
    KonqView* view = viewManager->createFirstView( "KonqAboutPage", "konq_aboutpage" );
    QVERIFY( view );
    KonqView* viewTab2 = viewManager->addTab("text/html");
    QVERIFY( viewTab2 );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[FF].") ); // mainWindow, tab widget, two tabs
    QVERIFY(viewTab2->supportsMimeType("text/html"));
    QVERIFY(!viewTab2->supportsMimeType("text/plain"));
    // correct since it's a subclass of text/html, khtml can display it
    QVERIFY(viewTab2->supportsMimeType("application/x-netscape-bookmarks"));
}

void ViewMgrTest::testDuplicateTab()
{
    KonqMainWindow mainWindow;
    KonqViewManager* viewManager = mainWindow.viewManager();
    KonqView* view = viewManager->createFirstView( "KonqAboutPage", "konq_aboutpage" );
    viewManager->duplicateTab(view->frame()); // should return a KonqFrameBase?

    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[FF].") ); // mainWindow, tab widget, two tabs
    // TODO check serviceType and serviceName of the new view
}

void ViewMgrTest::testDuplicateSplittedTab()
{
    KonqMainWindow mainWindow;
    KonqViewManager* viewManager = mainWindow.viewManager();
    KonqView* view = viewManager->createFirstView( "KonqAboutPage", "konq_aboutpage" );
    KonqView* view2 = viewManager->splitView( view, Qt::Vertical );
    QVERIFY( view2 );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)].") ); // mainWindow, tab widget, one splitter, two frames

    KonqFrameContainer* container = static_cast<KonqFrameContainer *>(view->frame()->parentContainer());
    QVERIFY( container );
    QVERIFY( container->parentContainer()->frameType() == "Tabs" ); // TODO enum instead

    viewManager->duplicateTab(container); // TODO shouldn't it return a KonqFrameBase?
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)C(FF)].") ); // mainWindow, tab widget, two tabs

    viewManager->removeTab(container);
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)].") ); // mainWindow, tab widget, one tab
}

void ViewMgrTest::testDuplicateWindow()
{
    KonqMainWindow mainWindow;
    mainWindow.openUrl(0, KUrl("data:text/html, <p>Hello World</p>"), "text/html");
    KonqViewManager* viewManager = mainWindow.viewManager();
    KonqView* viewTab2 = viewManager->addTab("text/html");
    KonqView* splitted = viewManager->splitView( viewTab2, Qt::Horizontal );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[FC(FF)].") ); // mainWindow, tab widget, first tab = one frame, second tab = splitter with two frames
    KonqMainWindow* secondWindow = viewManager->duplicateWindow();
    QCOMPARE( DebugFrameVisitor::inspect(secondWindow), QString("MT[FC(FF)].") ); // mainWindow, tab widget, first tab = one frame, second tab = splitter with two frames
    delete secondWindow;
}

// Like in http://bugs.kde.org/show_bug.cgi?id=153533,
// where the part deletes itself.
void ViewMgrTest::testDeletePartInTab()
{
    KonqMainWindow mainWindow;
    KonqViewManager* viewManager = mainWindow.viewManager();
    KonqView* view = viewManager->createFirstView( "KonqAboutPage", "konq_aboutpage" );
    QVERIFY( view );
    QPointer<KonqView> viewTab2 = viewManager->addTab("text/html");
    QVERIFY(!viewTab2.isNull());
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[FF].") ); // mainWindow, tab widget, two tabs

    delete viewTab2->part();
    QVERIFY(viewTab2.isNull());
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].") ); // mainWindow, tab widget, one tab
}

void ViewMgrTest::testLoadProfile()
{
    KonqMainWindow mainWindow;
    KonqViewManager* viewManager = mainWindow.viewManager();
    const QString profile = KStandardDirs::locate("data", "konqueror/profiles/filemanagement");
    QVERIFY(!profile.isEmpty());
    QString path = QDir::homePath();
    viewManager->loadViewProfileFromFile(profile, "filemanagement", KUrl(path));
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MC(FT[F]).") ); // mainWindow, splitter, frame, tab widget, one frame
    QCOMPARE(mainWindow.locationBarURL(), path);
    QCOMPARE(mainWindow.currentView()->locationBarURL(), path);

    sendAllPendingResizeEvents( &mainWindow );

    QVERIFY(mainWindow.width() > 680);
    // QCOMPARE(frameType,QByteArray("Container")) leads to unreadable output on a mismatch :)
    QCOMPARE(QString(mainWindow.childFrame()->frameType()), QString("Container"));
    KonqFrameContainer* container = static_cast<KonqFrameContainer *>(mainWindow.childFrame());
    KonqFrameBase* firstChild = container->firstChild();
    QWidget* sidebarFrame = container->widget(0);
    QCOMPARE(firstChild->asQWidget(), sidebarFrame);
    QCOMPARE(QString(firstChild->frameType()), QString("View"));
    //QVERIFY(qobject_cast<KonqFrame*>(sidebarFrame));
    KonqFrameBase* secondChild = container->secondChild();
    QCOMPARE(QString(secondChild->frameType()), QString("Tabs"));
    QWidget* tabFrame = container->widget(1);
    QCOMPARE(secondChild->asQWidget(), tabFrame);
    QCOMPARE(sidebarFrame->sizePolicy().horizontalPolicy(), QSizePolicy::Preferred);
    QCOMPARE(sidebarFrame->sizePolicy().horizontalStretch(), 0);
    QCOMPARE(tabFrame->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
    QCOMPARE(tabFrame->sizePolicy().horizontalStretch(), 0);
    const QList<int> sizes = container->sizes();
    QCOMPARE(sizes.count(), 2);
    QVERIFY(sizes[0] < sizes[1]);
}

#include "konqviewmgrtest.moc"
