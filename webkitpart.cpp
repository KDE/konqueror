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
    QHttpRequestHeader header(headerString);

    QWebNetworkRequest request(url, args.doPost() ? QWebNetworkRequest::Post : QWebNetworkRequest::Get,
                               args.postData);

    foreach (QString key, header.keys())
        request.setHttpHeaderField(key, header.value(key));

    webPage->open(request);

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

QWebPage::NavigationRequestResponse WebKitPart::navigationRequested(const QWebNetworkRequest &request)
{
    KParts::URLArgs args;
    args.postData = request.postData();
    if (!args.postData.isEmpty())
        args.setDoPost(true);

    args.metaData().unite(KWebNetworkInterface::metaDataForRequest(request.httpHeader()));

    emit browserExtension->openUrlRequest(request.url(), args);

    return QWebPage::IgnoreNavigationRequest;
}

KAboutData *WebKitPart::createAboutData()
{
    return new KAboutData("webkitpart", 0, ki18n("Webkit HTML Component"),
                          /*version*/ "1.0", ki18n(/*shortDescription*/ ""),
                          KAboutData::License_LGPL,
                          ki18n("Copyright (c) 2007 Trolltech ASA"));
}

WebPage::WebPage(WebKitPart *wpart, QWidget *parent)
    : QWebPage(parent), part(wpart)
{
}

QWebPage::NavigationRequestResponse WebPage::navigationRequested(QWebFrame *frame, const QWebNetworkRequest &request)
{
    if (frame != mainFrame())
        return AcceptNavigationRequest;
    return part->navigationRequested(request);
}

void WebPage::contextMenuEvent(QContextMenuEvent *e)
{
    KParts::BrowserExtension::PopupFlags flags = KParts::BrowserExtension::DefaultPopupItems;
    flags |= KParts::BrowserExtension::ShowReload;
    flags |= KParts::BrowserExtension::ShowBookmark;
    flags |= KParts::BrowserExtension::ShowNavigationItems;
    emit part->browserExt()->popupMenu(/*guiclient */0,
                                       e->globalPos(), part->url(), KParts::URLArgs(),
                                       flags);
}

WebKitBrowserExtension::WebKitBrowserExtension(WebKitPart *parent)
    : KParts::BrowserExtension(parent), part(parent)
{
    connect(part->page(), SIGNAL(selectionChanged()),
            this, SLOT(updateEditActions()));

    enableAction("cut", false);
    enableAction("copy", false);
    enableAction("paste", false);
}

void WebKitBrowserExtension::cut()
{
    part->page()->cut();
}

void WebKitBrowserExtension::copy()
{
    part->page()->copy();
}

void WebKitBrowserExtension::paste()
{
    part->page()->paste();
}

void WebKitBrowserExtension::updateEditActions()
{
    QWebPage *page = part->page();
    enableAction("cut", page->canCut());
    enableAction("copy", page->canCopy());
    enableAction("paste", page->canPaste());
}

typedef KParts::GenericFactory<WebKitPart> Factory;
Q_EXPORT_PLUGIN(Factory);

#include "webkitpart.moc"

