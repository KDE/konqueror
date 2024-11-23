/* This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999, 2016 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqmainwindowfactory.h"
#include "konqmainwindow.h"
#include "konqview.h"
#include "konqsettings.h"
#include "konqdebug.h"
#include "konqurl.h"
#include "konqmisc.h"
#include "konqapplication.h"

#include <KWindowInfo>
#include <KStartupInfo>
#include <QTimer>

#include <QtGui/private/qtx11extras_p.h>

#include <KWindowSystem>

// Terminates fullscreen-mode for any full-screen window on the current desktop
static void abortFullScreenMode()
{
    QList<KonqMainWindow *> *mainWindows = KonqMainWindow::mainWindowList();
    if (mainWindows) {
        for (KonqMainWindow *window: *mainWindows) {
            if (window->fullScreenMode()) {
                KWindowInfo info(window->winId(), NET::WMDesktop);
                if (info.valid() && info.isOnCurrentDesktop()) {
                    window->setWindowState(window->windowState() & ~Qt::WindowFullScreen);
                }
            }
        }
    }
}

// Prepare another preloaded window for next time
static void ensurePreloadedWindow()
{
    if (Konq::Settings::alwaysHavePreloaded()) {
        QTimer::singleShot(500, nullptr, []() {return KonqMainWindowFactory::createPreloadWindow();});
    }
}

KonqMainWindow* KonqMainWindowFactory::findPreloadedWindow()
{
    QList<KonqMainWindow *> *mainWindowList = KonqMainWindow::mainWindowList();
    if (!mainWindowList) {
        return nullptr;
    }
    auto it = std::find_if(mainWindowList->constBegin(), mainWindowList->constEnd(), [](KonqMainWindow* w){return w->isPreloaded();});
    return it != mainWindowList->constEnd() ? *it : nullptr;
}

KonqMainWindow *KonqMainWindowFactory::createEmptyWindow()
{
    abortFullScreenMode();

    // Let's see if we can reuse a preloaded window
    KonqMainWindow *win = findPreloadedWindow();
    if (win) {
        qCDebug(KONQUEROR_LOG) << "Reusing preloaded window" << win;
        QByteArray startupId = KWindowSystem::isPlatformX11() ? QX11Info::nextStartupId() : qEnvironmentVariable("XDG_ACTIVATION_TOKEN").toUtf8();
        KStartupInfo::setNewStartupId(win->windowHandle(), startupId);
        win->setOnActivities({KonquerorApplication::currentActivity()});
    } else {
        win = new KonqMainWindow(KonqUrl::url(KonqUrl::Type::Blank));
    }
    ensurePreloadedWindow();
    return win;
}

KonqMainWindow * KonqMainWindowFactory::createPreloadWindow()
{
    KonqMainWindow *mw = new KonqMainWindow(KonqUrl::url(KonqUrl::Type::Blank));
    return mw;
}

KonqMainWindow *KonqMainWindowFactory::createNewWindow(const QUrl &url,
                                          const KonqOpenURLRequest &req)
{
    KonqMainWindow *mainWindow = KonqMainWindowFactory::createEmptyWindow();
    if (!url.isEmpty()) {
        mainWindow->openUrl(nullptr, url, QString(), req);
        mainWindow->setInitialFrameName(req.browserArgs.frameName);
    } else {
        mainWindow->openUrl(nullptr, KonqMisc::konqFilteredURL(mainWindow, Konq::Settings::startURL()));
        mainWindow->focusLocationBar();
    }
    return mainWindow;
}
