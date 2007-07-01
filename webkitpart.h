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
#ifndef WEBKITPART_H
#define WEBKITPART_H

#include <KDE/KParts/ReadOnlyPart>
#include <KDE/KParts/BrowserExtension>

#include <qwebpage.h>

class WebPage;
class QWebFrame;
class KAboutData;
class WebKitBrowserExtension;
class KWebNetworkInterface;

class WebKitPart : public KParts::ReadOnlyPart
{
    Q_OBJECT
public:
    WebKitPart(QWidget *parentWidget, QObject *parent, const QStringList &/*args*/);
    ~WebKitPart();

    virtual bool openUrl(const KUrl &url);
    virtual bool closeUrl();

    QWebPage::NavigationRequestResponse navigationRequested(const QWebNetworkRequest &request);

    inline WebPage *page() { return webPage; }
    inline WebKitBrowserExtension *browserExt() const { return browserExtension; }

    static KAboutData *createAboutData();

protected:
    virtual bool openFile();

private slots:
    void frameStarted(QWebFrame *frame);
    void frameFinished(QWebFrame *frame);

private:
    WebPage *webPage;
    WebKitBrowserExtension *browserExtension;
};

class WebPage : public QWebPage
{
public:
    WebPage(WebKitPart *wpart, QWidget *parent);

protected:
    virtual NavigationRequestResponse navigationRequested(QWebFrame *frame, const QWebNetworkRequest &request);

    virtual void contextMenuEvent(QContextMenuEvent *e);

private:
    WebKitPart *part;
};

class WebKitBrowserExtension : public KParts::BrowserExtension
{
    Q_OBJECT
public:
    WebKitBrowserExtension(WebKitPart *parent);

public slots:
    void cut();
    void copy();
    void paste();

private slots:
    void updateEditActions();

private:
    WebKitPart *part;
};

#endif // WEBKITPART_H
