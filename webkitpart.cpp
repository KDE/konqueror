/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
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
#include "webkitview.h"

#include <KDE/KParts/GenericFactory>
#include <KDE/KAboutData>
#include <KDE/KUriFilterData>
#include <KDE/KDesktopFile>
#include <KDE/KConfigGroup>
#include <KDE/KAction>
#include <KDE/KActionCollection>
#include <KDE/KToolInvocation>
#include <QHttpRequestHeader>
#include <QtWebKit/QWebHitTestResult>
#include <QClipboard>

#include "searchwidget.h"

// #include "kwebnetworkinterface.h"

WebKitPart::WebKitPart(QWidget *parentWidget, QObject *parent, const QStringList &/*args*/)
    : KParts::ReadOnlyPart(parent)
{
    webView = new WebView(this, parentWidget);
//     webView->setNetworkInterface(new KWebNetworkInterface(this));
    setWidget(webView);

    connect(webView, SIGNAL(loadStarted()),
            this, SLOT(loadStarted()));
    connect(webView, SIGNAL(loadFinished()),
            this, SLOT(loadFinished()));
    connect(webView, SIGNAL(titleChanged(const QString &)),
            this, SIGNAL(setWindowCaption(const QString &)));

    connect(webView->page(), SIGNAL(linkHovered(const QString &, const QString &, const QString &)),
            this, SIGNAL(setStatusBarText(const QString &)));

    browserExtension = new WebKitBrowserExtension(this);

    connect(webView->page(), SIGNAL(loadProgress(int)),
            browserExtension, SIGNAL(loadingProgress(int)));
    connect(webView, SIGNAL(urlChanged(const QUrl &)),
            this, SLOT(urlChanged(const QUrl &)));

    initAction();
    setXMLFile( "webkitpart.rc" );
}

WebKitPart::~WebKitPart()
{
}

void WebKitPart::initAction()
{
    KAction *action = new KAction( KIcon(  "zoom-in" ), i18n( "Enlarge Font" ), this );
    actionCollection()->addAction( "incFontSizes", action );
    connect( action, SIGNAL(triggered(bool)), browserExt(), SLOT( zoomIn() ) );

    action = new KAction( KIcon(  "zoom-out" ),i18n( "Shrink Font" ), this );
    actionCollection()->addAction( "decFontSizes", action );
    connect( action, SIGNAL(triggered(bool)), browserExt(), SLOT( zoomOut() ) );


    action = actionCollection()->addAction( KStandardAction::Find, "find", browserExt(), SLOT( slotFind() ) );
    action->setWhatsThis( i18n( "Find text<br /><br />"
                                   "Shows a dialog that allows you to find text on the displayed page." ) );
}

bool WebKitPart::openUrl(const KUrl &url)
{
    setUrl(url);

#if 0
    KParts::OpenUrlArguments arguments;
    KParts::BrowserArguments browserArguments = browserExtension->browserArguments();
    QString headerString = browserArguments.doPost() ? "POST" : "GET";
    headerString += QLatin1Char(' ');
    headerString += url.toEncoded(QUrl::RemoveScheme|QUrl::RemoveAuthority);
    headerString += QLatin1String(" HTTP/1.1\n\n"); // ### does it matter?
    headerString += arguments.metaData().value("customHTTPHeader");
    QHttpRequestHeader header(headerString);

    QWebNetworkRequest request(url, browserArguments.doPost() ? QWebNetworkRequest::Post : QWebNetworkRequest::Get,
                               browserArguments.postData);

    foreach (QString key, header.keys())
        request.setHttpHeaderField(key, header.value(key));
#endif

    webView->load(url);

    return true;
}

bool WebKitPart::closeUrl()
{
    webView->stop();
    return true;
}

bool WebKitPart::openFile()
{
    // never reached
    return false;
}

void WebKitPart::loadStarted()
{
    emit started(0);
}

void WebKitPart::loadFinished()
{
    emit completed();
}

void WebKitPart::urlChanged(const QUrl &url)
{
    emit browserExtension->setLocationBarUrl(KUrl(url).prettyUrl());
}

#if 0
QWebPage::NavigationRequestResponse WebKitPart::navigationRequested(const QWebNetworkRequest &request)
{
    KParts::OpenUrlArguments arguments;
    KParts::BrowserArguments browserArguments;
    browserArguments.postData = request.postData();
    if (!browserArguments.postData.isEmpty())
        browserArguments.setDoPost(true);

    arguments.metaData().unite(KWebNetworkInterface::metaDataForRequest(request.httpHeader()));

    emit browserExtension->openUrlRequest(request.url(), arguments, browserArguments);

    return QWebPage::IgnoreNavigationRequest;
}
#endif

KAboutData *WebKitPart::createAboutData()
{
    return new KAboutData("webkitpart", 0, ki18n("Webkit HTML Component"),
                          /*version*/ "1.0", ki18n(/*shortDescription*/ ""),
                          KAboutData::License_LGPL,
                          ki18n("Copyright (c) 2007 Trolltech ASA"));
}

WebKitBrowserExtension::WebKitBrowserExtension(WebKitPart *parent)
    : KParts::BrowserExtension(parent), part(parent)
{
    connect(part->view()->page(), SIGNAL(selectionChanged()),
            this, SLOT(updateEditActions()));

    enableAction("cut", false);
    enableAction("copy", false);
    enableAction("paste", false);
}

void WebKitBrowserExtension::cut()
{
    part->view()->page()->triggerAction(QWebPage::Cut);
}

void WebKitBrowserExtension::copy()
{
    part->view()->page()->triggerAction(QWebPage::Copy);
}

void WebKitBrowserExtension::paste()
{
    part->view()->page()->triggerAction(QWebPage::Paste);
}

void WebKitBrowserExtension::updateEditActions()
{
    QWebPage *page = part->view()->page();
    enableAction("cut", page->action(QWebPage::Cut));
    enableAction("copy", page->action(QWebPage::Copy));
    enableAction("paste", page->action(QWebPage::Paste));
}

void WebKitBrowserExtension::searchProvider()
{
    // action name is of form "previewProvider[<searchproviderprefix>:]"
    const QString searchProviderPrefix = QString( sender()->objectName() ).mid( 14 );

    KUriFilterData data;
    QStringList list;
    data.setData( searchProviderPrefix + part->view()->page()->selectedText() );
    list << "kurisearchfilter" << "kuriikwsfilter";

    if( !KUriFilter::self()->filterUri(data, list) )
    {
        KDesktopFile file("services", "searchproviders/google.desktop");
        QString encodedSearchTerm = QUrl::toPercentEncoding(part->view()->page()->selectedText());
        KConfigGroup cg(file.desktopGroup());
        data.setData(cg.readEntry("Query").replace("\\{@}", encodedSearchTerm));
    }

    KParts::BrowserArguments browserArgs;
    browserArgs.frameName = "_blank";

    emit openUrlRequest( data.uri(), KParts::OpenUrlArguments(), browserArgs );
}

void WebKitBrowserExtension::zoomIn()
{
    part->view()->setTextSizeMultiplier( part->view()->textSizeMultiplier()*2 );
}

void WebKitBrowserExtension::zoomOut()
{
    part->view()->setTextSizeMultiplier( part->view()->textSizeMultiplier()/2 );
}

void WebKitBrowserExtension::slotFind()
{
    SearchWidgetDialog *dlg = new SearchWidgetDialog( part->view(), part->view()->page() );
    dlg->exec();
    delete dlg;
}


void WebKitBrowserExtension::slotFrameInWindow()
{
    KParts::OpenUrlArguments args;// = d->m_khtml->arguments();
    args.metaData()["referrer"] = part->view()->contextMenuResult().linkText();
    args.metaData()["forcenewwindow"] = "true";
    emit createNewWindow( part->view()->contextMenuResult().linkUrl(), args );
}

void WebKitBrowserExtension::slotFrameInTab()
{
    KParts::OpenUrlArguments args;// = d->m_khtml->arguments();
    args.metaData()["referrer"] = part->view()->contextMenuResult().linkText();
    KParts::BrowserArguments browserArgs;//( d->m_khtml->browserExtension()->browserArguments() );
    browserArgs.setNewTab(true);
    emit createNewWindow( part->view()->contextMenuResult().linkUrl(), args, browserArgs );
}

void WebKitBrowserExtension::slotFrameInTop()
{
    KParts::OpenUrlArguments args;// = d->m_khtml->arguments();
    args.metaData()["referrer"] = part->view()->contextMenuResult().linkText();
    KParts::BrowserArguments browserArgs;//( d->m_khtml->browserExtension()->browserArguments() );
    browserArgs.frameName = "_top";
    emit openUrlRequest( part->view()->contextMenuResult().linkUrl(), args, browserArgs );
}

void WebKitBrowserExtension::slotSaveImageAs()
{
    //TODO
}

void WebKitBrowserExtension::slotSendImage()
{
    QStringList urls;
    urls.append( part->view()->contextMenuResult().imageUrl().path());
    QString subject = part->view()->contextMenuResult().imageUrl().path();
    KToolInvocation::invokeMailer(QString(), QString(), QString(), subject,
                       QString(), //body
                       QString(),
                       urls); // attachments
}

void WebKitBrowserExtension::slotCopyImage()
{
    KUrl safeURL(part->view()->contextMenuResult().imageUrl());
    safeURL.setPass(QString());

    // Set it in both the mouse selection and in the clipboard
    QMimeData* mimeData = new QMimeData;
    mimeData->setImageData(part->view()->contextMenuResult().pixmap() );
    safeURL.populateMimeData( mimeData );
    QApplication::clipboard()->setMimeData( mimeData, QClipboard::Clipboard );

    mimeData = new QMimeData;
    mimeData->setImageData( part->view()->contextMenuResult().pixmap() );
    safeURL.populateMimeData( mimeData );
    QApplication::clipboard()->setMimeData( mimeData, QClipboard::Selection );
}

typedef KParts::GenericFactory<WebKitPart> Factory;
Q_EXPORT_PLUGIN(Factory);

#include "webkitpart.moc"
