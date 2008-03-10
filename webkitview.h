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
#ifndef WEBKITVIEW_H
#define WEBKITVIEW_H

#include <KDE/KParts/ReadOnlyPart>
#include <KDE/KParts/BrowserExtension>

#include <qwebview.h>

class WebView;
class QWebFrame;
class KAboutData;
class WebKitBrowserExtension;
class KWebNetworkInterface;
class WebKitPart;

class WebView : public QWebView
{
public:
    WebView(WebKitPart *wpart, QWidget *parent);

protected:
//     virtual NavigationRequestResponse navigationRequested(QWebFrame *frame, const QWebNetworkRequest &request);

    virtual void contextMenuEvent(QContextMenuEvent *e);

private:
    WebKitPart *part;
};

#endif // WEBKITVIEW_H
