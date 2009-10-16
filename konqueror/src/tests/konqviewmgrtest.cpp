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
#include "../konqsettingsxt.h"
#include <QToolBar>
#include <qtestmouse.h>

#include <konqframe.h>
#include <konqmainwindow.h>
#include <konqviewmanager.h>
#include <konqview.h>
#include <konqtabs.h>
#include <konqframevisitor.h>
#include <konqsessionmanager.h>
#include <kstandarddirs.h>
#include <kconfiggroup.h>
#include <kio/job.h>
#include <QLayout>

#include <khtml_part.h>
#include <khtmlview.h>
#include <dom/html_inline.h>
#include <dom/html_document.h>

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

void ViewMgrTest::sendAllPendingResizeEvents( QWidget* mainWindow )
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

        if (!foundOne) { // about to exit, reset visible flag, to avoid crashes in qt
            foreach( QWidget* w, allChildWidgets ) {
                w->setAttribute(Qt::WA_WState_Visible, false);
            }
        }
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
    KonqSessionManager::self()->disableAutosave();
    QCOMPARE(KGlobal::mainComponent().componentName(), QString("konqueror"));
    QCOMPARE(KonqSettings::mmbOpensTab(), true);
    QCOMPARE(KonqSettings::popupsWithinTabs(), false);
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
    QVERIFY(view->frame()->parentContainer()->frameType() == KonqFrameBase::Container);
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
    int widthFrame2 = frame2->width();
    KonqView* view3 = viewManager->splitView( view, Qt::Horizontal );
    QVERIFY( view3 );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(C(FF)F)].") );
    // Check that the width of the second frame has not changed (bug 160407)
    QCOMPARE( frame2->width(), widthFrame2 );

    // Now test removing the first view
    viewManager->removeView( view );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)].") ); // mainWindow, tab widget, one splitter, two frames
    // Check again that the width of the second frame has not changed (bug 160407 comments 18-20)
    QCOMPARE( frame2->width(), widthFrame2 );

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
    QVERIFY(view->frame()->parentContainer()->frameType() == KonqFrameBase::Tabs);
    QVERIFY(view2->frame()->parentContainer()->frameType() == KonqFrameBase::Container);
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

static void openHtmlWithLink(KonqMainWindow& mainWindow)
{
    // Much like KonqHtmlTest::loadSimpleHtml.
    mainWindow.openUrl(0, KUrl("data:text/html, <a href=\"data:text/html, Link target\">Click me</a>"), "text/html");
    KonqView* view = mainWindow.currentView();
    QVERIFY(view);
    QVERIFY(QTest::kWaitForSignal(view, SIGNAL(viewCompleted(KonqView*)), 20000));
    QCOMPARE(view->serviceType(), QString("text/html"));
}

void ViewMgrTest::testLinkedViews()
{
    KonqMainWindow mainWindow;
    openHtmlWithLink(mainWindow);
    KonqView* view = mainWindow.currentView();
    // Split it
    qDebug() << "SPLITTING";
    mainWindow.slotSplitViewHorizontal();
    KonqView* view2 = mainWindow.currentView();
    QVERIFY( view2 );
    QCOMPARE(view2->serviceType(), QString("text/html"));
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)].") ); // mainWindow, tab widget, one splitter, two frames
    QVERIFY(QTest::kWaitForSignal(view2, SIGNAL(viewCompleted(KonqView*)), 20000));
    KUrl origUrl = view->url();
    QCOMPARE(view2->url().url(), origUrl.url());
    view->setLinkedView(true);
    view2->setLinkedView(true);
    view->setLockedLocation(true);
    // "Click" on the link
    qDebug() << "ACTIVATING LINK";
    KHTMLPart* part = qobject_cast<KHTMLPart *>(view->part());
    DOM::HTMLAnchorElement anchor = part->htmlDocument().getElementsByTagName(DOM::DOMString("a")).item(0);
    Q_ASSERT(!anchor.isNull());
    anchor.focus();
    QKeyEvent ev( QKeyEvent::KeyPress, Qt::Key_Return, 0, "\n" );
    QApplication::sendEvent( part->view(), &ev );
    qApp->processEvents(); // openUrlRequestDelayed
    // Check that the link opened in the 2nd view, not the first one
    QCOMPARE(view->url().url(), origUrl.url());
    QCOMPARE(view2->url().url(), KUrl("data:text/html, Link target").url());
}

void ViewMgrTest::testPopupNewTab() // RMB, "Open in new tab"
{
    KonqMainWindow mainWindow;
    openHtmlWithLink(mainWindow);
    KFileItem item(KUrl("data:text/html, hello"), "text/html", S_IFREG);
    mainWindow.prepareForPopupMenu(KFileItemList() << item, KParts::OpenUrlArguments(), KParts::BrowserArguments());
    QMetaObject::invokeMethod(&mainWindow, "slotPopupNewTab");
    QTest::qWait(1000);
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[FF].")); // mainWindow, tab widget, two tabs
    QCOMPARE(KMainWindow::memberList().count(), 1);
}

static void checkSecondWindowHasOneTab() // and delete it.
{
    QCOMPARE(KMainWindow::memberList().count(), 2);
    KonqMainWindow* newWindow = qobject_cast<KonqMainWindow*>(KMainWindow::memberList().last());
    QVERIFY(newWindow);
    QCOMPARE(DebugFrameVisitor::inspect(newWindow), QString("MT[F].")); // mainWindow, tab widget, one tab
    delete newWindow;
}

void ViewMgrTest::testPopupNewWindow() // RMB, "Open new window"
{
    KonqMainWindow mainWindow;
    openHtmlWithLink(mainWindow);
    KFileItem item(KUrl("data:text/html, hello"), "text/html", S_IFREG);
    mainWindow.prepareForPopupMenu(KFileItemList() << item, KParts::OpenUrlArguments(), KParts::BrowserArguments());
    QMetaObject::invokeMethod(&mainWindow, "slotPopupNewWindow");
    QTest::qWait(100);
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].")); // mainWindow, tab widget, one tab
    QVERIFY(KMainWindow::memberList().last() != &mainWindow);
    checkSecondWindowHasOneTab();
}

void ViewMgrTest::testCtrlClickOnLink()
{
    KonqMainWindow mainWindow;
    openHtmlWithLink(mainWindow);
    KonqView* view = mainWindow.currentView();
    KHTMLPart* part = qobject_cast<KHTMLPart *>(view->part());
    qDebug() << "CLICKING NOW";
    QTest::mouseClick(part->view()->widget(), Qt::LeftButton, Qt::ControlModifier, QPoint(10, 10));
    QTest::qWait(100);
    // Expected behavior for Ctrl+click:
    //  new tab, if mmbOpensTab
    //  new window, if !mmbOpensTab
    if (KonqSettings::mmbOpensTab()) {
        QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[FF].")); // mainWindow, tab widget, two tabs
        QCOMPARE(KMainWindow::memberList().count(), 1);
    } else {
        QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].")); // mainWindow, tab widget, one tab
        checkSecondWindowHasOneTab();
    }
}

void ViewMgrTest::sameTestsWithMmbOpenTabsFalse()
{
    // Redo testPopupNewTab, testPopupNewWindow and testCtrlClickOnLink,
    // but as if the user (e.g. Pino) had disabled the setting "Open links in new tabs".
    KonqSettings::setMmbOpensTab(false);
    testPopupNewTab();
    testPopupNewWindow();
    testCtrlClickOnLink();
    KonqSettings::setMmbOpensTab(true);
}

static void openTabWithTitle(KonqMainWindow& mainWindow, const QString& title, KonqView*& view)
{
    KonqViewManager* viewManager = mainWindow.viewManager();
    view = viewManager->addTab("text/html");
    QVERIFY( view );
    QVERIFY(view->supportsMimeType("text/html"));
    QVERIFY(!view->supportsMimeType("text/plain"));
    // correct since it's a subclass of text/html, khtml can display it
    QVERIFY(view->supportsMimeType("application/x-netscape-bookmarks"));
    // Tab caption test
    view->openUrl(KUrl("data:text/html, <title>" + title.toUtf8() + "</title>"), QString("http://loc.bar.url"));
    QVERIFY(QTest::kWaitForSignal(view, SIGNAL(viewCompleted(KonqView*)), 10000));
    QCOMPARE(view->caption(), title);
    QCOMPARE(view->locationBarURL(), QString("http://loc.bar.url"));
}

void ViewMgrTest::testAddTabs()
{
    KonqMainWindow mainWindow;
    KonqViewManager* viewManager = mainWindow.viewManager();

    KonqView* view = viewManager->createFirstView( "KonqAboutPage", "konq_aboutpage" );
    QVERIFY( view );
    QStringList titles;
    // The testcase was "konqueror www.kde.org www.google.fr bugs.kde.org www.cuil.com www.davidfaure.fr"
    titles << "K Desktop Environment - Be free"
           << "Google"
           << "KDE Bug Tracking System"
           << "Cuil"
           << "http://www.davidfaure.fr/";
    view->setCaption(titles[0]);

    KTabWidget* tabWidget = mainWindow.findChild<KTabWidget*>();
    QVERIFY(tabWidget);
    KonqView* viewTab2, *viewTab3, *viewTab4, *viewTab5;
    openTabWithTitle(mainWindow, titles[1], viewTab2);
    openTabWithTitle(mainWindow, titles[2], viewTab3);
    openTabWithTitle(mainWindow, titles[3], viewTab4);
    openTabWithTitle(mainWindow, titles[4], viewTab5);
    for (int i = 0; i < titles.count(); ++i)
        QCOMPARE(tabWidget->tabText(i), QString(titles[i]));

    // Ensure tabwidget has a nice size
    mainWindow.resize(599, 699);
    sendAllPendingResizeEvents( &mainWindow );

    // Remove active tab (#170470)
    tabWidget->setCurrentIndex(2);
    KonqFrameBase* frame = dynamic_cast<KonqFrameBase*>(viewManager->tabContainer()->currentWidget());
    QVERIFY(frame);
    viewManager->removeTab(frame);
    QList<int> expectedTitles; expectedTitles << 0 << 1 << 3 << 4;
    for (int i = 0; i < expectedTitles.count(); ++i)
        QCOMPARE(tabWidget->tabText(i), titles[expectedTitles[i]]);
    for (int i = 0; i < expectedTitles.count(); ++i)
        QCOMPARE(tabWidget->QTabWidget::tabText(i).left(10), titles[expectedTitles[i]].left(10));

    tabWidget->removeTab(0);
    expectedTitles.removeAt(0);
    for (int i = 0; i < expectedTitles.count(); ++i)
        QCOMPARE(tabWidget->tabText(i), QString(titles[expectedTitles[i]]));
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
    QVERIFY(container->parentContainer()->frameType() == KonqFrameBase::Tabs);

    viewManager->duplicateTab(container); // TODO shouldn't it return a KonqFrameBase?
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)C(FF)].") ); // mainWindow, tab widget, two tabs

    viewManager->removeTab(container);
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)].") ); // mainWindow, tab widget, one tab
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

static void loadFileManagementProfile(KonqMainWindow& mainWindow)
{
    const QString profile = KStandardDirs::locate("data", "konqueror/profiles/filemanagement");
    QVERIFY(!profile.isEmpty());
    const QString path = QDir::homePath();
    mainWindow.viewManager()->loadViewProfileFromFile(profile, "filemanagement", KUrl(path));
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MC(FT[F]).") ); // mainWindow, splitter, frame, tab widget, one frame
    QCOMPARE(mainWindow.locationBarURL(), path);
    QCOMPARE(mainWindow.currentView()->locationBarURL(), path);
}

void ViewMgrTest::testLoadProfile()
{
    KonqMainWindow mainWindow;
    loadFileManagementProfile(mainWindow);

    sendAllPendingResizeEvents( &mainWindow );

    QVERIFY(mainWindow.width() > 680);
    // QCOMPARE(frameType,QByteArray("Container")) leads to unreadable output on a mismatch :)
    QCOMPARE(mainWindow.childFrame()->frameType(), KonqFrameBase::Container);
    KonqFrameContainer* container = static_cast<KonqFrameContainer *>(mainWindow.childFrame());
    KonqFrameBase* firstChild = container->firstChild();
    QWidget* sidebarFrame = container->widget(0);
    QCOMPARE(firstChild->asQWidget(), sidebarFrame);
    QCOMPARE(firstChild->frameType(), KonqFrameBase::View);
    //QVERIFY(qobject_cast<KonqFrame*>(sidebarFrame));
    KonqFrameBase* secondChild = container->secondChild();
    QCOMPARE(secondChild->frameType(), KonqFrameBase::Tabs);
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

void ViewMgrTest::testLoadOldProfile()
{
    KonqMainWindow mainWindow;

    const QString profileSrc = KDESRCDIR "/filemanagement.old.profile";
    const QString profile = profileSrc + ".copy";
    // KonqViewManager fixes up the old profile, so let's make a copy of it first.
    KIO::FileCopyJob* job = KIO::file_copy(profileSrc, profile, -1, KIO::Overwrite);
    QVERIFY(job->exec());
    const QString path = QDir::homePath();
    mainWindow.viewManager()->loadViewProfileFromFile(profile, "filemanagement", KUrl(path));
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MC(FT[F]).") ); // mainWindow, splitter, frame, tab widget, one frame
    QCOMPARE(mainWindow.locationBarURL(), path);
    QCOMPARE(mainWindow.currentView()->locationBarURL(), path);
    QFile::remove(profile);
}


void ViewMgrTest::testDuplicateWindow()
{
    KonqMainWindow mainWindow;
    mainWindow.openUrl(0, KUrl("data:text/html, <p>Hello World</p>"), "text/html");
    KonqViewManager* viewManager = mainWindow.viewManager();
    KonqView* viewTab2 = viewManager->addTab("text/html");
    KonqView* splitted = viewManager->splitView( viewTab2, Qt::Horizontal );
    Q_UNUSED(splitted);
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[FC(FF)].") ); // mainWindow, tab widget, first tab = one frame, second tab = splitter with two frames
    KonqMainWindow* secondWindow = viewManager->duplicateWindow();
    QCOMPARE( DebugFrameVisitor::inspect(secondWindow), QString("MT[FC(FF)].") ); // mainWindow, tab widget, first tab = one frame, second tab = splitter with two frames
    delete secondWindow;
}

void ViewMgrTest::testCloseOtherTabs()
{
    KonqMainWindow mainWindow;
    mainWindow.openUrl(0, KUrl("data:text/html, <p>Hello World</p>"), "text/html");
    KonqViewManager* viewManager = mainWindow.viewManager();
    viewManager->addTab("text/html");
    KonqView* viewTab2 = viewManager->addTab("text/html");
    viewManager->splitView( viewTab2, Qt::Horizontal );
    viewManager->addTab("text/html");
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[FFC(FF)F].") ); // mainWindow, tab widget, first tab = one frame, second tab = one frame, third tab = splitter with two frames, fourth tab = one frame
    mainWindow.setWorkingTab(viewTab2->frame()); // This actually sets a frame which is not a tab, but KonqViewManager::removeOtherTabs should be able to deal with these cases
    mainWindow.slotRemoveOtherTabsPopupDelayed();
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)].") ); // mainWindow, tab widget, first tab = splitter with two frames
}

void ViewMgrTest::testDuplicateWindowWithSidebar()
{
    KonqMainWindow mainWindow;
    loadFileManagementProfile(mainWindow);
    KonqViewManager* viewManager = mainWindow.viewManager();
    KonqMainWindow* secondWindow = viewManager->duplicateWindow();
    QCOMPARE( DebugFrameVisitor::inspect(secondWindow), QString("MC(FT[F]).") ); // mainWindow, splitter, frame, tab widget, one frame
    delete secondWindow;
}

void ViewMgrTest::testBrowserArgumentsNewTab()
{
    KonqMainWindow mainWindow;
    mainWindow.openUrl(0, KUrl("data:text/html, <p>Hello World</p>"), "text/html");
    KParts::OpenUrlArguments urlArgs;
    KParts::BrowserArguments browserArgs;
    browserArgs.setNewTab(true);
    KonqView* view = mainWindow.currentView();
    KParts::BrowserExtension* ext = view->browserExtension();
    QVERIFY(ext);
    emit ext->openUrlRequest(KUrl("data:text/html, <p>Second tab test</p>"), urlArgs, browserArgs);
    QTest::qWait(5000);
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[FF].")); // mainWindow, tab widget, two tabs
    QCOMPARE(view->url(), KUrl("data:text/html, <p>Hello World</p>"));

    // compare the url of the new view... how to?
//    QCOMPARE(view->url(), KUrl("data:text/html, <p>Second tab test</p>"));
}

void ViewMgrTest::testBreakOffTab()
{
    KonqMainWindow mainWindow;
    KonqViewManager* viewManager = mainWindow.viewManager();
    KonqView* view = viewManager->createFirstView( "KonqAboutPage", "konq_aboutpage" );

    KonqFrameBase* tab = view->frame();
    viewManager->duplicateTab( tab );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[FF].") ); // mainWindow, tab widget, two tabs

    // Break off a tab

    KonqMainWindow* mainWindow2 = viewManager->breakOffTab( tab, mainWindow.size() );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].") ); // mainWindow, one tab, one frame
    QCOMPARE( DebugFrameVisitor::inspect(mainWindow2), QString("MT[F].") ); // mainWindow, one tab, one frame

    // Verify that the new tab container has an active child and that duplicating the tab in the new window does not crash (bug 203069)

    QVERIFY( mainWindow2->viewManager()->tabContainer()->activeChild() );
    mainWindow2->viewManager()->duplicateTab( mainWindow2->activeChildView()->frame() );
    QCOMPARE( DebugFrameVisitor::inspect(mainWindow2), QString("MT[FF].") ); // mainWindow, tab widget, two tabs

    delete mainWindow2;
    
    // Now split the remaining view, duplicate the tab and verify that breaking off a split tab does not crash (bug 174292).
    // Also check that the tab container of the new main window has an active child.

    view = mainWindow.activeChildView();
    viewManager->splitView( view, Qt::Vertical );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)].") ); // mainWindow, tab widget, one splitter, two frames
    KonqFrameContainerBase* container = view->frame()->parentContainer();
    viewManager->duplicateTab( container );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)C(FF)].") ); // mainWindow, tab widget, two tabs with split views
    mainWindow2 = viewManager->breakOffTab( container, mainWindow.size() );
    QCOMPARE( DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)].") ); // mainWindow, tab widget, one splitter, two frames
    QCOMPARE( DebugFrameVisitor::inspect(mainWindow2), QString("MT[C(FF)].") ); // mainWindow, tab widget, one splitter, two frames
    QVERIFY( mainWindow2->viewManager()->tabContainer()->activeChild() );

    delete mainWindow2;

    // Verify that breaking off a tab preserves the view profile (bug 210686)
    
    const QString profile = KStandardDirs::locate("data", "konqueror/profiles/webbrowsing");
    QVERIFY(!profile.isEmpty());
    const QString path = QDir::homePath();
    mainWindow.viewManager()->loadViewProfileFromFile(profile, "webbrowsing");
    view = mainWindow.activeChildView();
    tab = view->frame();
    viewManager->duplicateTab( tab );
    mainWindow2 = viewManager->breakOffTab( tab, mainWindow.size() );
    QCOMPARE( viewManager->currentProfile(), mainWindow2->viewManager()->currentProfile() );

    delete mainWindow2;
}

#include "konqviewmgrtest.moc"
