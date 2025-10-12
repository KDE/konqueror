/* This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 2000, 2006 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "KonquerorAdaptor.h"
#include "konqmisc.h"
#include "KonqMainWindowAdaptor.h"
#include "konqmainwindow.h"
#include "konqmainwindowfactory.h"
#include "konqviewmanager.h"
#include "konqview.h"
#include "konqsettings.h"
#include "konqembedsettings.h"
#include "konqapplication.h"

#include "konqdebug.h"
#include <KStartupInfo>
#include <KWindowInfo>
#include <kwindowsystem.h>

#include <QFile>

#include <QtGui/private/qtx11extras_p.h>

#ifdef KActivities_FOUND
#include <PlasmaActivities/Consumer>
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
    QX11Info::setAppUserTime(0);
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
        for (KonqMainWindow *window: *mainWindows) {
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
    KonqMainWindow *window = KonqMainWindow::findMostSuitableWindow();
    if (window) {
        Q_ASSERT(!window->dbusName().isEmpty());
    }
    return QDBusObjectPath(window ? window->dbusName() : "/");
}
