/* This file is part of the KDE project
   Copyright (C) 1998, 1999, 2016 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konqmainwindowfactory.h"
#include "konqmainwindow.h"
#include "konqview.h"
#include "konqsettingsxt.h"
#include "konqdebug.h"
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
        QTimer::singleShot(500, nullptr, []() { new KonqMainWindow(QUrl(QStringLiteral("about:blank"))); });
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
