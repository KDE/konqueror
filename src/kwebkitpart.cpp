/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
 * Copyright (C) 2008 - 2010 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
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

#include "kwebkitpart.h"

#include "kwebkitpart_p.h"
#include "kwebkitpart_ext.h"
#include "webview.h"
#include "webpage.h"
#include "websslinfo.h"

#include "ui/searchbar.h"
#include "settings/webkitsettings.h"

#include <KDE/KAboutData>
#include <KDE/KComponentData>
#include <KDE/KDebug>
#include <kdeversion.h>
#include <kio/global.h>

#include <QtGui/QApplication>
#include <QtWebKit/QWebFrame>

#define QL1S(x)  QLatin1String(x)
#define QL1C(x)  QLatin1Char(x)

static void setFormData(WebFrameState &frameState, const QString &data)
{
    const QStringList formDataList = data.split(QL1C(';'));
    Q_FOREACH(const QString &formData, formDataList) {
        QStringList items = formData.split(QL1C(','));
        if (items.count() > 1) {
            kDebug() << "formData:" << items;
            frameState.formData.insert(items.at(0), items.at(1));
        }
    }
}

static inline int convertStr2Int(const QString& value)
{
   bool ok;
   const int tempValue = value.toInt(&ok);

   if (ok)
     return tempValue;

   return 0;
}

KWebKitPart::KWebKitPart(QWidget *parentWidget, QObject *parent, const QStringList &/*args*/)
            :KParts::ReadOnlyPart(parent), d(new KWebKitPartPrivate(this))
{
    KAboutData about = KAboutData("kwebkitpart", 0,
                                  ki18nc("Program Name", "KWebKitPart"),
                                  /*version*/ "0.9",
                                  ki18nc("Short Description", "QtWebKit Browser Engine Component"),
                                  KAboutData::License_LGPL,
                                  ki18n("(C) 2009-2010 Dawit Alemayehu\n"
                                        "(C) 2008-2010 Urs Wolfer\n"
                                        "(C) 2007 Trolltech ASA"));

    about.addAuthor(ki18n("Urs Wolfer"), ki18n("Maintainer, Developer"), "uwolfer@kde.org");
    about.addAuthor(ki18n("Dawit Alemayehu"), ki18n("Developer"), "adawit@kde.org");
    about.addAuthor(ki18n("Michael Howell"), ki18n("Developer"), "mhowell123@gmail.com");
    about.addAuthor(ki18n("Laurent Montel"), ki18n("Developer"), "montel@kde.org");
    about.addAuthor(ki18n("Dirk Mueller"), ki18n("Developer"), "mueller@kde.org");
    KComponentData componentData(&about);
    setComponentData(componentData);

    // NOTE: If the application does not set its version number, we automatically
    // set it to KDE's version number so that the default user-agent string contains
    // proper application version number information. See QWebPage::userAgentForUrl...
    if (QCoreApplication::applicationVersion().isEmpty())
        QCoreApplication::setApplicationVersion(QString("%1.%2.%3")
                                                .arg(KDE::versionMajor())
                                                .arg(KDE::versionMinor())
                                                .arg(KDE::versionRelease()));

    QWidget *mainWidget = new QWidget (parentWidget);
    mainWidget->setObjectName("kwebkitpart");
    setWidget(mainWidget);

    d->init(mainWidget);
    setXMLFile("kwebkitpart.rc");
    d->initActions();
}

KWebKitPart::~KWebKitPart()
{
    delete d;
}

bool KWebKitPart::openUrl(const KUrl &u)
{
    kDebug() << u;

    // Ignore empty requests...
    if (u.isEmpty())
        return false;

    // Do not update history when url is typed in since konqueror
    // automatically does that itself.
    d->updateHistory = false;

    // Handle error conditions...
    if (u.protocol().compare(QL1S("error"), Qt::CaseInsensitive) == 0 && u.hasSubUrl()) {
        /**
         * The format of the error url is that two variables are passed in the query:
         * error = int kio error code, errText = QString error text from kio
         * and the URL where the error happened is passed as a sub URL.
         */
        KUrl::List urls = KUrl::split(u);

        if ( urls.count() > 1 ) {
            KUrl mainURL = urls.first();
            int error = convertStr2Int(mainURL.queryItem("error"));

            // error=0 isn't a valid error code, so 0 means it's missing from the URL
            if ( error == 0 )
                error = KIO::ERR_UNKNOWN;

            const QString errorText = mainURL.queryItem( "errText" );
            urls.pop_front();
            KUrl reqUrl = KUrl::join( urls );
            emit d->browserExtension->setLocationBarUrl(reqUrl.prettyUrl());
            d->webView->setHtml(d->webPage->errorPage(error, errorText, reqUrl));
            return true;
        }

        return false;
    }

    // Set the url...
    setUrl(u);

    // Ignore about:blank urls...
    if (u.url() == "about:blank") {
        emit setWindowCaption (u.url());
        emit completed();
        return true;
    } else {
        KParts::BrowserArguments bargs (browserExtension()->browserArguments());
        KParts::OpenUrlArguments args (arguments());
        KIO::MetaData metaData (args.metaData());

        // Get the SSL information sent, if any...
        if (metaData.contains(QL1S("ssl_in_use"))) {
            WebSslInfo sslinfo;
            sslinfo.fromMetaData(metaData.toVariant());
            sslinfo.setUrl(u);
            d->webPage->setSslInfo(sslinfo);
        }

        // Check if this is a restore state request, i.e. a history navigation
        // or session restore request. If it is, set the state information so
        // that the page can be properly restored...
        if (metaData.contains(QL1S("kwebkitpart-restore-state"))) {
            WebFrameState frameState;
            frameState.url = u;
            frameState.scrollPosX = args.xOffset();
            frameState.scrollPosY = args.yOffset();
            setFormData(frameState, metaData.value(QL1S("kwebkitpart-saved-form-data")));
            d->webPage->saveFrameState(QString(), frameState);

            /*
               'docState' contains information about child frame documents in
               the following order:
               0 => Frame name
               1 => Frame url
               2 => Frame scroll position X
               3 => Frame scroll position Y
               4 => Frame form data information in a name/value pair format:
                      name=value;[...;nameN=valueN]
                    where name identifies the input widget (input, textarea) in
                    CSS selector language format.
            */
            const int count = bargs.docState.count();
            for (int i = 0; i < count; i += 5) {
                frameState.url = bargs.docState.at(i+1);
                frameState.scrollPosX = convertStr2Int(bargs.docState.at(i+2));
                frameState.scrollPosY = convertStr2Int(bargs.docState.at(i+3));
                setFormData(frameState, bargs.docState.at(i+4));
                d->webPage->saveFrameState(bargs.docState.at(i), frameState);
            }
        }

        d->webView->loadUrl(u, args, bargs);
    }

    return true;
}

bool KWebKitPart::closeUrl()
{
    d->webView->stop();
    return true;
}

QWebView * KWebKitPart::view()
{
    return d->webView;
}

void KWebKitPart::guiActivateEvent(KParts::GUIActivateEvent *event)
{
    Q_UNUSED(event);
    // just overwrite, but do nothing for the moment
}

bool KWebKitPart::openFile()
{
    // never reached
    return false;
}
