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
#include "konqsettingsxt.h"
#include "konqsettings.h"
#include "konqapplication.h"

#include "konqdebug.h"
#include <KStartupInfo>
#include <KWindowInfo>
#include <kwindowsystem.h>

#include <QFile>

#if QT_VERSION_MAJOR < 6
#include <QX11Info>
#else
#include <QtGui/private/qtx11extras_p.h>
#endif

#ifdef KActivities_FOUND
#if QT_VERSION_MAJOR < 6
#include <KActivities/Consumer>
#else //QT_VERSION_MAJOR
#include <PlasmaActivities/Consumer>
#endif //QT_VERSION_MAJOR
#endif //KActivities_FOUND

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
    QList<KonqMainWindow *> *mainWindows = KonqMainWindow::mainWindowList();
    if (!mainWindows) {
        return QDBusObjectPath("/");
    }
    //Accept only windows which are on the current desktop and in the current activity
    //(if activities are enabled)
    auto filter = [](KonqMainWindow *mw) {
        KWindowInfo winfo(mw->winId(), NET::WMDesktop, NET::WM2Activities);
#ifdef KActivities_FOUND
        QString currentActivity = KonquerorApplication::currentActivity();
        //if currentActivity is empty, it means that the activity service status is not running
        if (winfo.isOnCurrentDesktop() && (currentActivity.isEmpty() || winfo.activities().contains(currentActivity))) {
#else
        if (winfo.isOnCurrentDesktop()) {
#endif
            Q_ASSERT(!mw->dbusName().isEmpty());
            return true;
        } else {
            return false;
        }
    };
    QList<KonqMainWindow*> visibleWindows;
    std::copy_if(mainWindows->constBegin(), mainWindows->constEnd(), std::back_inserter(visibleWindows), filter);

    //Sort the windows according to the last deactivation order, so that windows deactivated last come first
    //(if a window is active, it'll come before the others)
    auto sorter = [](KonqMainWindow *w1, KonqMainWindow *w2) {
        if (w1->isActiveWindow()) {
            return true;
        } else if (w2->isActiveWindow()) {
            return false;
        } else {
            return w2->lastDeactivationTime() < w1->lastDeactivationTime();
        }
    };
    std::sort(visibleWindows.begin(), visibleWindows.end(), sorter);
    if (!visibleWindows.isEmpty()) {
        return QDBusObjectPath(visibleWindows.first()->dbusName());
    } else {
        return QDBusObjectPath("/");
    }
}
