/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2006 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqviewmgrtest.h"
#include <konqmainwindowfactory.h>
#include "../src/konqsettingsxt.h"
#include <QToolBar>
#include <QProcess>
#include <QScrollArea>
#include <qtestkeyboard.h>
#include <qtest_gui.h>
#include <qtestmouse.h>
#include <QLabel>

#include <konqframe.h>
#include <konqmainwindow.h>
#include <konqviewmanager.h>
#include <konqview.h>
#include <konqtabs.h>
#include <konqframevisitor.h>
#include <konqsessionmanager.h>
#include <kconfiggroup.h>
#include <kio/job.h>
#include <ksycoca.h>
#include <KLocalizedString>

//#define TEST_KHTML 1

#ifdef TEST_KHTML
#include <khtml_part.h>
#include <khtmlview.h>
#else
#include <webenginepart.h>
#include <webengineview.h>
#include "../webenginepart/autotests/webengine_testutils.h"
#endif

#include <QStandardPaths>
#include <KSharedConfig>
#include <QSignalSpy>

QTEST_MAIN(ViewMgrTest)

#if 0
// could be used to load dummy parts; or to check that the right parts are being loaded
// (and to detect the case where a part is loaded and then replaced with another one for no good reason)
class KonqTestFactory : public KonqAbstractFactory
{
public:
    virtual KonqViewFactory createView(const QString &serviceType,
                                       const QString &serviceName = QString(),
                                       KService::Ptr *serviceImpl = 0,
                                       KService::List *partServiceOffers = 0,
                                       KService::List *appServiceOffers = 0,
                                       bool forceAutoEmbed = false);

};
#endif


// Return the main widget for the given KonqView; used for clicking onto it
// Duplicated from konqhtmltest.cpp -> move to KonqView?
static QWidget *partWidget(KonqView *view)
{
    QWidget *widget = view->part()->widget();
#ifdef TEST_KHTML
    KHTMLPart *htmlPart = qobject_cast<KHTMLPart *>(view->part());
#else
    WebEnginePart *htmlPart = qobject_cast<WebEnginePart *>(view->part());
#endif
    if (htmlPart) {
        widget = htmlPart->view();    // khtmlview != widget() nowadays, due to find bar
    }
    if (QScrollArea *scrollArea = qobject_cast<QScrollArea *>(widget)) {
        widget = scrollArea->widget();
    }
    if (widget && widget->focusProxy()) { // for WebEngine's RenderWidgetHostViewQtDelegateWidget
        return widget->focusProxy();
    }
    return widget;
}

void ViewMgrTest::sendAllPendingResizeEvents(QWidget *mainWindow)
{
    bool foundOne = true;
    while (foundOne) {
        foundOne = false;
        QList<QWidget *> allChildWidgets = mainWindow->findChildren<QWidget *>();
        allChildWidgets.prepend(mainWindow);
        foreach (QWidget *w, allChildWidgets) {
            if (w->testAttribute(Qt::WA_PendingResizeEvent)) {
                w->setAttribute(Qt::WA_WState_Created, true); // hack: avoid assert in Qt-4.6
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
            foreach (QWidget *w, allChildWidgets) {
                w->setAttribute(Qt::WA_WState_Visible, false);
            }
        }
    }
}

class DebugFrameVisitor : public KonqFrameVisitor
{
public:
    DebugFrameVisitor() {}
    QString output() const
    {
        return m_output;
    }
    bool visit(KonqFrame *) override
    {
        m_output += 'F';
        return true;
    }
    bool visit(KonqFrameContainer *) override
    {
        m_output += QLatin1String("C(");
        return true;
    }
    bool visit(KonqFrameTabs *) override
    {
        m_output += QLatin1String("T[");
        return true;
    }
    bool visit(KonqMainWindow *) override
    {
        m_output += 'M';
        return true;
    }
    bool endVisit(KonqFrameContainer *) override
    {
        m_output += ')';
        return true;
    }
    bool endVisit(KonqFrameTabs *) override
    {
        m_output += ']';
        return true;
    }
    bool endVisit(KonqMainWindow *) override
    {
        m_output += '.';
        return true;
    }

    static QString inspect(KonqMainWindow *mainWindow)
    {
        DebugFrameVisitor dfv;
        bool ok = mainWindow->accept(&dfv);
        if (!ok) {
            return QStringLiteral("ERROR: visitor returned false");
        }
        return dfv.output();
    }

private:
    QString m_output;
};

void ViewMgrTest::initTestCase()
{
    KLocalizedString::setApplicationDomain("konqviewmgrtest");

    QStandardPaths::setTestModeEnabled(true);
    QDir(KonqSessionManager::self()->autosaveDirectory()).removeRecursively();
    QString configLocationDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    const QLatin1String expConfigDir(".qttest/config");
    QString msg = QString("Can't remove the config file because it isn't in the %1 directory but in %2").arg(expConfigDir).arg(configLocationDir);
    QVERIFY2(configLocationDir.endsWith(expConfigDir), msg.toLatin1());
    QDir(configLocationDir).remove("konquerorrc");

    KonqSessionManager::self()->disableAutosave();
    QCOMPARE(KonqSettings::mmbOpensTab(), true);
    QCOMPARE(KonqSettings::popupsWithinTabs(), false);
    KonqSettings::setAlwaysHavePreloaded(false); // it would confuse the mainwindow counting

    // Ensure the tests use webenginepart (not khtml or webkit)
    // This code is inspired by settings/konqhtml/generalopts.cpp
    KSharedConfig::Ptr profile = KSharedConfig::openConfig(QStringLiteral("mimeapps.list"), KConfig::NoGlobals, QStandardPaths::ApplicationsLocation);
    KConfigGroup addedServices(profile, "Added KDE Service Associations");
    bool needsUpdate = false;
    for (const QString &mimeType : QStringList{"text/html", "application/xhtml+xml", "application/xml"}) {
        QStringList services = addedServices.readXdgListEntry(mimeType);
        const QString wanted = QStringLiteral("webenginepart.desktop");
        if (services.isEmpty() || services.at(0) != wanted) {
            services.removeAll(wanted);
            services.prepend(wanted); // make it the preferred one
            addedServices.writeXdgListEntry(mimeType, services);
            needsUpdate = true;
        }
    }
    if (needsUpdate) {
        profile->sync();
        // kbuildsycoca is the one reading mimeapps.list, so we need to run it now
        QProcess::execute(QStandardPaths::findExecutable(KBUILDSYCOCA_EXENAME), {});
    }
}

void ViewMgrTest::testCreateFirstView()
{
    KonqMainWindow mainWindow;
    KonqViewManager *viewManager = mainWindow.viewManager();
    KonqView *view = viewManager->createFirstView(QStringLiteral("text/html"), QStringLiteral("webenginepart"));
    QVERIFY(view);
    QVERIFY(viewManager->tabContainer());

    // Use DebugFrameVisitor to find out the structure of the frame hierarchy
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[F]."));   // mainWindow, one tab, one frame
    QCOMPARE(mainWindow.linkableViewsCount(), 1);

    // Check widget parents:  part's widget -> frame -> [tabwidget's stack] -> tabwidget -> mainWindow
    QWidget *partWidget = view->part()->widget();
    QCOMPARE(partWidget->window(), &mainWindow);
    QWidget *frame = view->frame()->asQWidget();
    QCOMPARE(partWidget->parentWidget(), frame);
    QWidget *tabWidget = viewManager->tabContainer()->asQWidget();
    QCOMPARE(frame->parentWidget()->parentWidget(), tabWidget);

    // Check frame geometry, to check that all layouts are there
    // (the mainWindow is resized to 700x480 in its constructor)
    // But pending resize events are only sent by show(), and we don't want to see
    // widgets from unit tests.
    // So we iterate over all widgets and ensure the pending resize events are sent.
    sendAllPendingResizeEvents(&mainWindow);
    for (QWidget *w = partWidget; w; w = w->parentWidget()) {
        qDebug() << w << w->geometry();
    }
    //const QList<QToolBar*> toolbars = mainWindow.findChildren<QToolBar *>();
    //foreach( QToolBar* toolbar, toolbars ) {
    //    if (!toolbar->isHidden())
    //        qDebug() << toolbar << toolbar->geometry();
    //}
    //qDebug() << "frame geom: " << frame->geometry();
    QVERIFY(frame->width() > 680);
    QVERIFY(frame->height() > 240);   // usually 325, but can be 256 with oxygen when three toolbars are shown
    //qDebug() << "partWidget geom:" << partWidget->geometry();
    QVERIFY(partWidget->width() > 680);
    QVERIFY(partWidget->height() > frame->height() - 50 /*statusbar*/);
    //qDebug() << "tabWidget geom: " << tabWidget->geometry();
    QVERIFY(tabWidget->width() > 680);
    QVERIFY(tabWidget->height() >= frame->height());   // equal, unless there's a border in the frame

    // Part widget should have focus, not location bar
    QCOMPARE(mainWindow.focusWidget()->metaObject()->className(), partWidget->focusWidget()->metaObject()->className());
}

void ViewMgrTest::testEmptyWindow()
{
    QScopedPointer<KonqMainWindow> emptyWindow(KonqMainWindowFactory::createNewWindow());
    QCOMPARE(emptyWindow->currentView()->url().url(), QString("konq:konqueror"));
    QCOMPARE(emptyWindow->focusWidget()->metaObject()->className(), "KonqCombo");
}

void ViewMgrTest::testRemoveFirstView()
{
    KonqMainWindow mainWindow;
    KonqViewManager *viewManager = mainWindow.viewManager();
    KonqView *view = viewManager->createFirstView(QStringLiteral("text/html"), QStringLiteral("webenginepart"));
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[F]."));   // mainWindow, tab widget, one frame
    viewManager->removeView(view);
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[F]."));   // removing not allowed
    // real test for removeView is part of testSplitView
}

void ViewMgrTest::testSplitView()
{
    KonqMainWindow mainWindow;
    KonqViewManager *viewManager = mainWindow.viewManager();
    KonqView *view = viewManager->createFirstView(QStringLiteral("text/html"), QStringLiteral("webenginepart"));

    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[F]."));   // mainWindow, tab widget, one frame
    // mainWindow.show();
    // qApp->processEvents();
    KonqView *view2 = viewManager->splitView(view, Qt::Horizontal);
    QVERIFY(view2);
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)]."));   // mainWindow, tab widget, one splitter, two frames

    QCOMPARE(mainWindow.linkableViewsCount(), 2);

    // Check widget parents
    //mainWindow.dumpObjectTree();
    QWidget *partWidget = view->part()->widget();
    QCOMPARE(partWidget->window(), &mainWindow);
    QWidget *frame = view->frame()->asQWidget();
    QCOMPARE(partWidget->parentWidget(), frame);
    QVERIFY(!frame->isHidden());

    QWidget *part2Widget = view2->part()->widget();
    QCOMPARE(part2Widget->window(), &mainWindow);
    QWidget *frame2 = view2->frame()->asQWidget();
    QCOMPARE(part2Widget->parentWidget(), frame2);
    QVERIFY(!frame2->isHidden());

    // Check container
    QCOMPARE(view->frame()->parentContainer()->frameType(), KonqFrameBase::Container);
    KonqFrameContainer *container = static_cast<KonqFrameContainer *>(view->frame()->parentContainer());
    QVERIFY(container);
    QCOMPARE(container->count(), 2);
    QCOMPARE(container, view2->frame()->parentContainer());
    QCOMPARE(container->firstChild(), view->frame());
    QCOMPARE(container->secondChild(), view2->frame());
    QCOMPARE(container->widget(0), view->frame()->asQWidget());
    QCOMPARE(container->widget(1), view2->frame()->asQWidget());

    // mainWindow.show();
    // qApp->processEvents();
    // Check frame geometries
    sendAllPendingResizeEvents(&mainWindow);
    //for ( QWidget* w = partWidget; w; w = w->parentWidget() )
    //    qDebug() << w << w->geometry() << "visible:" << w->isVisible();

    //qDebug() << "view geom:" << frame->geometry();
    QVERIFY(frame->width() > 300 && frame->width() < 400);   // horiz split, so half the mainWindow width
    QVERIFY(frame->height() > 240);   // usually 325, but can be 256 with oxygen when three toolbars are shown
    //qDebug() << "view2 geom:" << frame2->geometry();
    QVERIFY(frame2->width() > 300 && frame2->width() < 400);   // horiz split, so half the mainWindow width
    QVERIFY(frame2->height() > 240);   // usually 325, but can be 256 with oxygen when three toolbars are shown
    // Both frames should have the same size; well, if the width was odd then there can be an off-by-one...
    QCOMPARE(frame->height(), frame2->height());
    // qDebug() << "FRAME WIDTH" << frame->width() << "FRAME2 WIDTH" << frame2->width();
    // qDebug() << "FRAME MINIMUM WIDTH" << frame->minimumSizeHint() << "FRAME2 MINIMIUM WIDTH" << frame2->minimumSizeHint();
    QVERIFY(qAbs(frame->width() - frame2->width()) <= 1);   // e.g. 173 and 172 are "close enough"
    //qDebug() << "partWidget geom:" << partWidget->geometry();
    QVERIFY(partWidget->width() > 300 && partWidget->width() < 400);   // horiz split, so half the mainWindow width
    QVERIFY(partWidget->height() > 220);   // frame minus statusbar height
    QVERIFY(part2Widget->width() > 300 && part2Widget->width() < 400);   // horiz split, so half the mainWindow width
    QVERIFY(part2Widget->height() > 220);

    //KonqFrameContainerBase* container = view->frame()->parentContainer();
    //QVERIFY( container );
    //qDebug() << "container geom: " << container->asQWidget()->geometry();

    // Split again
    int widthFrame2 = frame2->width();
    KonqView *view3 = viewManager->splitView(view, Qt::Horizontal);
    QVERIFY(view3);
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(C(FF)F)]."));
    // Check that the width of the second frame has not changed (bug 160407)
    QCOMPARE(frame2->width(), widthFrame2);
    QCOMPARE(mainWindow.linkableViewsCount(), 3);

    // Now test removing the first view
    viewManager->removeView(view);
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)]."));   // mainWindow, tab widget, one splitter, two frames
    // Check again that the width of the second frame has not changed (bug 160407 comments 18-20)
    QCOMPARE(frame2->width(), widthFrame2);
    QCOMPARE(mainWindow.linkableViewsCount(), 2);

    // Now test removing the last view
    viewManager->removeView(view3);
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[F]."));   // mainWindow, tab widget, one frame
    QCOMPARE(mainWindow.linkableViewsCount(), 1);
}

void ViewMgrTest::testSplitMainContainer()
{
    KonqMainWindow mainWindow;
    KonqViewManager *viewManager = mainWindow.viewManager();
    KonqView *view = viewManager->createFirstView(QStringLiteral("text/html"), QStringLiteral("webenginepage"));
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[F]."));   // mainWindow, tab widget, one frame
    KonqFrameContainerBase *tabContainer = view->frame()->parentContainer();
    KonqView *view2 = viewManager->splitMainContainer(view, Qt::Horizontal, QStringLiteral("text/html"), QStringLiteral("webenginepart"), true);
    QVERIFY(view2);
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MC(FT[F])."));   // mainWindow, splitter, frame, tab widget, one frame

    // Check widget parents
    QWidget *partWidget = view->part()->widget();
    QCOMPARE(partWidget->window(), &mainWindow);
    QWidget *frame = view->frame()->asQWidget();
    QCOMPARE(partWidget->parentWidget(), frame);
    QVERIFY(!frame->isHidden());

    QWidget *part2Widget = view2->part()->widget();
    QCOMPARE(part2Widget->window(), &mainWindow);
    QWidget *frame2 = view2->frame()->asQWidget();
    QCOMPARE(part2Widget->parentWidget(), frame2);
    QVERIFY(!frame2->isHidden());

    // Check container
    QCOMPARE(view->frame()->parentContainer()->frameType(), KonqFrameBase::Tabs);
    QCOMPARE(view2->frame()->parentContainer()->frameType(), KonqFrameBase::Container);
    KonqFrameContainer *container = static_cast<KonqFrameContainer *>(view2->frame()->parentContainer());
    QVERIFY(container);
    QCOMPARE(container->count(), 2);
    QCOMPARE(container, view2->frame()->parentContainer());
    QCOMPARE(container->firstChild(), view2->frame());
    QCOMPARE(container->secondChild(), tabContainer);
    QCOMPARE(container->widget(0), view2->frame()->asQWidget());
    QCOMPARE(container->widget(1), tabContainer->asQWidget());

    // Now test removing the view we added last
    viewManager->removeView(view2);
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[F]."));   // mainWindow, tab widget, one frame
}

static void openHtmlWithLink(KonqMainWindow &mainWindow)
{
    // Much like KonqHtmlTest::loadSimpleHtml.
    // We use text/plain as the linked file, in order to test #67956 (switching parts in new tab)
    mainWindow.openUrl(nullptr, QUrl(QStringLiteral("data:text/html, <a href=\"data:text/plain, Link target\" id=\"linkid\">Click me</a>")), QStringLiteral("text/html"));
    KonqView *view = mainWindow.currentView();
    QVERIFY(view);
    QSignalSpy spyCompleted(view, SIGNAL(viewCompleted(KonqView*)));
    QVERIFY(spyCompleted.wait(20000));
    QCOMPARE(view->serviceType(), QString("text/html"));
}

void ViewMgrTest::testLinkedViews()
{
    KonqMainWindow mainWindow;
    openHtmlWithLink(mainWindow);
    KonqView *view = mainWindow.currentView();
    WebEnginePart *part = qobject_cast<WebEnginePart *>(view->part());
    QVERIFY(part);
    // Split it
    qDebug() << "SPLITTING";
    mainWindow.slotSplitViewHorizontal();
    KonqView *view2 = mainWindow.currentView();
    QVERIFY(view2);
    QCOMPARE(view2->serviceType(), QString("text/html"));
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)]."));   // mainWindow, tab widget, one splitter, two frames
    QSignalSpy spyCompleted(view2, SIGNAL(viewCompleted(KonqView*)));
    QVERIFY(spyCompleted.wait(20000));
    const QUrl origUrl = view->url();
    QCOMPARE(view2->url().url(), origUrl.url());
    QCOMPARE(mainWindow.linkableViewsCount(), 2);
    view->setLinkedView(true);
    view2->setLinkedView(true);
    view->setLockedLocation(true);

    mainWindow.show();

    // "Click" on the link
    qDebug() << "ACTIVATING LINK";

#ifdef TEST_KHTML
    KHTMLPart *part = qobject_cast<KHTMLPart *>(view->part());
    QVERIFY(part);
    DOM::HTMLAnchorElement anchor = part->htmlDocument().getElementsByTagName(DOM::DOMString("a")).item(0);
    QVERIFY(!anchor.isNull());
    anchor.focus();
    QKeyEvent ev(QKeyEvent::KeyPress, Qt::Key_Return, 0, "\n");
    QApplication::sendEvent(part->view(), &ev);
    qApp->processEvents(); // openUrlRequestDelayed
    QTest::qWait(0);
#else
    QTest::mouseClick(part->view()->focusProxy(), Qt::LeftButton, Qt::KeyboardModifiers(),
            elementCenter(part->view()->page(), QStringLiteral("linkid")));

    // Check that the link opened in the 2nd view, not the first one
#endif
    QCOMPARE(view->url().url(), origUrl.url());
#ifndef TEST_KHTML
    QEXPECT_FAIL("", "Broken feature right now, requires URLs to be opened by konq rather than the part", Abort);
#endif
    QTRY_COMPARE_WITH_TIMEOUT(view2->url().url(), QUrl("data:text/plain, Link target").url(), 400);
}

void ViewMgrTest::testPopupNewTab() // RMB, "Open in new tab"
{
    KonqMainWindow mainWindow;
    openHtmlWithLink(mainWindow);
    KonqFrameTabs *tabs = mainWindow.viewManager()->tabContainer();
    QCOMPARE(tabs->currentIndex(), 0);
    KFileItem item(QUrl(QStringLiteral("data:text/html, hello")), QStringLiteral("text/html"), S_IFREG);
    mainWindow.prepareForPopupMenu(KFileItemList() << item, KParts::OpenUrlArguments(), KParts::BrowserArguments());
    QMetaObject::invokeMethod(&mainWindow, "slotPopupNewTab");
    QTest::qWait(1000);
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[FF].")); // mainWindow, tab widget, two tabs
    QCOMPARE(KMainWindow::memberList().count(), 1);
    QCOMPARE(mainWindow.linkableViewsCount(), 1);

    if (KonqSettings::newTabsInFront()) {
        QCOMPARE(tabs->currentIndex(), 1);
    }
}

static void checkSecondWindowHasOneTab(bool fromPopup) // and delete it.
{
    QTRY_COMPARE(KMainWindow::memberList().count(), 2);
    QScopedPointer<KonqMainWindow> newWindow(qobject_cast<KonqMainWindow *>(KMainWindow::memberList().last()));
    QVERIFY(newWindow.data());
    QTRY_COMPARE(DebugFrameVisitor::inspect(newWindow.data()), QString("MT[F].")); // mainWindow, tab widget, one tab
    QTabWidget *tabWidget = newWindow->findChild<QTabWidget *>();
    QVERIFY(tabWidget);
    // The location bar shouldn't get focus (#208821)
    QTRY_VERIFY(newWindow->focusWidget());
    QTRY_VERIFY_WITH_TIMEOUT(tabWidget->focusWidget(), 200);
    QCOMPARE(newWindow->focusWidget()->metaObject()->className(), tabWidget->focusWidget()->metaObject()->className());
}

void ViewMgrTest::testPopupNewWindow() // RMB, "Open new window"
{
    KonqMainWindow mainWindow;
    openHtmlWithLink(mainWindow);
    KFileItem item(QUrl(QStringLiteral("data:text/html, hello")), QStringLiteral("text/html"), S_IFREG);
    mainWindow.prepareForPopupMenu(KFileItemList() << item, KParts::OpenUrlArguments(), KParts::BrowserArguments());
    QMetaObject::invokeMethod(&mainWindow, "slotPopupNewWindow");
    QTRY_COMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].")); // mainWindow, tab widget, one tab
    QVERIFY(KMainWindow::memberList().last() != &mainWindow);
    checkSecondWindowHasOneTab(true);
}

void ViewMgrTest::testCtrlClickOnLink()
{
    KonqMainWindow mainWindow;
    openHtmlWithLink(mainWindow);
    KonqFrameTabs *tabs = mainWindow.viewManager()->tabContainer();
    KonqView *view = mainWindow.currentView();
    mainWindow.show();
    qDebug() << "CLICKING NOW";
    QTest::mouseClick(partWidget(view), Qt::LeftButton, Qt::ControlModifier, QPoint(10, 10));
    QTest::qWait(100);
    KonqView *newView = nullptr;
    // Expected behavior for Ctrl+click:
    //  new tab, if mmbOpensTab
    //  new window, if !mmbOpensTab
    //  (this code is called for both cases)
    if (KonqSettings::mmbOpensTab()) {
        QCOMPARE(KMainWindow::memberList().count(), 1);
        QTRY_COMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[FF].")); // mainWindow, tab widget, two tabs
        if (KonqSettings::newTabsInFront()) { // when called by sameTestsWithNewTabsInFront
            QCOMPARE(tabs->currentIndex(), 1);
            QVERIFY(mainWindow.currentView() != view);
            QVERIFY(mainWindow.viewManager()->activePart() != view->part());
        } else {                              // the default case
            QCOMPARE(tabs->currentIndex(), 0);
            QCOMPARE(mainWindow.currentView(), view);
            QCOMPARE(mainWindow.viewManager()->activePart(), view->part());
        }
        KonqFrame *newFrame = dynamic_cast<KonqFrame *>(mainWindow.viewManager()->tabContainer()->tabAt(1));
        QVERIFY(newFrame);
        newView = newFrame->activeChildView();
        QVERIFY(newView);
        //QVERIFY(newView->supportsMimeType("text/plain"));
    } else {
        QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[F].")); // mainWindow, tab widget, one tab
        checkSecondWindowHasOneTab(false);
    }
}

void ViewMgrTest::sameTestsWithNewTabsInFront()
{
    // Redo testCtrlClickOnLink,
    // but with the option "NewTabsInFront" set to true.
    QVERIFY(!KonqSettings::newTabsInFront()); // default setting = false
    KonqSettings::setNewTabsInFront(true);
    testPopupNewTab();
    testCtrlClickOnLink();
    KonqSettings::setNewTabsInFront(false);
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

static void openTabWithTitle(KonqMainWindow &mainWindow, const QString &title, KonqView *&view)
{
    KonqViewManager *viewManager = mainWindow.viewManager();
    view = viewManager->addTab(QStringLiteral("text/html"));
    QVERIFY(view);
    QVERIFY(view->supportsMimeType("text/html"));
    QVERIFY(!view->supportsMimeType("text/plain"));
    // correct since it's a subclass of text/html, khtml can display it
    QVERIFY(view->supportsMimeType("application/x-netscape-bookmarks"));
    // Tab caption test
    view->openUrl(QUrl(QStringLiteral("data:text/html, <title>") + title + QStringLiteral("</title>")), QStringLiteral("http://loc.bar.url"));
    QSignalSpy spyCompleted(view, SIGNAL(viewCompleted(KonqView*)));
    QVERIFY(spyCompleted.wait(10000));
    QCOMPARE(view->caption(), title);
    QCOMPARE(view->locationBarURL(), QString("http://loc.bar.url"));
}

void ViewMgrTest::testAddTabs()
{
    KonqMainWindow mainWindow;
    KonqViewManager *viewManager = mainWindow.viewManager();

    KonqView *view = viewManager->createFirstView(QStringLiteral("text/html"), QStringLiteral("webenginepart"));
    QVERIFY(view);
    // The testcase was "konqueror www.kde.org www.google.fr bugs.kde.org www.cuil.com www.davidfaure.fr"
    const QStringList titles {
        "K Desktop Environment - Be free",
        "Google",
        "KDE Bug Tracking System",
        "Cuil",
        "http://www.davidfaure.fr/" };
    view->setCaption(titles.at(0));

    KTabWidget *tabWidget = mainWindow.findChild<KTabWidget *>();
    QVERIFY(tabWidget);
    KonqView *viewTab1, *viewTab2, *viewTab3, *viewTab4;
    openTabWithTitle(mainWindow, titles.at(1), viewTab1);
    openTabWithTitle(mainWindow, titles.at(2), viewTab2);
    openTabWithTitle(mainWindow, titles.at(3), viewTab3);
    openTabWithTitle(mainWindow, titles.at(4), viewTab4);
    for (int i = 0; i < titles.count(); ++i) {
        QCOMPARE(tabWidget->tabText(i), QString(titles.at(i)));
    }
    QPointer<KonqView> viewTab2Pointer(viewTab2);
    QPointer<KParts::ReadOnlyPart> tab2PartPointer(viewTab2->part());

    // Ensure tabwidget has a nice size
    mainWindow.resize(1000, 699);
    sendAllPendingResizeEvents(&mainWindow);

    // Remove active tab (#170470)
    tabWidget->setCurrentIndex(2);
    KonqFrameBase *frame = dynamic_cast<KonqFrameBase *>(viewManager->tabContainer()->currentWidget());
    QVERIFY(frame);
    viewManager->removeTab(frame);
    QVERIFY(viewTab2Pointer.isNull()); // check the view got deleted
    QVERIFY(tab2PartPointer.isNull()); // check the part got deleted too, since pino is a non-believer :)
    QList<int> expectedTitles{0, 1, 3, 4};
    for (int i = 0; i < expectedTitles.count(); ++i) {
        QCOMPARE(tabWidget->tabText(i), titles.at(expectedTitles[i]));
    }
    for (int i = 0; i < expectedTitles.count(); ++i) {
        QCOMPARE(tabWidget->QTabWidget::tabText(i).left(10), titles.at(expectedTitles[i]).left(10));
    }

    tabWidget->removeTab(0);
    expectedTitles.removeAt(0);
    for (int i = 0; i < expectedTitles.count(); ++i) {
        QCOMPARE(tabWidget->tabText(i), QString(titles.at(expectedTitles[i])));
    }
}

void ViewMgrTest::testDuplicateTab()
{
    KonqMainWindow mainWindow;
    KonqViewManager *viewManager = mainWindow.viewManager();
    /*KonqView* view =*/ viewManager->createFirstView(QStringLiteral("text/html"), QStringLiteral("webenginepart"));
    viewManager->duplicateTab(0); // should return a KonqFrameBase?

    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[FF]."));   // mainWindow, tab widget, two tabs
    // TODO check serviceType and serviceName of the new view
}

void ViewMgrTest::testDuplicateSplittedTab()
{
    KonqMainWindow mainWindow;
    KonqViewManager *viewManager = mainWindow.viewManager();
    KonqView *view = viewManager->createFirstView(QStringLiteral("text/html"), QStringLiteral("webenginepart"));
    KonqView *view2 = viewManager->splitView(view, Qt::Vertical);
    QVERIFY(view2);
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)]."));   // mainWindow, tab widget, one splitter, two frames

    KonqFrameContainer *container = static_cast<KonqFrameContainer *>(view->frame()->parentContainer());
    QVERIFY(container);
    QCOMPARE(container->parentContainer()->frameType(), KonqFrameBase::Tabs);

    viewManager->duplicateTab(0); // TODO shouldn't it return a KonqFrameBase?
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)C(FF)]."));   // mainWindow, tab widget, two tabs
    QCOMPARE(mainWindow.linkableViewsCount(), 2);

    // While we're here, let's test Ctrl+Tab navigation
    view->setProperty("num", 0);
    view2->setProperty("num", 1);
    KonqFrameTabs *tabs = viewManager->tabContainer();
    KonqFrameBase *tab2base = tabs->tabAt(1);
    QVERIFY(tab2base->isContainer());
    KonqFrameContainer *tab2 = dynamic_cast<KonqFrameContainer *>(tab2base);
    QVERIFY(tab2);
    KonqView *view3 = tab2->firstChild()->activeChildView();
    QVERIFY(view3);
    KonqView *view4 = tab2->secondChild()->activeChildView();
    QVERIFY(view4);
    QVERIFY(view3 != view);
    QVERIFY(view4 != view2);
    view3->setProperty("num", 2);
    view4->setProperty("num", 3);
    QCOMPARE(mainWindow.currentView()->property("num").toInt(), 3);
    QCOMPARE(tabs->currentIndex(), 1);
    QVERIFY(mainWindow.focusWidget());
    QTest::keyClick(mainWindow.focusWidget(), Qt::Key_Tab, Qt::ControlModifier);
    QCOMPARE(mainWindow.currentView()->property("num").toInt(), 0);
    QCOMPARE(tabs->currentIndex(), 0);
    QTest::keyClick(mainWindow.focusWidget(), Qt::Key_Tab, Qt::ControlModifier);
    QCOMPARE(mainWindow.currentView()->property("num").toInt(), 1);
    QCOMPARE(tabs->currentIndex(), 0);
    QTest::keyClick(mainWindow.focusWidget(), Qt::Key_Tab, Qt::ControlModifier);
    QCOMPARE(mainWindow.currentView()->property("num").toInt(), 2);
    QCOMPARE(tabs->currentIndex(), 1);
    QTest::keyClick(mainWindow.focusWidget(), Qt::Key_Tab, Qt::ControlModifier);
    QCOMPARE(mainWindow.currentView()->property("num").toInt(), 3);
    QCOMPARE(tabs->currentIndex(), 1);

    viewManager->removeTab(container);
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)]."));   // mainWindow, tab widget, one tab
    QCOMPARE(mainWindow.linkableViewsCount(), 2);
}

// Like in https://bugs.kde.org/show_bug.cgi?id=153533,
// where the part deletes itself.
void ViewMgrTest::testDeletePartInTab()
{
    QPointer<KonqMainWindow> mainWindow = new KonqMainWindow;
    QVERIFY(mainWindow->testAttribute(Qt::WA_DeleteOnClose));
    KonqViewManager *viewManager = mainWindow->viewManager();
    QPointer<KonqView> view = viewManager->createFirstView(QStringLiteral("text/html"), QStringLiteral("webenginepart"));
    QVERIFY(!view.isNull());
    QPointer<QWidget> partWidget = view->part()->widget();

    QPointer<KonqView> viewTab2 = viewManager->addTab(QStringLiteral("text/html"));
    QPointer<QWidget> partWidget2 = viewTab2->part()->widget();
    QVERIFY(!viewTab2.isNull());
    QCOMPARE(DebugFrameVisitor::inspect(mainWindow), QString("MT[FF]."));   // mainWindow, tab widget, two tabs

    delete viewTab2->part();
    QVERIFY(viewTab2.isNull());
    QVERIFY(partWidget2.isNull());
    QCOMPARE(DebugFrameVisitor::inspect(mainWindow), QString("MT[F]."));   // mainWindow, tab widget, one tab

    delete view->part();
    QVERIFY(view.isNull());
    QVERIFY(partWidget.isNull());
    qApp->sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(mainWindow.isNull());
}

void ViewMgrTest::testSaveProfile()
{
    KonqMainWindow mainWindow;
    const QUrl url(QStringLiteral("data:text/html, <p>Hello World</p>"));
    mainWindow.openUrl(nullptr, url, QStringLiteral("text/html"));
    KonqViewManager *viewManager = mainWindow.viewManager();
    KonqView *view2 = viewManager->addTab(QStringLiteral("text/html"));
    const QUrl url2(QStringLiteral("data:text/html, <p>view2</p>"));
    view2->openUrl(url2, QStringLiteral("2"));
    QTabWidget *tabWidget = mainWindow.findChild<QTabWidget *>();
    QVERIFY(tabWidget);

    // Save a profile with two tabs (via KonqSessionManager)
    KonqSessionManager *sessionMgr = KonqSessionManager::self();
    const QString filePath = QDir::currentPath() + "unittest_profile";
    sessionMgr->saveCurrentSessionToFile(filePath);
    QVERIFY(QFile::exists(filePath));

    {
        KConfig cfg(filePath, KConfig::SimpleConfig);
        KConfigGroup profileGroup(&cfg, "Window0");
        QCOMPARE(profileGroup.readEntry("RootItem"), QString("Tabs0"));
        QCOMPARE(profileGroup.readEntry("Tabs0_Children"), QString("ViewT0,ViewT1"));
        QCOMPARE(profileGroup.readEntry("HistoryItemViewT0_0Url"), url.url());
        QCOMPARE(profileGroup.readEntry("HistoryItemViewT1_0Url"), url2.url());
    }

    // Now close a tab and save again - to check that the stuff from the old
    // tab isn't lying around.
    viewManager->removeTab(view2->frame());
    sessionMgr->saveCurrentSessionToFile(filePath);
    {
        KConfig cfg(filePath, KConfig::SimpleConfig);
        KConfigGroup profileGroup(&cfg, "Window0");
        QCOMPARE(profileGroup.readEntry("RootItem"), QString("Tabs0"));
        QCOMPARE(profileGroup.readEntry("Tabs0_Children"), QString("ViewT0"));
        QCOMPARE(profileGroup.readEntry("HistoryItemViewT0_0Url"), url.url());
        QVERIFY(!profileGroup.hasKey("HistoryItemViewT1_0Url"));
    }
    QFile::remove(filePath);
}

void ViewMgrTest::testDuplicateWindow()
{
    KonqMainWindow mainWindow;
    mainWindow.openUrl(nullptr, QUrl(QStringLiteral("data:text/html, <p>Hello World</p>")), QStringLiteral("text/html"));
    KonqViewManager *viewManager = mainWindow.viewManager();
    KonqView *viewTab2 = viewManager->addTab(QStringLiteral("text/html"));
    KonqView *splitted = viewManager->splitView(viewTab2, Qt::Horizontal);
    Q_UNUSED(splitted);
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[FC(FF)]."));   // mainWindow, tab widget, first tab = one frame, second tab = splitter with two frames
    QScopedPointer<KonqMainWindow> secondWindow(viewManager->duplicateWindow());
    QCOMPARE(DebugFrameVisitor::inspect(secondWindow.data()), QString("MT[FC(FF)]."));   // mainWindow, tab widget, first tab = one frame, second tab = splitter with two frames
}

void ViewMgrTest::testCloseOtherTabs()
{
    KonqMainWindow mainWindow;
    mainWindow.openUrl(nullptr, QUrl(QStringLiteral("data:text/html, <p>Hello World</p>")), QStringLiteral("text/html"));
    KonqViewManager *viewManager = mainWindow.viewManager();
    QTabWidget *tabWidget = mainWindow.findChild<QTabWidget *>();
    QVERIFY(tabWidget);
    QVERIFY(tabWidget->focusWidget());
    QCOMPARE(mainWindow.focusWidget()->metaObject()->className(), tabWidget->focusWidget()->metaObject()->className());
    viewManager->addTab(QStringLiteral("text/html"));
    KonqView *viewTab2 = viewManager->addTab(QStringLiteral("text/html"));
    viewManager->splitView(viewTab2, Qt::Horizontal);
    viewManager->addTab(QStringLiteral("text/html"));
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[FFC(FF)F]."));   // mainWindow, tab widget, first tab = one frame, second tab = one frame, third tab = splitter with two frames, fourth tab = one frame
    QCOMPARE(tabWidget->count(), 4);
    tabWidget->setCurrentIndex(2);
    QCOMPARE(mainWindow.linkableViewsCount(), 2);
    tabWidget->setCurrentIndex(3);
    QCOMPARE(mainWindow.linkableViewsCount(), 1);
    // Switching to an empty tab -> focus goes to location bar (#84867)
    QEXPECT_FAIL("", "Known bug with unknown causes. Worked around it with commit 78f47b4d4ce97f7ff0278b16c04182e7d80c8dba", Continue);
    QCOMPARE(mainWindow.focusWidget()->metaObject()->className(), "KonqCombo");

    // Check that removeOtherTabs deals with split views correctly
    mainWindow.viewManager()->removeOtherTabs(2);
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)]."));   // mainWindow, tab widget, first tab = splitter with two frames
}

void ViewMgrTest::testCloseTabsFast() // #210551/#150162
{
    KonqMainWindow mainWindow;
    mainWindow.openUrl(nullptr, QUrl(QStringLiteral("data:text/html, <p>Hello World</p>")), QStringLiteral("text/html"));
    KonqViewManager *viewManager = mainWindow.viewManager();
    viewManager->addTab(QStringLiteral("text/html"));
    viewManager->addTab(QStringLiteral("text/html"));
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[FFF]."));   // mainWindow, tab widget, 3 simple tabs
    QTabWidget *tabWidget = mainWindow.findChild<QTabWidget *>();
    tabWidget->setCurrentIndex(2);

    mainWindow.setWorkingTab(1);
    mainWindow.slotRemoveTabPopup();
    mainWindow.slotRemoveTabPopup();
    QTest::qWait(100); // process the delayed invocations
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[F]."));   // mainWindow, tab widget, 1 tab left
}

void ViewMgrTest::testBrowserArgumentsNewTab()
{
    KonqMainWindow mainWindow;
    mainWindow.openUrl(nullptr, QUrl(QStringLiteral("data:text/html, <p>Hello World</p>")), QStringLiteral("text/html"));
    KParts::OpenUrlArguments urlArgs;
    KParts::BrowserArguments browserArgs;
    browserArgs.setNewTab(true);
    KonqView *view = mainWindow.currentView();
    KParts::BrowserExtension *ext = view->browserExtension();
    QVERIFY(ext);
    emit ext->openUrlRequest(QUrl(QStringLiteral("data:text/html, <p>Second tab test</p>")), urlArgs, browserArgs);
    QTest::qWait(5000);
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[FF].")); // mainWindow, tab widget, two tabs
    QCOMPARE(view->url(), QUrl("data:text/html, <p>Hello World</p>"));

    // compare the url of the new view... how to?
//    QCOMPARE(view->url(), QUrl("data:text/html, <p>Second tab test</p>"));
}

void ViewMgrTest::testBreakOffTab()
{
    KonqMainWindow mainWindow;
    KonqViewManager *viewManager = mainWindow.viewManager();
    /*KonqView* firstView =*/ viewManager->createFirstView(QStringLiteral("text/html"), QStringLiteral("webenginepart"));

    //KonqFrameBase* tab = firstView->frame();
    viewManager->duplicateTab(0);
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[FF]."));   // mainWindow, tab widget, two tabs

    // Break off a tab

    KonqMainWindow *mainWindow2 = viewManager->breakOffTab(0, mainWindow.size());
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[F]."));   // mainWindow, one tab, one frame
    QCOMPARE(DebugFrameVisitor::inspect(mainWindow2), QString("MT[F]."));   // mainWindow, one tab, one frame

    // Verify that the new tab container has an active child and that duplicating the tab in the new window does not crash (bug 203069)

    QVERIFY(mainWindow2->viewManager()->tabContainer()->activeChild());
    mainWindow2->viewManager()->duplicateTab(0);
    QCOMPARE(DebugFrameVisitor::inspect(mainWindow2), QString("MT[FF]."));   // mainWindow, tab widget, two tabs

    delete mainWindow2;

    // Now split the remaining view, duplicate the tab and verify that breaking off a split tab does not crash (bug 174292).
    // Also check that the tab container of the new main window has an active child.

    KonqView *view = mainWindow.activeChildView();
    viewManager->splitView(view, Qt::Vertical);
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)]."));   // mainWindow, tab widget, one splitter, two frames
    // KonqFrameContainerBase* container = view->frame()->parentContainer();
    viewManager->duplicateTab(0);
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)C(FF)]."));   // mainWindow, tab widget, two tabs with split views
    mainWindow2 = viewManager->breakOffTab(0, mainWindow.size());
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[C(FF)]."));   // mainWindow, tab widget, one splitter, two frames
    QCOMPARE(DebugFrameVisitor::inspect(mainWindow2), QString("MT[C(FF)]."));   // mainWindow, tab widget, one splitter, two frames
    QVERIFY(mainWindow2->viewManager()->tabContainer()->activeChild());

    delete mainWindow2;
}

void ViewMgrTest::moveTabLeft()
{
    KonqMainWindow mainWindow;
    mainWindow.openUrl(nullptr, QUrl(QStringLiteral("data:text/html, <p>Hello World</p>")), QStringLiteral("text/html"));
    KonqViewManager *viewManager = mainWindow.viewManager();
    KonqView *view1 = viewManager->addTab(QStringLiteral("text/html"));
    view1->openUrl(QUrl(QStringLiteral("data:text/html, <p>view1</p>")), QStringLiteral("1"));
    KonqView *view2 = viewManager->addTab(QStringLiteral("text/html"));
    view2->openUrl(QUrl(QStringLiteral("data:text/html, <p>view2</p>")), QStringLiteral("2"));
    QCOMPARE(DebugFrameVisitor::inspect(&mainWindow), QString("MT[FFF]."));   // mainWindow, tab widget, 3 simple tabs
    QTabWidget *tabWidget = mainWindow.findChild<QTabWidget *>();
    tabWidget->setCurrentIndex(2);
    view2->part()->widget()->setFocus();
    //qDebug() << mainWindow.focusWidget() << view2->part()->widget()->focusWidget();
    QCOMPARE(mainWindow.focusWidget(), view2->part()->widget()->focusWidget());
    viewManager->moveTabBackward();
    // Now we should have the views (tabs) in the order 0, 2, 1
    QList<KonqView *> views = KonqViewCollector::collect(&mainWindow);
    QCOMPARE(views[1], view2);
    QCOMPARE(views[2], view1);
    QCOMPARE(tabWidget->currentIndex(), 1);
    QCOMPARE(mainWindow.currentView(), view2);
    qDebug() << mainWindow.focusWidget() << view2->part()->widget()->focusWidget();
    // the focus should stay with that view (#208821)
    QCOMPARE(mainWindow.focusWidget()->metaObject()->className(), view2->part()->widget()->focusWidget()->metaObject()->className());
    QCOMPARE(mainWindow.focusWidget(), view2->part()->widget()->focusWidget());
}

