/*
    This file is part of Akregator.

    SPDX-FileCopyrightText: 2004 Teemu Rytilahti <tpr@d5k.net>

    SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-exception
*/

#include "pluginutil.h"

#include <kprocess.h>
#include <KLocalizedString>
#include <KMessageBox>

#include "feeddetector.h"
#include "akregatorplugindebug.h"

#include <QStringList>
#include <QUrl>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>

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
        KMessageBox::error(nullptr, i18n("Unable to contact Akregator via D-Bus"),
                                    i18nc("@title:window", "D-Bus call failed"));
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
