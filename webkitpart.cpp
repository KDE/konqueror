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
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "webkitpart.h"

#include <KDE/KParts/GenericFactory>
#include <KDE/KAboutData>

#include <qwebpage.h>

WebKitPart::WebKitPart(QWidget *parentWidget, QObject *parent, const QStringList &/*args*/)
    : KParts::ReadOnlyPart(parent)
{
    webPage = new QWebPage(parentWidget);
    setWidget(webPage);

    connect(webPage, SIGNAL(loadStarted(QWebFrame *)),
            this, SLOT(frameStarted(QWebFrame *)));
    connect(webPage, SIGNAL(loadFinished(QWebFrame *)),
            this, SLOT(frameFinished(QWebFrame *)));
    connect(webPage, SIGNAL(titleChanged(const QString &)),
            this, SIGNAL(setWindowCaption(const QString &)));

    browserExtension = new WebKitBrowserExtension(this);

    connect(webPage, SIGNAL(loadProgressChanged(int)),
            browserExtension, SIGNAL(loadingProgress(int)));
}

bool WebKitPart::openUrl(const KUrl &url)
{
    webPage->open(url);
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

KAboutData *WebKitPart::createAboutData()
{
    return new KAboutData("webkitpart", I18N_NOOP("Webkit HTML Component"),
                          /*version*/ "1.0", /*shortDescription*/ "",
                          KAboutData::License_LGPL,
                          I18N_NOOP("Copyright (c) 2007 Trolltech ASA"));
}

WebKitBrowserExtension::WebKitBrowserExtension(WebKitPart *parent)
    : KParts::BrowserExtension(parent)
{
}

typedef KParts::GenericFactory<WebKitPart> Factory;
Q_EXPORT_PLUGIN(Factory);

#include "webkitpart.moc"

