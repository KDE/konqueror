/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 2000, 2006 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "KonqMainWindowAdaptor.h"
#include "KonqViewAdaptor.h"
#include "konqview.h"

#include "konqdebug.h"
#include <kstartupinfo.h>

KonqMainWindowAdaptor::KonqMainWindowAdaptor(KonqMainWindow *mainWindow)
    : QDBusAbstractAdaptor(mainWindow), m_pMainWindow(mainWindow)
{
}

KonqMainWindowAdaptor::~KonqMainWindowAdaptor()
{
}

void KonqMainWindowAdaptor::openUrl(const QString &url, bool tempFile)
{
    m_pMainWindow->openFilteredUrl(url, false, tempFile);
}

void KonqMainWindowAdaptor::newTab(const QString &url, bool tempFile)
{
    m_pMainWindow->openFilteredUrl(url, true, tempFile);
}

void KonqMainWindowAdaptor::newTabASN(const QString &url, const QByteArray &startup_id, bool tempFile)
{
    m_pMainWindow->setAttribute(Qt::WA_NativeWindow, true);
    KStartupInfo::setNewStartupId(m_pMainWindow->windowHandle(), startup_id);
    m_pMainWindow->openFilteredUrl(url, true, tempFile);
}

void KonqMainWindowAdaptor::newTabASNWithMimeType(const QString &url, const QString &mimetype, const QByteArray &startup_id, bool tempFile)
{
    m_pMainWindow->setAttribute(Qt::WA_NativeWindow, true);
    KStartupInfo::setNewStartupId(m_pMainWindow->windowHandle(), startup_id);
    m_pMainWindow->openFilteredUrl(url, mimetype, true, tempFile);
}

void KonqMainWindowAdaptor::reload()
{
    m_pMainWindow->slotReload();
}

QDBusObjectPath KonqMainWindowAdaptor::currentView()
{
    qCDebug(KONQUEROR_LOG);
    KonqView *view = m_pMainWindow->currentView();
    if (!view) {
        return QDBusObjectPath();
    }

    return QDBusObjectPath(view->dbusObjectPath());
}

QDBusObjectPath KonqMainWindowAdaptor::currentPart()
{
    KonqView *view = m_pMainWindow->currentView();
    if (!view) {
        return QDBusObjectPath();
    }

    return QDBusObjectPath(view->partObjectPath());
}

QDBusObjectPath KonqMainWindowAdaptor::view(int viewNumber)
{
    KonqMainWindow::MapViews viewMap = m_pMainWindow->viewMap();
    KonqMainWindow::MapViews::const_iterator it = viewMap.constBegin();
    for (int i = 0; it != viewMap.constEnd() && i < viewNumber; ++i) {
        ++it;
    }
    if (it == viewMap.constEnd()) {
        return QDBusObjectPath();
    }
    return QDBusObjectPath((*it)->dbusObjectPath());
}

QDBusObjectPath KonqMainWindowAdaptor::part(int partNumber)
{
    KonqMainWindow::MapViews viewMap = m_pMainWindow->viewMap();
    KonqMainWindow::MapViews::const_iterator it = viewMap.constBegin();
    for (int i = 0; it != viewMap.constEnd() && i < partNumber; ++i) {
        ++it;
    }
    if (it == viewMap.constEnd()) {
        return QDBusObjectPath();
    }
    return QDBusObjectPath((*it)->partObjectPath());
}

void KonqMainWindowAdaptor::splitViewHorizontally()
{
    m_pMainWindow->slotSplitViewHorizontal();
}

void KonqMainWindowAdaptor::splitViewVertically()
{
    m_pMainWindow->slotSplitViewVertical();
}

