/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2009 Dawit Alemayehu <adawit @ kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "networkcookiejar_p.h"

#include <kconfiggroup.h>
#include <ksharedconfig.h>
#include <kdebug.h>

#include <QtCore/QUrl>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusReply>


namespace KDEPrivate {

class NetworkCookieJar::CookieJarPrivate
{
public:
  CookieJarPrivate(): windowId(-1), enabled(true) {}

  qlonglong windowId;
  bool enabled;
};


NetworkCookieJar::NetworkCookieJar(QObject* parent)
                 :QNetworkCookieJar(parent), d(new NetworkCookieJar::CookieJarPrivate) {
    reparseConfiguration();
}

NetworkCookieJar::~NetworkCookieJar() {
    delete d;
}

qlonglong NetworkCookieJar::windowId() const {
    return d->windowId;
}

QList<QNetworkCookie> NetworkCookieJar::cookiesForUrl(const QUrl &url) const {
    QList<QNetworkCookie> cookieList;

    if (d->enabled) {
        QDBusInterface kcookiejar("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer");
        QDBusReply<QString> reply = kcookiejar.call("findDOMCookies", url.toString(), d->windowId);

        if (reply.isValid()) {
            cookieList << reply.value().toUtf8();
            //kDebug() << url.host() << reply.value();
        } else {
            kWarning() << "Unable to communicate with the cookiejar!";
        }
    }

    return cookieList;
}

bool NetworkCookieJar::setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url) {
    if (d->enabled) {
        QDBusInterface kcookiejar("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer");

        QByteArray cookieHeader;
        Q_FOREACH(const QNetworkCookie &cookie, cookieList) {
            cookieHeader = "Set-Cookie: ";
            cookieHeader += cookie.toRawForm();
            kcookiejar.call("addCookies", url.toString(), cookieHeader, d->windowId);
            //kDebug() << "[" << d->windowId << "] Got Cookie: " << cookieHeader << " from " << url;
        }

        return !kcookiejar.lastError().isValid();
    }

    return false;
}

void NetworkCookieJar::setWindowId(qlonglong id) {
    d->windowId = id;
}

void NetworkCookieJar::reparseConfiguration() {
    KConfigGroup cfg = KSharedConfig::openConfig("kcookiejarrc", KConfig::NoGlobals)->group("Cookie Policy");
    d->enabled = cfg.readEntry("Cookies", true);
}

}

#include "networkcookiejar_p.moc"
