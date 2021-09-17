/* This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999, 2016 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqmainwindowfactory.h"
#include "konqmainwindow.h"
#include "konqview.h"
#include "konqsettingsxt.h"
#include "konqdebug.h"
#include "konqurl.h"

#include <KWindowInfo>
#include <KStartupInfo>
#include <QTimer>

// Terminates fullscreen-mode for any full-screen window on the current desktop
static void abortFullScreenMode()
{
    QList<KonqMainWindow *> *mainWindows = KonqMainWindow::mainWindowList();
    if (mainWindows) {
        foreach (KonqMainWindow *window, *mainWindows) {
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
    if (KonqSettings::alwaysHavePreloaded()) {
        QTimer::singleShot(500, nullptr, []() { new KonqMainWindow(KonqUrl::url(KonqUrl::Type::Blank)); });
    }
}

KonqMainWindow *KonqMainWindowFactory::createEmptyWindow()
{
    abortFullScreenMode();

    // Let's see if we can reuse a preloaded window
    QList<KonqMainWindow *> *mainWindowList = KonqMainWindow::mainWindowList();
    if (mainWindowList) {
        for (KonqMainWindow *win : *mainWindowList) {
            if (win->isPreloaded()) {
                qCDebug(KONQUEROR_LOG) << "Reusing preloaded window" << win;
                KStartupInfo::setWindowStartupId(win->winId(), KStartupInfo::startupId());
                ensurePreloadedWindow();
                return win;
            }
        }
    }
    ensurePreloadedWindow();
    return new KonqMainWindow;
}

KonqMainWindow *KonqMainWindowFactory::createNewWindow(const QUrl &url,
                                          const KonqOpenURLRequest &req)
{
    KonqMainWindow *mainWindow = KonqMainWindowFactory::createEmptyWindow();
    if (!url.isEmpty()) {
        mainWindow->openUrl(nullptr, url, QString(), req);
        mainWindow->setInitialFrameName(req.browserArgs.frameName);
    } else {
        mainWindow->openUrl(nullptr, QUrl(KonqSettings::startURL()));
        mainWindow->focusLocationBar();
    }
    return mainWindow;
}
