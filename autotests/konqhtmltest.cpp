/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2008 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <konqmainwindowfactory.h>
#include "../src/konqsettingsxt.h"
#include <KLocalizedString>

#include <konqmainwindow.h>
#include <konqviewmanager.h>
#include <konqview.h>
#include <konqsessionmanager.h>

#include <webenginepart.h>
#include <webengineview.h>
#include <webenginepage.h>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

#include <KSharedConfig>
#include <ktoolbar.h>
#include <ksycoca.h>

#include <QTemporaryFile>
#include <QScrollArea>
#include <QProcess>
#include <QTest>
#include <QSignalSpy>
#include <QObject>
#include <QStandardPaths>

class KonqHtmlTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        KLocalizedString::setApplicationDomain("konqhtmltest");
        QStandardPaths::setTestModeEnabled(true);

        KonqSessionManager::self()->disableAutosave();
        //qRegisterMetaType<KonqView *>("KonqView*");

        // Ensure the tests use webenginepart, not KHTML or kwebkitpart
        // This code is inspired by settings/konqhtml/generalopts.cpp
        bool needsUpdate = false;
        KSharedConfig::Ptr profile = KSharedConfig::openConfig(QStringLiteral("mimeapps.list"), KConfig::NoGlobals, QStandardPaths::ApplicationsLocation);
        KConfigGroup addedServices(profile, "Added KDE Service Associations");
        QStringList mimetypes = {"text/html", "application/xhtml+xml", "application/xml"};
        for (const QString &mimeType: mimetypes) {
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
    void cleanupTestCase()
    {
        // in case some test broke, don't assert in khtmlglobal...
        deleteAllMainWindows();
    }
    void loadSimpleHtml()
    {
        KonqMainWindow mainWindow;
        // we specify the mimetype so that we don't have to wait for a KonqRun
        mainWindow.openUrl(nullptr, QUrl(QStringLiteral("data:text/html, <p>Hello World</p>")), QStringLiteral("text/html"));
        KonqView *view = mainWindow.currentView();
        QVERIFY(view);
        QVERIFY(view->part());
        QSignalSpy spyCompleted(view, &KonqView::viewCompleted);
        QVERIFY(spyCompleted.wait(20000));
        QCOMPARE(view->serviceType(), QString("text/html"));
        WebEnginePart* part = qobject_cast<WebEnginePart *>(view->part());
        QVERIFY(part);
    }

    void loadDirectory() // #164495, konqueror gets in a loop when setting a directory as homepage
    {
        KonqMainWindow mainWindow;
        const QUrl url = QUrl::fromLocalFile(QDir::homePath());
        mainWindow.openUrl(nullptr, url, QStringLiteral("text/html"));
        KonqView *view = mainWindow.currentView();
        qDebug() << "Waiting for first completed signal";
        QSignalSpy spyCompleted(view, &KonqView::viewCompleted);
        QVERIFY(spyCompleted.wait(20000));        // error calls openUrlRequest
        if (view->aborted()) {
            qDebug() << "Waiting for second completed signal";
            QVERIFY(spyCompleted.wait(20000));        // which then opens the right part
            QCOMPARE(view->serviceType(), QString("inode/directory"));
        } else {
            // WebEngine can actually list directories, no error.
            // To test this: konqueror --mimetype text/html $HOME
            QCOMPARE(view->url().adjusted(QUrl::StripTrailingSlash), url);
        }
    }

    void rightClickClose() // #149736
    {
        QPointer<KonqMainWindow> mainWindow = new KonqMainWindow;
        // we specify the mimetype so that we don't have to wait for a KonqRun
        mainWindow->openUrl(nullptr, QUrl(
                                "data:text/html, <script type=\"text/javascript\">"
                                "function closeMe() { window.close(); } "
                                "document.onmousedown = closeMe; "
                                "</script>"), QStringLiteral("text/html"));
        QPointer<KonqView> view = mainWindow->currentView();
        QVERIFY(view);
        QSignalSpy spyCompleted(view, SIGNAL(viewCompleted(KonqView*)));
        QVERIFY(spyCompleted.wait(20000));
        QWidget *widget = partWidget(view);
        qDebug() << "Clicking on" << widget;
        QTest::mousePress(widget, Qt::RightButton);
        QTRY_VERIFY(!view); // deleted
        QTRY_VERIFY(!mainWindow); // the whole window gets deleted, in fact
    }

    void windowOpen()
    {
        // Simple test for window.open in a onmousedown handler.

        // Want a window, not a tab (historical test)
        KonqSettings::setMmbOpensTab(false);
        KonqSettings::setAlwaysHavePreloaded(false);

        // We have to use the same protocol for both the orig and dest urls.
        // KAuthorized would forbid a data: URL to redirect to a file: URL for instance.
        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        tempFile.write("<title>Popup</title><script>document.title=\"Opener=\" + window.opener;</script>");

        QTemporaryFile origTempFile;
        QVERIFY(origTempFile.open());
        origTempFile.write(
            "<html><script>"
            "function openWindow() { window.open('" + QUrl::fromLocalFile(tempFile.fileName()).url().toUtf8() + "'); } "
            "document.onmousedown = openWindow; "
            "</script></html>"
        );
        tempFile.close();
        const QString origFile = origTempFile.fileName();
        origTempFile.close();

        KonqMainWindow *mainWindow = KonqMainWindowFactory::createNewWindow(QUrl::fromLocalFile(origFile));
        QCOMPARE(KMainWindow::memberList().count(), 1);
        KonqView *view = mainWindow->currentView();
        QVERIFY(view);
        QSignalSpy spyCompleted(view, SIGNAL(viewCompleted(KonqView*)));
        QVERIFY(spyCompleted.wait(20000));
        qApp->processEvents();
        WebEnginePart *htmlPart = qobject_cast<WebEnginePart *>(view->part());
        htmlPart->view()->page()->profile()->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
        htmlPart->view()->page()->profile()->settings()->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, true);
        htmlPart->view()->page()->profile()->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls	, true);
        htmlPart->view()->page()->profile()->settings()->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
        htmlPart->view()->page()->profile()->settings()->setAttribute(QWebEngineSettings::AllowWindowActivationFromJavaScript, true);
        QWidget *widget = partWidget(view);
        QVERIFY(widget);
        qDebug() << "Clicking on the khtmlview";
        QTest::mousePress(widget, Qt::LeftButton);
        qApp->processEvents(); // openurlrequestdelayed
        qApp->processEvents(); // browserrun
        hideAllMainWindows(); // TODO: why does it appear nonetheless? hiding too early? hiding too late
        // Did it open a window?
        QTRY_COMPARE(KMainWindow::memberList().count(), 2);
        KonqMainWindow *newWindow = qobject_cast<KonqMainWindow *>(KMainWindow::memberList().last());
        QVERIFY(newWindow);
        QVERIFY(newWindow != mainWindow);
        compareToolbarSettings(mainWindow, newWindow);
        // Does the window contain exactly one view?
        QCOMPARE(newWindow->viewCount(), 1);
        KonqFrame *frame = newWindow->currentView()->frame();
        QVERIFY(frame);
        QVERIFY(!frame->childView()->isLoading());
        WebEnginePart *part = qobject_cast<WebEnginePart *>(frame->part());
        QVERIFY(part);
        QTRY_VERIFY(!part->view()->url().isEmpty() && part->view()->url().scheme() != QStringLiteral("konq")); // hack to wait for webengine to load the page
        QTRY_COMPARE(part->view()->title(), QString("Opener=[object Window]"));
        deleteAllMainWindows();
    }

    void testJSError()
    {
        // JS errors appear in a statusbar label, and deleting the frame first
        // would lead to double deletion (#228255)
        QPointer<KonqMainWindow> mainWindow = new KonqMainWindow;
        // we specify the mimetype so that we don't have to wait for a KonqRun
        mainWindow->openUrl(nullptr, QUrl(QStringLiteral("data:text/html, <script>window.foo=bar</script><p>Hello World</p>")), QStringLiteral("text/html"));
        KonqView *view = mainWindow->currentView();
        QVERIFY(view);
        QVERIFY(view->part());
        QSignalSpy spyCompleted(view, &KonqView::viewCompleted);
        QVERIFY(spyCompleted.wait(20000));
        QCOMPARE(view->serviceType(), QString("text/html"));
        delete view->part();
        QTRY_VERIFY(!mainWindow); // the window gets deleted
    }

private:
    // Return the main widget for the given KonqView; used for clicking onto it
    static QWidget *partWidget(KonqView *view)
    {
        QWidget *widget = view->part()->widget();
        WebEnginePart *htmlPart = qobject_cast<WebEnginePart *>(view->part());
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

    // Delete all KonqMainWindows
    static void deleteAllMainWindows()
    {
        const QList<KMainWindow *> windows = KMainWindow::memberList();
        qDeleteAll(windows);
    }

    void compareToolbarSettings(KMainWindow *mainWindow, KMainWindow *newWindow)
    {
        QVERIFY(mainWindow != newWindow);
        KToolBar *firstToolBar = mainWindow->toolBars().first();
        QVERIFY(firstToolBar);
        KToolBar *newFirstToolBar = newWindow->toolBars().first();
        QVERIFY(newFirstToolBar);
        QCOMPARE(firstToolBar->toolButtonStyle(), newFirstToolBar->toolButtonStyle());
    }

    static void hideAllMainWindows()
    {
        const QList<KMainWindow *> windows = KMainWindow::memberList();
        qDebug() << "hiding" << windows.count() << "windows";
        for (KMainWindow *window: windows) {
            window->hide();
        }
    }
};

QTEST_MAIN(KonqHtmlTest)

#include "konqhtmltest.moc"
