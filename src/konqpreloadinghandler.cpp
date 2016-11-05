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

#include <QDBusConnection>
#include <QDBusConnectionInterface>
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

// keep in sync with kfmclient.cpp
static const char s_preloadDBusName[] = "org.kde.konqueror.preloaded";

bool KonqPreloadingHandler::registerAsPreloaded()
{
    auto connection = QDBusConnection::sessionBus();
    if (!connection.registerService(QString::fromLatin1(s_preloadDBusName))) {
        return false; // another process holds this name already
    }
    KonqSessionManager::self()->disableAutosave(); // don't save sessions
    makePreloadedWindow();
    qDebug() << "Konqy preloaded:" << QDBusConnection::sessionBus().baseService();
    return true;
}

void KonqPreloadingHandler::startNextPreloadedProcess()
{
    // Let's make another process ready for the next window
    if (!KonqSettings::alwaysHavePreloaded()) {
        return;
    }

    // not running in full KDE environment?
    if (qEnvironmentVariableIsEmpty("KDE_FULL_SESSION")) {
        return;
    }
    // not the same user like the one running the session (most likely we're run via sudo or something)
    bool uidSet = false;
    const int uidEnvValue = qEnvironmentVariableIntValue("KDE_SESSION_UID", &uidSet);
    if (uidSet && uid_t(uidEnvValue) != getuid()) {
        return;
    }

    qDebug() << "Preloading next Konqueror instance";
    const QStringList args = { QStringLiteral("--preload") };
    QProcess::startDetached(QStringLiteral("konqueror"), args);
}

bool KonqPreloadingHandler::hasPreloadedWindow() const
{
    return m_preloadedWindow;
}

void KonqPreloadingHandler::makePreloadedWindow()
{
    KonqMainWindow *win = new KonqMainWindow(QUrl(QStringLiteral("about:blank"))); // prepare an empty window, with the web renderer preloaded
    win->viewManager()->clear();
    m_preloadedWindow = win;
}

KonqMainWindow *KonqPreloadingHandler::takePreloadedWindow()
{
    if (!m_preloadedWindow)
        return nullptr;

    KonqMainWindow *win = m_preloadedWindow;
    m_preloadedWindow = nullptr;

    KonqSessionManager::self()->enableAutosave(); // enable session saving again
    auto connection = QDBusConnection::sessionBus();
    connection.unregisterService(QString::fromLatin1(s_preloadDBusName));

    startNextPreloadedProcess();

    return win;
}

void KonqPreloadingHandler::ensurePreloadedProcessExists()
{
    if (!KonqSettings::alwaysHavePreloaded()) {
        return;
    }

    auto connection = QDBusConnection::sessionBus();
    if (!connection.interface()->isServiceRegistered(QString::fromLatin1(s_preloadDBusName))) {
        startNextPreloadedProcess();
    }
}
