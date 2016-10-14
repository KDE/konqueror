/* This file is part of the KDE project
   Copyright (C) 2000-2016 David Faure <faure@kde.org>

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

#include "konqpreloadinghandler.h"
#include "konqmainwindow.h"
#include "konqsessionmanager.h"
#include "konqviewmanager.h"
#include "konqsettingsxt.h"

#include <QDBusInterface>
#include <QDBusConnection>
#include <QProcess>

static KonqPreloadingHandler *s_self = nullptr;

KonqPreloadingHandler::KonqPreloadingHandler()
{
    s_self = this;
}

KonqPreloadingHandler *KonqPreloadingHandler::self()
{
    return s_self;
}

void KonqPreloadingHandler::setPreloadedFlag(bool preloaded)
{
    if (m_preloaded == preloaded) {
        return;
    }
    m_preloaded = preloaded;
    if (m_preloaded) {
        KonqSessionManager::self()->disableAutosave(); // don't save sessions
        return; // was registered before calling this
    }
    delete m_preloadedWindow; // preloaded state was abandoned without reusing the window
    m_preloadedWindow = nullptr;
    KonqSessionManager::self()->enableAutosave(); // enable session saving again
    QDBusInterface ref("org.kde.kded5", "/modules/konqy_preloader", "org.kde.konqueror.Preloader", QDBusConnection::sessionBus());
    ref.call("unregisterPreloadedKonqy", QDBusConnection::sessionBus().baseService());
}

bool KonqPreloadingHandler::isPreloaded() const
{
    return m_preloaded;
}

void KonqPreloadingHandler::makePreloadedWindow()
{
    KonqMainWindow *win = new KonqMainWindow(QUrl("about:blank")); // prepare an empty window, with the web renderer preloaded
    // KonqMainWindow ctor sets always the preloaded flag to false, so create the window before this
    setPreloadedFlag(true);
    win->viewManager()->clear();
    m_preloadedWindow = win;
}

KonqMainWindow *KonqPreloadingHandler::takePreloadedWindow()
{
    if (!m_preloaded)
        return nullptr;

    KonqMainWindow *win = m_preloadedWindow;
    m_preloadedWindow = nullptr;
    setPreloadedFlag(false);

    // Let's make another process ready for the next window
    if (KonqSettings::alwaysHavePreloaded()) {
        qDebug() << "Preloading next Konqueror instance";
        const QStringList args = { QStringLiteral("--preload") };
        QProcess::startDetached(QStringLiteral("konqueror"), args);
    }

    return win;
}
