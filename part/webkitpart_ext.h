/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2009 Dawit Alemayehu <adawit@kde.org>
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

#ifndef WEBKITPART_EXT_H
#define WEBKITPART_EXT_H

#include <KDE/KParts/BrowserExtension>

class QWebView;
class KUrl;
class WebKitPart;

class WebKitBrowserExtension : public KParts::BrowserExtension
{
    Q_OBJECT

public:
    WebKitBrowserExtension(WebKitPart *parent);
    ~WebKitBrowserExtension();

  virtual void saveState( QDataStream &);
  virtual void restoreState( QDataStream &);

Q_SIGNALS:
    void saveUrl(const KUrl &);

public Q_SLOTS:
    void cut();
    void copy();
    void paste();
    void slotSaveDocument();
    void slotSaveFrame();
    void print();
    void printFrame();
    void searchProvider();
    void reparseConfiguration();

    void zoomIn();
    void zoomOut();
    void zoomNormal();
    void toogleZoomTextOnly();
    void slotSelectAll();

    void slotFrameInWindow();
    void slotFrameInTab();
    void slotFrameInTop();

    void slotSaveImageAs();
    void slotSendImage();
    void slotCopyImage();
    void slotViewImage();

    void slotCopyLinkLocation();
    void slotSaveLinkAs();

    void slotViewDocumentSource();

    void updateEditActions();

private:
    class WebKitBrowserExtensionPrivate;
    WebKitBrowserExtensionPrivate* const d;
};

#endif // WEBKITPART_EXT_H
