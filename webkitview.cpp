/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
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

#include "webkitview.h"
#include "webkitpart.h"

#include <KDE/KParts/GenericFactory>
#include <KDE/KAboutData>

#include <QHttpRequestHeader>

WebView::WebView(WebKitPart *wpart, QWidget *parent)
    : QWebView(parent), part(wpart)
{
}

#if 0
QWebPage::NavigationRequestResponse WebPage::navigationRequested(QWebFrame *frame, const QWebNetworkRequest &request)
{
    if (frame != mainFrame())
        return AcceptNavigationRequest;
    return part->navigationRequested(request);
}
#endif

void WebView::contextMenuEvent(QContextMenuEvent *e)
{
    KParts::BrowserExtension::PopupFlags flags = KParts::BrowserExtension::DefaultPopupItems;
    flags |= KParts::BrowserExtension::ShowReload;
    flags |= KParts::BrowserExtension::ShowBookmark;
    flags |= KParts::BrowserExtension::ShowNavigationItems;
    emit part->browserExt()->popupMenu(/*guiclient */
                                       e->globalPos(), part->url(), 0, KParts::OpenUrlArguments(), KParts::BrowserArguments(),
                                       flags);
}
