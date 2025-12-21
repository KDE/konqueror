/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2008 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <konqmainwindowfactory.h>
#include "konqsettings.h"
#include <KLocalizedString>

#include <konqmainwindow.h>
#include <konqviewmanager.h>
#include <konqview.h>
#include <konqsessionmanager.h>

#include <webenginepart.h>
#include <webengineview.h>
#include <webenginepage.h>
#include <webenginepartcontrols.h>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QMenu>

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

using namespace Konq;

class KonqHtmlTest : public QObject
{
    static constexpr const char* s_htmlTemplate{"/XXXXXX.html"};

    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        KLocalizedString::setApplicationDomain("konqhtmltest");
        QStandardPaths::setTestModeEnabled(true);

        KonqSessionManager::self()->disableAutosave();
        WebEnginePartControls::self()->disablePageLifecycleStateManagement();

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

    void cleanup() {
        deleteAllMainWindows();
    }

    bool copyToTemporaryFile(const QString &fileName, QTemporaryFile &tmpFile, std::function<QString(QString)> fnc={}) {
        QFile file(fileName);
        if (!file.open(QFile::ReadOnly)) {
            return false;
        }
        QString contents = file.readAll();
        tmpFile.write(fnc ? fnc(contents).toLatin1() : contents.toLatin1());
        return true;
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
        QCOMPARE(view->type().mimetype().value(), QString("text/html"));
        WebEnginePart* part = qobject_cast<WebEnginePart *>(view->part());
        QVERIFY(part);
    }

    void loadDirectory() // #164495, konqueror gets in a loop when setting a directory as homepage
    {
        KonqMainWindow mainWindow;
        const QUrl url = QUrl::fromLocalFile(QDir::homePath());
        mainWindow.openUrl(nullptr, url, QStringLiteral("text/html"));
        KonqView *view = mainWindow.currentView();
        QSignalSpy spyCompleted(view, &KonqView::viewCompleted);
        QVERIFY(spyCompleted.wait(20000));        // error calls openUrlRequest
        if (view->aborted()) {
            qDebug() << "Waiting for second completed signal";
            QVERIFY(spyCompleted.wait(20000));        // which then opens the right part
            QCOMPARE(view->type().mimetype().value(), QString("inode/directory"));
        } else {
            // WebEngine can actually list directories, no error.
            // To test this: konqueror --mimetype text/html $HOME
            QCOMPARE(view->url().adjusted(QUrl::StripTrailingSlash), url);
        }
    }

    void adjustSettings(WebEnginePart *part) {
        Settings::self()->setMmbOpensTab(false);
        Konq::Settings::setAlwaysHavePreloaded(false);
        QWebEngineSettings *settings = part->profile()->settings();
        settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
        settings->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, true);
        settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
        settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
        settings->setAttribute(QWebEngineSettings::AllowWindowActivationFromJavaScript, true);
    }

    /*
     * Test that right-clicking on a page which automatically closes on a mouse click doesn't crash trying
     * to display the context menu (BUG #149736).
     *
     * This requires a complex setup because javascript window.close() is only allowed to close a window
     * opened from javascript, so we need two windows: one which automatically closes on mouse press and
     * one to open the other one. Another complication raises from the fact that, when the popup menu is shown
     * everything is blocked (apparently, including running javascript). To work around this, we use a QTimer
     * to send a Qt::Key_Escape key click to the active menu, so that it can be closed
     */
    void rightClickClose()
    {
        Settings::self()->setAlwaysEmbedInNewTab(false);

        QTemporaryFile popupFile(QDir::tempPath() + s_htmlTemplate);
        QVERIFY(popupFile.open());
        QVERIFY(copyToTemporaryFile(":popupmouseclose.html", popupFile));
        QUrl popupUrl = QUrl::fromLocalFile(popupFile.fileName());
        popupFile.close();

        QTemporaryFile openerFile(QDir::tempPath() + s_htmlTemplate);
        QVERIFY(openerFile.open());
        QVERIFY(copyToTemporaryFile(":windowopener.html", openerFile, [popupUrl](const QString &s){return s.arg(popupUrl.toString());}));

        QUrl url = QUrl::fromLocalFile(openerFile.fileName());
        openerFile.close();

        //Create the launcher window
        KonqMainWindow *mainWindow = KonqMainWindowFactory::createNewWindow(url);
        QCOMPARE(KMainWindow::memberList().count(), 1);
        QPointer<KonqView> view = mainWindow->currentView();
        QVERIFY(view);
        QSignalSpy spyCompleted(view, &KonqView::viewCompleted);
        QVERIFY(spyCompleted.wait(20000));
        WebEnginePart *htmlPart = qobject_cast<WebEnginePart *>(view->part());
        adjustSettings(htmlPart);
        QWidget *widget = partWidget(view);
        QVERIFY(widget);
        QTest::mousePress(widget, Qt::LeftButton);

        //Ensure the new window has been created
        QTRY_COMPARE(KMainWindow::memberList().count(), 2);
        hideAllMainWindows();
        QPointer<KonqMainWindow> newWindow = KonqMainWindow::mainWindows().last();
        QVERIFY(newWindow);
        QVERIFY(newWindow != mainWindow);

        //Get the view in the new window
        QCOMPARE(newWindow->viewCount(), 1);
        view = newWindow->currentView();
        KonqFrame *frame = newWindow->currentView()->frame();
        WebEnginePart *part = qobject_cast<WebEnginePart *>(frame->part());
        QVERIFY(part);
        QSignalSpy spyPopupLoaded(part, &WebEnginePart::completed);
        //Ensure the view has finished loading
        QVERIFY(spyPopupLoaded.wait(20000));
        widget = partWidget(newWindow->currentView());
        bool menuShown = false; //Check the menu is shown
        QTimer t(this);
        auto timeout = [newWindow, &t, &menuShown] {
            //Ensure there is at most one active menu
            QList<QMenu*> menus = newWindow->findChildren<QMenu*>();
            QList<QMenu*> activeMenus;
            std::copy_if(menus.constBegin(), menus.constEnd(), std::back_inserter(activeMenus), [](QMenu *m){return m->isActiveWindow();});
            QVERIFY(activeMenus.count() < 2);

            if (activeMenus.count() == 0) {
                return;
            }
            menuShown = true;
            QTest::keyClick(activeMenus.at(0), Qt::Key_Escape);
            t.stop();
        };
        connect(&t, &QTimer::timeout, newWindow, timeout);
        t.start(100);
        QTest::mousePress(widget, Qt::RightButton);
        QTRY_VERIFY(menuShown);
        QTRY_VERIFY(!view);
        QTRY_VERIFY(!newWindow);
    }

    void windowOpen()
    {
        // Simple test for window.open in a onmousedown handler.

        // We have to use the same protocol for both the orig and dest urls.
        // KAuthorized would forbid a data: URL to redirect to a file: URL for instance.
        QTemporaryFile popupTempFile(QDir::tempPath() + s_htmlTemplate);
        QVERIFY(popupTempFile.open());
        QVERIFY(copyToTemporaryFile(":popup.html", popupTempFile));

        QTemporaryFile openerTempFile(QDir::tempPath() + s_htmlTemplate);
        QString fileName = QUrl::fromLocalFile(popupTempFile.fileName()).url().toUtf8();
        QVERIFY(openerTempFile.open());
        QVERIFY(copyToTemporaryFile(":windowopener.html", openerTempFile, [fileName](const QString &s){return s.arg(fileName);}));
        popupTempFile.close();
        const QString openerFileName = openerTempFile.fileName();
        openerTempFile.close();

        KonqOpenURLRequest req;
        req.browserArgs.setEmbedWith("webenginepart");
        KonqMainWindow *mainWindow = KonqMainWindowFactory::createNewWindow(QUrl::fromLocalFile(openerFileName), req);
        QCOMPARE(KMainWindow::memberList().count(), 1);
        KonqView *view = mainWindow->currentView();
        QVERIFY(view);
        QSignalSpy spyCompleted(view, &KonqView::viewCompleted);
        QVERIFY(spyCompleted.wait(20000));
        qApp->processEvents();
        WebEnginePart *htmlPart = qobject_cast<WebEnginePart *>(view->part());
        adjustSettings(htmlPart);
        QWidget *widget = partWidget(view);
        QVERIFY(widget);
        qDebug() << "Clicking on the webengineview";
        QTest::mousePress(widget, Qt::LeftButton);
        // Did it open a window?
        QTRY_COMPARE(KMainWindow::memberList().count(), 2);
        hideAllMainWindows();
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
        QCOMPARE(view->type().mimetype().value(), QString("text/html"));
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
