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
#ifndef WEBKITPART_H
#define WEBKITPART_H

#include <KDE/KParts/ReadOnlyPart>
#include <KDE/KParts/BrowserExtension>

#include <QtWebKit/QWebView>

#include "webkitkde_export.h"

class WebKitPageView;
class WebView;
class WebKitBrowserExtension;

class WEBKITKDE_EXPORT WebKitPart : public KParts::ReadOnlyPart
{
    Q_OBJECT
public:
    WebKitPart(QWidget *parentWidget = 0, QObject *parent = 0, const QStringList &/*args*/ = QStringList());
    ~WebKitPart();

    virtual bool openUrl(const KUrl &url);
    virtual bool closeUrl();

//     QWebPage::NavigationRequestResponse navigationRequested(const QWebNetworkRequest &request);

    WebView *view();
    WebKitBrowserExtension *browserExtension() const;

    /** required because KPart::setStatusBarText(..) is protected **/
    void setStatusBarTextProxy(const QString &message);

protected:
    virtual bool openFile();
    void initAction();

private Q_SLOTS:
    void loadStarted();
    void loadFinished();
    void urlChanged(const QUrl &url);

private:
    WebKitPageView *m_webPageView;
    WebKitBrowserExtension *m_browserExtension;
};

class WebKitBrowserExtension : public KParts::BrowserExtension
{
    Q_OBJECT
public:
    WebKitBrowserExtension(WebKitPart *parent);

public Q_SLOTS:
    void cut();
    void copy();
    void paste();
    void searchProvider();

    void zoomIn();
    void zoomOut();

    void slotFrameInWindow();
    void slotFrameInTab();
    void slotFrameInTop();

    void slotSaveImageAs();
    void slotSendImage();
    void slotCopyImage();

    void slotViewDocumentSource();

private Q_SLOTS:
    void updateEditActions();

private:
    WebKitPart *part;
};

#endif // WEBKITPART_H
