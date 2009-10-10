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

#ifndef KNETWORKCOOKIEJAR_H
#define KNETWORKCOOKIEJAR_H

#include "kdewebkit_export.h"

#include <QtNetwork/QNetworkCookieJar>

/**
 * @short A KDE implementation of QNetworkCookieJar.
 *
 * Use this class in place of QNetworkCookieJar if you want to integrate with
 * KDE's cookiejar instead of the one that comes with Qt.
 *
 * Here is a simple example that shows how to switch QtWebKit to use KDE's
 * cookiejar:
 *
 *   QWebView *view = new QWebView(this);
 *   KNetworkCookieJar *cookiejar = new KNetworkCookieJar;
 *   cookiejar->setWindowId(view->window()->winId());
 *   view->page()->networkAccessManager()->setCookieJar(cookiejar);
 *
 * To access member functions in the cookiejar class at a later point in your
 * code simply downcast the pointer returned by QNetworkAccessManager::cookieJar
 * as follows:
 *
 *   KNetworkCookieJar *cookiejar = qobject_cast<KNetworkCookieJar*>(view->page()->accessManager()->cookieJar());
 *
 * NOTE: This class is not a replacement for the standard KDE API. It should
 * ONLY be used to to provide KDE integration in applications that cannot use
 * the standard KDE API directly.

 * @see QNetworkAccessManager::setCookieJar for details.
 *
 * @since 4.4
 */
class KDEWEBKIT_EXPORT KNetworkCookieJar : public QNetworkCookieJar
{
    Q_OBJECT
public:
    KNetworkCookieJar(QObject *parent = 0);
    ~KNetworkCookieJar();

   /**
    * Returns the currently set window id. The default value is -1.
    */
    qlonglong windowId() const;

    /**
     * Sets the window id of the application.
     *
     * This value is used by KDE's cookiejar to manage session cookies, namely
     * to delete them when the last application refering to such cookies is
     * closed by the end user.
     *
     * @see QWidget::window()
     * @see QWidget::winId()
     *
     * @param id  the value of @ref QWidget::winId() from the window that contains your widget.
     */
    void setWindowId(qlonglong id);

    /**
     * Reparse the KDE cookiejar configuration file.
     */
    void reparseConfiguration();

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QNetworkCookieJar::cookiesForUrl
     * @internal
     */
    QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const;

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QNetworkCookieJar::setCookiesFromUrl
     * @internal
     */
    bool setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url);

private:
    class KNetworkCookieJarPrivate;
    KNetworkCookieJarPrivate* const d;
};

#endif // KIO_COOKIEJAR_H
