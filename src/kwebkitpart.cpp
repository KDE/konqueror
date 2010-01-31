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
#include <KDE/KConfigGroup>
#include <KDE/KAction>
#include <KDE/KActionCollection>
#include <KDE/KStandardDirs>
#include <KDE/KIconLoader>
#include <KDE/KGlobal>
#include <KDE/KDebug>
#include <kdeversion.h>
#include <kio/global.h>

#include <QtCore/QFile>
#include <QtGui/QApplication>

#include <QtWebKit/QWebFrame>

#define QL1S(x)  QLatin1String(x)
#define QL1C(x)  QLatin1Char(x)

static QString htmlError (int code, const QString& text, const KUrl& reqUrl)
{
  QString errorName, techName, description;
  QStringList causes, solutions;

  QByteArray raw = KIO::rawErrorDetail( code, text, &reqUrl );
  QDataStream stream(raw);

  stream >> errorName >> techName >> description >> causes >> solutions;

  QString url, protocol, datetime;
  url = reqUrl.url();
  protocol = reqUrl.protocol();
  datetime = KGlobal::locale()->formatDateTime( QDateTime::currentDateTime(),
                                                KLocale::LongDate );

  QString filename( KStandardDirs::locate( "data", "kwebkitpart/error.html" ) );
  QFile file( filename );
  bool isOpened = file.open( QIODevice::ReadOnly );
  if ( !isOpened )
    kWarning() << "Could not open error html template:" << filename;

  QString html = QString( QL1S( file.readAll() ) );

  html.replace( QL1S( "TITLE" ), i18n( "Error: %1", errorName ) );
  html.replace( QL1S( "DIRECTION" ), QApplication::isRightToLeft() ? "rtl" : "ltr" );
  html.replace( QL1S( "ICON_PATH" ), KUrl(KIconLoader::global()->iconPath("dialog-warning", -KIconLoader::SizeHuge)).url() );

  QString doc = QL1S( "<h1>" );
  doc += i18n( "The requested operation could not be completed" );
  doc += QL1S( "</h1><h2>" );
  doc += errorName;
  doc += QL1S( "</h2>" );

  if ( !techName.isNull() ) {
    doc += QL1S( "<h2>" );
    doc += i18n( "Technical Reason: %1", techName );
    doc += QL1S( "</h2>" );
  }

  doc += QL1S( "<h3>" );
  doc += i18n( "Details of the Request:" );
  doc += QL1S( "</h3><ul><li>" );
  doc += i18n( "URL: %1" ,  url );
  doc += QL1S( "</li><li>" );

  if ( !protocol.isNull() ) {
    doc += i18n( "Protocol: %1", protocol );
    doc += QL1S( "</li><li>" );
  }

  doc += i18n( "Date and Time: %1" ,  datetime );
  doc += QL1S( "</li><li>" );
  doc += i18n( "Additional Information: %1" ,  text );
  doc += QL1S( "</li></ul><h3>" );
  doc += i18n( "Description:" );
  doc += QL1S( "</h3><p>" );
  doc += description;
  doc += QL1S( "</p>" );

  if ( causes.count() ) {
    doc += QL1S( "<h3>" );
    doc += i18n( "Possible Causes:" );
    doc += QL1S( "</h3><ul><li>" );
    doc += causes.join( "</li><li>" );
    doc += QL1S( "</li></ul>" );
  }

  if ( solutions.count() ) {
    doc += QL1S( "<h3>" );
    doc += i18n( "Possible Solutions:" );
    doc += QL1S( "</h3><ul><li>" );
    doc += solutions.join( "</li><li>" );
    doc += QL1S( "</li></ul>" );
  }

  html.replace( QL1S("TEXT"), doc );

  return html;
}

KWebKitPart::KWebKitPart(QWidget *parentWidget, QObject *parent, const QStringList &/*args*/)
            :KParts::ReadOnlyPart(parent), d(new KWebKitPartPrivate(this))
{
    KAboutData about = KAboutData("kwebkitpart", 0,
                                  ki18nc("component about data name", "WebKit Browser Engine Component"),
                                  /*version*/ "0.9", /*ki18n("shortDescription")*/ KLocalizedString(),
                                  KAboutData::License_LGPL,
                                  ki18n("(c) 2009-2010 Dawit Alemayehu\n"
                                        "(c) 2008-2010 Urs Wolfer\n"
                                        "(c) 2007 Trolltech ASA"));

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
    initAction();
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
    if (handleError(u, d->webView->page()->mainFrame())) {
        return true;
    }

    // Set the url...
    setUrl(u);

    if (u.url() == "about:blank") {
        emit setWindowCaption (u.url());
        d->webView->setUrl(u);
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
            d->webPage->saveFrameState(QString(), frameState);

            const int count = bargs.docState.count();
            for (int i = 0; i < count; i += 4) {
                frameState.url = bargs.docState.at(i+1);
                frameState.scrollPosX = bargs.docState.at(i+2).toInt();
                frameState.scrollPosY = bargs.docState.at(i+3).toInt();
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

void KWebKitPart::initAction()
{
    KAction *action = actionCollection()->addAction(KStandardAction::SaveAs, "saveDocument",
                                                    d->browserExtension, SLOT(slotSaveDocument()));

    action = new KAction(i18n("Save &Frame As..."), this);
    actionCollection()->addAction("saveFrame", action);
    connect(action, SIGNAL(triggered(bool)), d->browserExtension, SLOT(slotSaveFrame()));

    action = new KAction(KIcon("document-print-frame"), i18n("Print Frame..."), this);
    actionCollection()->addAction("printFrame", action);
    connect(action, SIGNAL(triggered(bool)), d->browserExtension, SLOT(printFrame()));

    action = new KAction(KIcon("zoom-in"), i18nc("zoom in action", "Zoom In"), this);
    actionCollection()->addAction("zoomIn", action);
    action->setShortcut(KShortcut("CTRL++; CTRL+="));
    connect(action, SIGNAL(triggered(bool)), d->browserExtension, SLOT(zoomIn()));

    action = new KAction(KIcon("zoom-out"), i18nc("zoom out action", "Zoom Out"), this);
    actionCollection()->addAction("zoomOut", action);
    action->setShortcut(KShortcut("CTRL+-; CTRL+_"));
    connect(action, SIGNAL(triggered(bool)), d->browserExtension, SLOT(zoomOut()));

    action = new KAction(KIcon("zoom-original"), i18nc("reset zoom action", "Actual Size"), this);
    actionCollection()->addAction("zoomNormal", action);
    action->setShortcut(KShortcut("CTRL+0"));
    connect(action, SIGNAL(triggered(bool)), d->browserExtension, SLOT(zoomNormal()));

    action = new KAction(i18n("Zoom Text Only"), this);
    action->setCheckable(true);
    KConfigGroup cgHtml(KGlobal::config(), "HTML Settings");
    bool zoomTextOnly = cgHtml.readEntry("ZoomTextOnly", false);
    action->setChecked(zoomTextOnly);
    actionCollection()->addAction("zoomTextOnly", action);
    connect(action, SIGNAL(triggered(bool)), d->browserExtension, SLOT(toogleZoomTextOnly()));

    action = actionCollection()->addAction(KStandardAction::SelectAll, "selectAll",
                                           d->browserExtension, SLOT(slotSelectAll()));
    action->setShortcutContext(Qt::WidgetShortcut);
    d->webView->addAction(action);

    action = new KAction(i18n("View Do&cument Source"), this);
    actionCollection()->addAction("viewDocumentSource", action);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_U));
    connect(action, SIGNAL(triggered(bool)), d->browserExtension, SLOT(slotViewDocumentSource()));

    action = new KAction(i18nc("Secure Sockets Layer", "SSL"), this);
    actionCollection()->addAction("security", action);
    connect(action, SIGNAL(triggered(bool)), d, SLOT(slotShowSecurity()));

    action = actionCollection()->addAction(KStandardAction::Find, "find", d, SLOT(slotShowSearchBar()));
    action->setWhatsThis(i18nc("find action \"whats this\" text", "<h3>Find text</h3>"
                              "Shows a dialog that allows you to find text on the displayed page."));

    action = actionCollection()->addAction(KStandardAction::FindNext, "findnext",
                                           d->searchBar, SLOT(findNext()));
    action = actionCollection()->addAction(KStandardAction::FindPrev, "findprev",
                                           d->searchBar, SLOT(findPrevious()));
}

bool KWebKitPart::handleError(const KUrl &u, QWebFrame *frame)
{
    if ( u.protocol() == "error" && u.hasSubUrl() ) {
        /**
         * The format of the error url is that two variables are passed in the query:
         * error = int kio error code, errText = QString error text from kio
         * and the URL where the error happened is passed as a sub URL.
         */
        KUrl::List urls = KUrl::split(u);

        if ( urls.count() > 1 ) {
            KUrl mainURL = urls.first();
            int error = mainURL.queryItem( "error" ).toInt();

            // error=0 isn't a valid error code, so 0 means it's missing from the URL
            if ( error == 0 )
                error = KIO::ERR_UNKNOWN;

            if (error == KIO::ERR_USER_CANCELED) {
                setUrl(d->webView->url());
                emit d->browserExtension->setLocationBarUrl(KUrl(d->webView->url()).prettyUrl());
            } else {
                const QString errorText = mainURL.queryItem( "errText" );
                urls.pop_front();
                KUrl reqUrl = KUrl::join( urls );
                frame->setHtml(htmlError(error, errorText, reqUrl), reqUrl);
            }
            return true;
        }
    }

    return false;
}
