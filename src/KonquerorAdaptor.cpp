/* This file is part of the KDE project
   Copyright 1998, 1999 Simon Hausmann <hausmann@kde.org>
   Copyright 2000, 2006 David Faure <faure@kde.org>

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

#include "KonquerorAdaptor.h"
#include "konqmisc.h"
#include "KonqMainWindowAdaptor.h"
#include "konqmainwindow.h"
#include "konqmainwindowfactory.h"
#include "konqviewmanager.h"
#include "konqview.h"
#include "konqsettingsxt.h"
#include "konqsettings.h"

#include "konqdebug.h"
#include <kwindowsystem.h>
#include <KStartupInfo>

#include <QFile>
#if KONQ_HAVE_X11
#include <QX11Info>
#endif

// these DBus calls come from outside, so any windows created by these
// calls would have old user timestamps (for KWin's no-focus-stealing),
// it's better to reset the timestamp and rely on other means
// of detecting the time when the user action that triggered all this
// happened
// TODO a valid timestamp should be passed in the DBus calls that
// are not for user scripting

KonquerorAdaptor::KonquerorAdaptor()
    : QObject(qApp)
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(KONQ_MAIN_PATH, this, QDBusConnection::ExportNonScriptableSlots);
}

KonquerorAdaptor::~KonquerorAdaptor()
{
}

static void setStartupId(const QByteArray &startup_id)
{
    KStartupInfo::setStartupId(startup_id);
#if KONQ_HAVE_X11
    QX11Info::setAppUserTime(0);
#endif
}

QDBusObjectPath KonquerorAdaptor::openBrowserWindow(const QString &url, const QByteArray &startup_id)
{
    setStartupId(startup_id);
    KonqMainWindow *res = KonqMainWindowFactory::createNewWindow(QUrl::fromUserInput(url));
    if (!res) {
        return QDBusObjectPath("/");
    }
    return QDBusObjectPath(res->dbusName());
}

QDBusObjectPath KonquerorAdaptor::createNewWindow(const QString &url, const QString &mimetype, const QByteArray &startup_id, bool tempFile)
{
    setStartupId(startup_id);
    KParts::OpenUrlArguments args;
    args.setMimeType(mimetype);
    // Filter the URL, so that "kfmclient openURL gg:foo" works also when konq is already running
    QUrl finalURL = KonqMisc::konqFilteredURL(nullptr, url);
    KonqOpenURLRequest req;
    req.args = args;
    req.tempFile = tempFile;
    KonqMainWindow *res = KonqMainWindowFactory::createNewWindow(finalURL, req);
    if (!res) {
        return QDBusObjectPath("/");
    }
    res->show();
    return QDBusObjectPath(res->dbusName());
}

QDBusObjectPath KonquerorAdaptor::createNewWindowWithSelection(const QString &url, const QStringList &filesToSelect, const QByteArray &startup_id)
{
    setStartupId(startup_id);
    KonqOpenURLRequest req;
    req.filesToSelect = QUrl::fromStringList(filesToSelect);
    KonqMainWindow *res = KonqMainWindowFactory::createNewWindow(QUrl::fromUserInput(url), req);
    if (!res) {
        return QDBusObjectPath("/");
    }
    res->show();
    return QDBusObjectPath(res->dbusName());
}

QList<QDBusObjectPath> KonquerorAdaptor::getWindows()
{
    QList<QDBusObjectPath> lst;
    QList<KonqMainWindow *> *mainWindows = KonqMainWindow::mainWindowList();
    if (mainWindows) {
        foreach (KonqMainWindow *window, *mainWindows) {
            lst.append(QDBusObjectPath(window->dbusName()));
        }
    }
    return lst;
}

QStringList KonquerorAdaptor::urls() const
{
    QStringList lst;
    QList<KonqMainWindow *> *mainWindows = KonqMainWindow::mainWindowList();
    if (mainWindows) {
        for (KonqMainWindow *window : *mainWindows) {
            if (!window->isPreloaded()) {
                for (KonqView *view : window->viewMap()) {
                    lst.append(view->url().toString());
                }
            }
        }
    }
    return lst;
}

QDBusObjectPath KonquerorAdaptor::windowForTab()
{
    QList<KonqMainWindow *> *mainWindows = KonqMainWindow::mainWindowList();
    if (mainWindows) {
        foreach (KonqMainWindow *window, *mainWindows) {
            KWindowInfo winfo(window->winId(), NET::WMDesktop);
            if (winfo.isOnCurrentDesktop()) {  // we want a tab in an already shown window
                Q_ASSERT(!window->dbusName().isEmpty());
                return QDBusObjectPath(window->dbusName());
            }
        }
    }
    // We can't use QDBusObjectPath(), dbus type 'o' must be a valid object path.
    // So we use "/" as an indicator for not found.
    return QDBusObjectPath("/");
}
