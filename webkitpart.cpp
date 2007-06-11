/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
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

#include "webkitpart.h"

#include <KDE/KParts/GenericFactory>
#include <KDE/KAboutData>

#include "kwebnetworkinterface.h"

WebKitPart::WebKitPart(QWidget *parentWidget, QObject *parent, const QStringList &/*args*/)
    : KParts::ReadOnlyPart(parent)
{
    webPage = new WebPage(this, parentWidget);
    webPage->setNetworkInterface(new KWebNetworkInterface(this));
    setWidget(webPage);

    connect(webPage, SIGNAL(loadStarted(QWebFrame *)),
            this, SLOT(frameStarted(QWebFrame *)));
    connect(webPage, SIGNAL(loadFinished(QWebFrame *)),
            this, SLOT(frameFinished(QWebFrame *)));
    connect(webPage, SIGNAL(titleChanged(const QString &)),
            this, SIGNAL(setWindowCaption(const QString &)));

    connect(webPage, SIGNAL(hoveringOverLink(const QString &, const QString &)),
            this, SIGNAL(setStatusBarText(const QString &)));

    browserExtension = new WebKitBrowserExtension(this);

    connect(webPage, SIGNAL(loadProgressChanged(int)),
            browserExtension, SIGNAL(loadingProgress(int)));
}

WebKitPart::~WebKitPart()
{
}

bool WebKitPart::openUrl(const KUrl &url)
{
    setUrl(url);

    KParts::URLArgs args = browserExtension->urlArgs();

    QString headerString = args.doPost() ? "POST" : "GET";
    headerString += QLatin1Char(' ');
    headerString += url.toEncoded(QUrl::RemoveScheme|QUrl::RemoveAuthority);
    headerString += QLatin1String(" HTTP/1.1\n\n"); // ### does it matter?
    headerString += args.metaData().value("customHTTPHeader");

    webPage->open(url, QHttpRequestHeader(headerString), args.postData);

    return true;
}

bool WebKitPart::closeUrl()
{
    webPage->stop();
    return true;
}

bool WebKitPart::openFile()
{
    // never reached
    return false;
}

void WebKitPart::frameStarted(QWebFrame *frame)
{
    if (frame == webPage->mainFrame())
        emit started(0);
}

void WebKitPart::frameFinished(QWebFrame *frame)
{
    if (frame == webPage->mainFrame())
        emit completed();
}

QWebPage::NavigationRequestResponse WebKitPart::navigationRequested(const QUrl &url, const QHttpRequestHeader &request, const QByteArray &postData)
{
    KParts::URLArgs args;
    args.postData = postData;
    if (!postData.isEmpty())
        args.setDoPost(true);

    args.metaData().unite(KWebNetworkInterface::metaDataForRequest(request));

    emit browserExtension->openUrlRequest(url, args);

    return QWebPage::IgnoreNavigationRequest;
}

KAboutData *WebKitPart::createAboutData()
{
    return new KAboutData("webkitpart", I18N_NOOP("Webkit HTML Component"),
                          /*version*/ "1.0", /*shortDescription*/ "",
                          KAboutData::License_LGPL,
                          I18N_NOOP("Copyright (c) 2007 Trolltech ASA"));
}

WebPage::WebPage(WebKitPart *wpart, QWidget *parent)
    : QWebPage(parent), part(wpart)
{
}

QWebPage::NavigationRequestResponse WebPage::navigationRequested(QWebFrame *frame, const QUrl &url, const QHttpRequestHeader &request, const QByteArray &postData)
{
    if (frame != mainFrame())
        return AcceptNavigationRequest;
    return part->navigationRequested(url, request, postData);
}

WebKitBrowserExtension::WebKitBrowserExtension(WebKitPart *parent)
    : KParts::BrowserExtension(parent)
{
}

typedef KParts::GenericFactory<WebKitPart> Factory;
Q_EXPORT_PLUGIN(Factory);

#include "webkitpart.moc"

