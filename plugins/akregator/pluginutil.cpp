/*
    This file is part of Akregator.

    Copyright (C) 2004 Teemu Rytilahti <tpr@d5k.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include "pluginutil.h"

#include <kprocess.h>
#include <KLocalizedString>
#include <KMessageBox>

#include "feeddetector.h"
#include "akregatorplugindebug.h"

#include <qstringlist.h>
#include <qurl.h>
#include <qdbusconnection.h>
#include <qdbusconnectioninterface.h>
#include <qdbusinterface.h>
#include <qdbusreply.h>

using namespace Akregator;

static bool isAkregatorRunning()
{
    //Laurent if akregator is registered into dbus so akregator is running
    return QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.kde.akregator"));
}

static void addFeedsViaDBUS(const QStringList &urls)
{
    QDBusInterface akregator(QStringLiteral("org.kde.akregator"), QStringLiteral("/Akregator"), QStringLiteral("org.kde.akregator.part"));
    QDBusReply<void> reply  = akregator.call(QStringLiteral("addFeedsToGroup"), urls, i18n("Imported Feeds"));
    if (!reply.isValid()) {
        KMessageBox::error(nullptr, i18n("Unable to contact Akregator via DBus"),
                                    i18nc("@title:window", "DBus call failed"));
    }
}

static void addFeedsViaCmdLine(const QStringList &urls)
{
    KProcess proc;
    proc << QStringLiteral("akregator") << QStringLiteral("-g") << i18n("Imported Feeds");
    foreach (const QString &url, urls) {
        proc << QStringLiteral("-a") << url;
    }
    proc.startDetached();
}

void PluginUtil::addFeeds(const QStringList &urls)
{
    if (isAkregatorRunning()) {
        qCDebug(AKREGATORPLUGIN_LOG) << "adding" << urls.count() << "feeds via DBus";
        addFeedsViaDBUS(urls);
    } else {
        qCDebug(AKREGATORPLUGIN_LOG) << "adding" << urls.count() << "feeds via command line";
        addFeedsViaCmdLine(urls);
    }
}

// handle all the wild stuff that QUrl doesn't handle
QString PluginUtil::fixRelativeURL(const QString &s, const QUrl &baseurl)
{
    QString s2 = s;
    QUrl u;
    if (QUrl(s2).isRelative()) {
        if (s2.startsWith(QLatin1String("//"))) {
            s2.prepend(baseurl.scheme() + ':');
            u.setUrl(s2);
        } else if (s2.startsWith(QLatin1String("/"))) {
            // delete path/query/fragment, so that only protocol://host remains
            QUrl b2(baseurl.adjusted(QUrl::RemovePath|QUrl::RemoveQuery|QUrl::RemoveFragment));
            u = b2.resolved(QUrl(s2.mid(1)));		// remove leading "/"
        } else {
            u = baseurl.resolved(QUrl(s2));
        }
    } else {
        u.setUrl(s2);
    }

    u = u.adjusted(QUrl::NormalizePathSegments);
    //qCDebug(AKREGATORPLUGIN_LOG) << "url=" << s << " baseurl=" << baseurl << "fixed=" << u;
    return u.url();
}
