/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
 * Copyright (C) 2008 - 2009 Urs Wolfer <uwolfer @ kde.org>
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

#include "webview.h"
#include "webpage.h"
#include "websslinfo.h"

#include <kdewebkit/settings/webkitsettings.h>

#include <KDE/KParts/GenericFactory>
#include <KDE/KParts/Plugin>
#include <KDE/KAboutData>
#include <KDE/KUriFilterData>
#include <KDE/KDesktopFile>
#include <KDE/KConfigGroup>
#include <KDE/KAction>
#include <KDE/KActionCollection>
#include <KDE/KRun>
#include <KDE/KTemporaryFile>
#include <KDE/KToolInvocation>
#include <KDE/KFileDialog>
#include <KDE/KMessageBox>
#include <KDE/KStandardDirs>
#include <KDE/KIconLoader>
#include <KDE/KGlobal>

#include <KDE/KIO/NetAccess>
#include <kio/global.h>

#ifdef HAS_SSL_INFO_DIALOG
#include <kio/ksslinfodialog.h>
#endif

#include <QHttpRequestHeader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QFile>

#include <QtWebKit/QWebHistory>
#include <QtWebKit/QWebHitTestResult>
#include <QClipboard>
#include <QApplication>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QPrintPreviewDialog>


static QString htmlError (int code, const QString& text, const KUrl& reqUrl)
{
  kDebug(6050) << "errorCode" << code << "text" << text;

  QString errorName, techName, description;
  QStringList causes, solutions;

  QByteArray raw = KIO::rawErrorDetail( code, text, &reqUrl );
  QDataStream stream(raw);

  stream >> errorName >> techName >> description >> causes >> solutions;

  QString url, protocol, datetime;
  url = Qt::escape( reqUrl.prettyUrl() );
  protocol = reqUrl.protocol();
  datetime = KGlobal::locale()->formatDateTime( QDateTime::currentDateTime(),
                                                KLocale::LongDate );

  QString filename( KStandardDirs::locate( "data", "khtml/error.html" ) );
  QFile file( filename );
  bool isOpened = file.open( QIODevice::ReadOnly );
  if ( !isOpened )
    kWarning() << "Could not open error html template:" << filename;

  QString html = QString( QLatin1String( file.readAll() ) );

  html.replace( QLatin1String( "TITLE" ), i18n( "Error: %1 - %2", errorName, url ) );
  html.replace( QLatin1String( "DIRECTION" ), QApplication::isRightToLeft() ? "rtl" : "ltr" );
  html.replace( QLatin1String( "ICON_PATH" ), KIconLoader::global()->iconPath( "dialog-warning", -KIconLoader::SizeHuge ) );

  QString doc = QLatin1String( "<h1>" );
  doc += i18n( "The requested operation could not be completed" );
  doc += QLatin1String( "</h1><h2>" );
  doc += errorName;
  doc += QLatin1String( "</h2>" );
  if ( !techName.isNull() ) {
    doc += QLatin1String( "<h2>" );
    doc += i18n( "Technical Reason: " );
    doc += techName;
    doc += QLatin1String( "</h2>" );
  }
  doc += QLatin1String( "<h3>" );
  doc += i18n( "Details of the Request:" );
  doc += QLatin1String( "</h3><ul><li>" );
  doc += i18n( "URL: %1" ,  url );
  doc += QLatin1String( "</li><li>" );
  if ( !protocol.isNull() ) {
    doc += i18n( "Protocol: %1", protocol );
    doc += QLatin1String( "</li><li>" );
  }
  doc += i18n( "Date and Time: %1" ,  datetime );
  doc += QLatin1String( "</li><li>" );
  doc += i18n( "Additional Information: %1" ,  text );
  doc += QLatin1String( "</li></ul><h3>" );
  doc += i18n( "Description:" );
  doc += QLatin1String( "</h3><p>" );
  doc += description;
  doc += QLatin1String( "</p>" );
  if ( causes.count() ) {
    doc += QLatin1String( "<h3>" );
    doc += i18n( "Possible Causes:" );
    doc += QLatin1String( "</h3><ul><li>" );
    doc += causes.join( "</li><li>" );
    doc += QLatin1String( "</li></ul>" );
  }
  if ( solutions.count() ) {
    doc += QLatin1String( "<h3>" );
    doc += i18n( "Possible Solutions:" );
    doc += QLatin1String( "</h3><ul><li>" );
    doc += solutions.join( "</li><li>" );
    doc += QLatin1String( "</li></ul>" );
  }

  html.replace( QLatin1String("TEXT"), doc );

  return html;
}

// Converts QNetworkReply::Error to KIO::Error...
// NOTE: This probably needs to be moved somewhere more convenient
// in the future. Perhaps KIO::AccessManager itself ???
static int convertErrorCode(int code)
{
  switch (code)
  {
    case QNetworkReply::NoError:
      return 0;
    case QNetworkReply::ConnectionRefusedError:
      return KIO::ERR_COULD_NOT_CONNECT;
    case QNetworkReply::HostNotFoundError:
      return KIO::ERR_UNKNOWN_HOST;
    case QNetworkReply::TimeoutError:
      return KIO::ERR_SERVER_TIMEOUT;
    case QNetworkReply::OperationCanceledError:
      return KIO::ERR_USER_CANCELED;
    case QNetworkReply::ProxyNotFoundError:
      return KIO::ERR_UNKNOWN_PROXY_HOST;
    case QNetworkReply::ContentAccessDenied:
      return KIO::ERR_ACCESS_DENIED;
    case QNetworkReply::ContentOperationNotPermittedError:
      return KIO::ERR_WRITE_ACCESS_DENIED;
    case QNetworkReply::ContentNotFoundError:
      return KIO::ERR_NO_CONTENT;
    case QNetworkReply::AuthenticationRequiredError:
      return KIO::ERR_COULD_NOT_AUTHENTICATE;
    case QNetworkReply::ProtocolUnknownError:
      return KIO::ERR_UNSUPPORTED_PROTOCOL;
    case QNetworkReply::ProtocolInvalidOperationError:
      return KIO::ERR_UNSUPPORTED_ACTION;
    default:
      return KIO::ERR_UNKNOWN;
  }
}

class SslInfo : public WebSslInfo
{
  friend class WebKitPart;
};

class WebKitPart::WebKitPartPrivate
{
public:
  enum PageSecurity { Unencrypted, Encrypted, Mixed };
  WebKitPartPrivate() {}

  KUrl workingURL;
  WebView *webView;
  SslInfo sslInfo;
  WebKitBrowserExtension *browserExtension;
};

WebKitPart::WebKitPart(QWidget *parentWidget, QObject *parent, const QStringList &/*args*/)
        : KParts::ReadOnlyPart(parent), d(new WebKitPart::WebKitPartPrivate())
{
    KAboutData about = KAboutData("webkitpart", "webkitkde", ki18n("WebKit HTML Component"),
                                  /*version*/ "0.2", /*ki18n("shortDescription")*/ KLocalizedString(),
                                  KAboutData::License_LGPL,
                                  ki18n("(c) 2009 Dawit Alemayehu\n"
                                        "(c) 2008-2009 Urs Wolfer\n"
                                        "(c) 2007 Trolltech ASA"));

    about.addAuthor(ki18n("Laurent Montel"), KLocalizedString(), "montel@kde.org");
    about.addAuthor(ki18n("Michael Howell"), KLocalizedString(), "mhowell123@gmail.com");
    about.addAuthor(ki18n("Urs Wolfer"), KLocalizedString(), "uwolfer@kde.org");
    about.addAuthor(ki18n("Dirk Mueller"), KLocalizedString(), "mueller@kde.org");
    about.addAuthor(ki18n("Dawit Alemayehu"), KLocalizedString(), "adawit@kde.org");
    KComponentData componentData(&about);
    setComponentData(componentData);

    // NOTE: If the application does not set its version number, we automatically
    // set it to KDE's version number so that the default user-agent string contains
    // proper version number information.
    if (QCoreApplication::applicationVersion().isEmpty())
      QCoreApplication::setApplicationVersion(QString("%1.%2.%3")
                                              .arg(KDE::versionMajor())
                                              .arg(KDE::versionMinor())
                                              .arg(KDE::versionRelease()));

    setWidget(new QWidget(parentWidget));
    QVBoxLayout* lay = new QVBoxLayout(widget());
    lay->setMargin(0);
    lay->setSpacing(0);
    d->webView = new WebView(this, widget());
    lay->addWidget(d->webView);
    lay->addWidget(d->webView->searchBar());

    connect(d->webView, SIGNAL(loadStarted()),
            this, SLOT(loadStarted()));
    connect(d->webView, SIGNAL(loadFinished(bool)),
            this, SLOT(loadFinished()));
    connect(d->webView, SIGNAL(titleChanged(const QString &)),
            this, SIGNAL(setWindowCaption(const QString &)));
    connect(d->webView, SIGNAL(urlChanged(const QUrl &)),
            this, SLOT(urlChanged(const QUrl &)));


    KWebPage* webPage = d->webView->page();

    connect(webPage, SIGNAL(linkHovered(const QString &, const QString &, const QString &)),
            this, SIGNAL(setStatusBarText(const QString &)));
    connect(webPage->networkAccessManager(), SIGNAL(finished(QNetworkReply*)),
            SLOT(requestFinished(QNetworkReply*)));

    d->browserExtension = new WebKitBrowserExtension(this);

    connect(webPage, SIGNAL(loadProgress(int)),
            d->browserExtension, SIGNAL(loadingProgress(int)));

    initAction();

    setXMLFile("webkitpart.rc");
}

WebKitPart::~WebKitPart()
{
    delete d;
}

void WebKitPart::initAction()
{
    KAction *action = actionCollection()->addAction(KStandardAction::SaveAs, "saveDocument",
                                                    d->browserExtension, SLOT(slotSaveDocument()));

    action = new KAction(i18n("Save &Frame As..."), this);
    actionCollection()->addAction("saveFrame", action);
    connect(action, SIGNAL(triggered(bool)), d->browserExtension, SLOT(slotSaveFrame()));

    action = new KAction(KIcon("document-print-frame"), i18n("Print Frame..."), this);
    actionCollection()->addAction("printFrame", action);
    connect(action, SIGNAL(triggered(bool)), d->browserExtension, SLOT(printFrame()));

    action = new KAction(KIcon("zoom-in"), i18n("Zoom In"), this);
    actionCollection()->addAction("zoomIn", action);
    action->setShortcut(KShortcut("CTRL++; CTRL+="));
    connect(action, SIGNAL(triggered(bool)), d->browserExtension, SLOT(zoomIn()));

    action = new KAction(KIcon("zoom-out"), i18n("Zoom Out"), this);
    actionCollection()->addAction("zoomOut", action);
    action->setShortcut(KShortcut("CTRL+-; CTRL+_"));
    connect(action, SIGNAL(triggered(bool)), d->browserExtension, SLOT(zoomOut()));

    action = new KAction(KIcon("zoom-original"), i18n("Actual Size"), this);
    actionCollection()->addAction("zoomNormal", action);
    action->setShortcut(KShortcut("CTRL+0"));
    connect(action, SIGNAL(triggered(bool)), d->browserExtension, SLOT(zoomNormal()));
#if QT_VERSION >= 0x040500
    action = new KAction(i18n("Zoom Text Only"), this);
    action->setCheckable(true);
    KConfigGroup cgHtml(KGlobal::config(), "HTML Settings");
    bool zoomTextOnly = cgHtml.readEntry("ZoomTextOnly", false);
    action->setChecked(zoomTextOnly);
    actionCollection()->addAction("zoomTextOnly", action);
    connect(action, SIGNAL(triggered(bool)), d->browserExtension, SLOT(toogleZoomTextOnly()));
#endif
    action = actionCollection()->addAction(KStandardAction::SelectAll, "selectAll",
                                           d->browserExtension, SLOT(slotSelectAll()));
    action->setShortcutContext(Qt::WidgetShortcut);
    d->webView->addAction(action);

    action = new KAction(i18n("View Do&cument Source"), this);
    actionCollection()->addAction("viewDocumentSource", action);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_U));
    connect(action, SIGNAL(triggered(bool)), d->browserExtension, SLOT(slotViewDocumentSource()));

    action = new KAction(i18n("SSL"), this);
    actionCollection()->addAction("security", action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(showSecurity()));

    action = actionCollection()->addAction(KStandardAction::Find, "find", d->webView->searchBar(), SLOT(show()));
    action->setWhatsThis(i18n("Find text<br /><br />"
                              "Shows a dialog that allows you to find text on the displayed page."));
}

void WebKitPart::guiActivateEvent(KParts::GUIActivateEvent *event)
{
    Q_UNUSED(event);
    // just overwrite, but do nothing for the moment
}

bool WebKitPart::openUrl(const KUrl &url)
{
    kDebug() << url;

    if ( url.protocol() == "error" && url.hasSubUrl() )
    {
      closeUrl();

      /**
       * The format of the error url is that two variables are passed in the query:
       * error = int kio error code, errText = QString error text from kio
       * and the URL where the error happened is passed as a sub URL.
       */
      KUrl::List urls = KUrl::split( url );
      //kDebug() << "Handling error URL. URL count:" << urls.count();

      if ( urls.count() > 1 ) {
        KUrl mainURL = urls.first();
        int error = mainURL.queryItem( "error" ).toInt();
        // error=0 isn't a valid error code, so 0 means it's missing from the URL
        if ( error == 0 ) error = KIO::ERR_UNKNOWN;
        QString errorText = mainURL.queryItem( "errText" );
        urls.pop_front();
        KUrl reqUrl = KUrl::join( urls );

        kDebug() << "Setting Url back to => " << reqUrl;
        emit d->browserExtension->setLocationBarUrl(reqUrl.prettyUrl());
        setUrl(reqUrl);

        emit started(0);
        showError(htmlError(error, errorText, reqUrl));
        emit completed();

        return true;
      }
    }

    KParts::OpenUrlArguments args (arguments());

    KIO::MetaData metaData (args.metaData());
    setSslInfo(metaData.toVariant(), url);
    args.metaData().insert("ssl_was_in_use", (d->sslInfo.isValid() ? "TRUE" : "FALSE"));

    setUrl(url); //We can't wait that urlChanged is calling otherwise some plugins as babelfish can't be enabled
    d->webView->loadUrl(url, args, browserExtension()->browserArguments());
    return true;
}

bool WebKitPart::closeUrl()
{
    d->webView->stop();
    return true;
}

WebKitBrowserExtension *WebKitPart::browserExtension() const
{
    return d->browserExtension;
}

bool WebKitPart::openFile()
{
    // never reached
    return false;
}

void WebKitPart::loadStarted()
{
    // Make sure the visual SSL indicator does not get rendered when we
    // navigate away from the current page.
    d->browserExtension->setPageSecurity(WebKitPart::WebKitPartPrivate::Unencrypted);
    emit started(0);
}

void WebKitPart::loadFinished()
{
    emit completed();
}

void WebKitPart::requestFinished(QNetworkReply* reply)
{
    KUrl currentUrl(url());
    KUrl requestUrl(reply->request().url());

    kDebug() << "current url: " << currentUrl.url();
    kDebug() << "request url: " << requestUrl.url();

    if (urlcmp(currentUrl.url(), requestUrl.url(), KUrl::CompareWithoutTrailingSlash)) {
        QVariant metaData = reply->attribute(QNetworkRequest::User);

        //  Set the SSL information from the meta data sent by the ioslave...
        setSslInfo(metaData, reply->request().url());

        // Enable/disable the visual SSL indicator...
        int pageSecurity;
        if (d->sslInfo.isValid())
            pageSecurity = WebKitPart::WebKitPartPrivate::Encrypted;
        else
            pageSecurity = WebKitPart::WebKitPartPrivate::Unencrypted;

        d->browserExtension->setPageSecurity(pageSecurity);

        // Handle any error messages...
        if (reply->error() != QNetworkReply::NoError) {
            emit canceled(reply->errorString());
            showError(htmlError(convertErrorCode(reply->error()),
                                reply->errorString(), reply->url()));
        }
    }
}



void WebKitPart::urlChanged(const QUrl &url)
{
    const QList<QWebHistoryItem> backItemsList = view()->history()->backItems(2);
#ifndef NDEBUG
    if (backItemsList.count() > 0) {
        kDebug() << backItemsList.at(0).url() << url;
    }
#endif

    if (!(backItemsList.count() > 0 && backItemsList.at(0).url() == url)) {
        emit d->browserExtension->openUrlNotify();
    }

    // If the current host and the requested host are not the same, then
    // invalidate any stored SSL information...
    kDebug() << "current host: " << this->url().host() << ", requested host: " << url.host();

    if (this->url().isValid() && this->url().host() != url.host()) {
      kDebug() << "Reseting SSL information";
      d->sslInfo.reset();
    }

    kDebug() << url;
    setUrl(url);
    emit d->browserExtension->setLocationBarUrl(KUrl(url).prettyUrl());
}

void WebKitPart::showSecurity()
{
    if (d->sslInfo.isValid()) {
#ifdef HAS_SSL_INFO_DIALOG
        KSslInfoDialog *kid = new KSslInfoDialog(0);
        kid->setSslInfo(d->sslInfo.certificateChain(),
                        d->sslInfo.peerAddress().toString(),
                        d->sslInfo.url().host(),
                        d->sslInfo.protocol(),
                        d->sslInfo.ciphers(),
                        d->sslInfo.usedChiperBits(),
                        d->sslInfo.supportedChiperBits(),
                        KSslInfoDialog::errorsFromString(d->sslInfo.certificateErrors()));
        kid->exec();
#else
        kDebug() << "Host: " << d->sslInfo.url().host();
        kDebug() << "Peer address: " << d->sslInfo.peerAddress().toString();
        kDebug() << "Protocol: " << d->sslInfo.protocol();
        kDebug() << "Cipher: " << d->sslInfo.ciphers().split("\n").join(",");
        kDebug() << "Bits Used: " << d->sslInfo.usedChiperBits();
        kDebug() << "Bits Available: " << d->sslInfo.supportedChiperBits();
#endif
    } else {
        KMessageBox::information(0, i18n("The peer SSL certificate chain "
                                         "appears to be corrupt."),
                                 i18n("SSL"));
    }
}

WebView * WebKitPart::view()
{
    return d->webView;
}

void WebKitPart::setStatusBarTextProxy(const QString &message)
{
    emit setStatusBarText(message);
}

void WebKitPart::showError(const QString& html)
{
    const bool signalsBlocked = d->webView->blockSignals(true);
    d->webView->setHtml(html);
    d->webView->blockSignals(signalsBlocked);
}

void WebKitPart::setSslInfo(const QVariant& metaData, const QUrl& url)
{
  if (metaData.isValid() && metaData.type() == QVariant::Map) {
    QMap<QString,QVariant> metaDataMap = metaData.toMap();
    if (metaDataMap.value("ssl_in_use").toBool()) {
      d->sslInfo.setUrl(url);
      d->sslInfo.setCertificateChain(metaDataMap.value("ssl_peer_chain").toByteArray());
      d->sslInfo.setPeerAddress(metaDataMap.value("ssl_peer_ip").toString());
      d->sslInfo.setParentAddress(metaDataMap.value("ssl_parent_ip").toString());
      d->sslInfo.setProtocol(metaDataMap.value("ssl_protocol_version").toString());
      d->sslInfo.setCiphers(metaDataMap.value("ssl_cipher").toString());
      d->sslInfo.setCertificateErrors(metaDataMap.value("ssl_cert_errors").toString());
      d->sslInfo.setUsedCipherBits(metaDataMap.value("ssl_cipher_used_bits").toString());
      d->sslInfo.setSupportedCipherBits(metaDataMap.value("ssl_cipher_bits").toString());
    }
  }
}


WebKitBrowserExtension::WebKitBrowserExtension(WebKitPart *parent)
        : KParts::BrowserExtension(parent), part(parent)
{
    connect(part->view()->page(), SIGNAL(selectionChanged()),
            this, SLOT(updateEditActions()));
    connect(part->view(), SIGNAL(openUrl(const KUrl &)), this, SIGNAL(openUrlRequest(const KUrl &)));
    connect(part->view(), SIGNAL(openUrlInNewTab(const KUrl &)), this, SIGNAL(createNewWindow(const KUrl &)));

    enableAction("cut", false);
    enableAction("copy", false);
    enableAction("paste", false);
    enableAction("print", true);
}

void WebKitBrowserExtension::cut()
{
    part->view()->page()->triggerAction(KWebPage::Cut);
}

void WebKitBrowserExtension::copy()
{
    part->view()->page()->triggerAction(KWebPage::Copy);
}

void WebKitBrowserExtension::paste()
{
    part->view()->page()->triggerAction(KWebPage::Paste);
}

void WebKitBrowserExtension::slotSaveDocument()
{
    qobject_cast<WebPage*>(part->view()->page())->saveUrl(part->view()->url());
}

void WebKitBrowserExtension::slotSaveFrame()
{
    qobject_cast<WebPage*>(part->view()->page())->saveUrl(part->view()->page()->currentFrame()->url());
}

void WebKitBrowserExtension::print()
{
    QPrintPreviewDialog dlg(part->view());
    connect(&dlg, SIGNAL(paintRequested(QPrinter *)),
            part->view(), SLOT(print(QPrinter *)));
    dlg.exec();
}

void WebKitBrowserExtension::printFrame()
{
    QPrintPreviewDialog dlg(part->view());
    connect(&dlg, SIGNAL(paintRequested(QPrinter *)),
            part->view()->page()->currentFrame(), SLOT(print(QPrinter *)));
    dlg.exec();
}

void WebKitBrowserExtension::updateEditActions()
{
    KWebPage *page = part->view()->page();
    enableAction("cut", page->action(KWebPage::Cut));
    enableAction("copy", page->action(KWebPage::Copy));
    enableAction("paste", page->action(KWebPage::Paste));
}

void WebKitBrowserExtension::searchProvider()
{
    // action name is of form "previewProvider[<searchproviderprefix>:]"
    const QString searchProviderPrefix = QString(sender()->objectName()).mid(14);

    const QString text = part->view()->page()->selectedText();
    KUriFilterData data;
    QStringList list;
    data.setData(searchProviderPrefix + text);
    list << "kurisearchfilter" << "kuriikwsfilter";

    if (!KUriFilter::self()->filterUri(data, list)) {
        KDesktopFile file("services", "searchproviders/google.desktop");
        const QString encodedSearchTerm = QUrl::toPercentEncoding(text);
        KConfigGroup cg(file.desktopGroup());
        data.setData(cg.readEntry("Query").replace("\\{@}", encodedSearchTerm));
    }

    KParts::BrowserArguments browserArgs;
    browserArgs.frameName = "_blank";

    emit openUrlRequest(data.uri(), KParts::OpenUrlArguments(), browserArgs);
}

void WebKitBrowserExtension::reparseConfiguration()
{
  // Force the configuration stuff to repase...
  WebKitSettings::self()->init();
}

void WebKitBrowserExtension::zoomIn()
{
#if QT_VERSION >= 0x040500
    part->view()->setZoomFactor(part->view()->zoomFactor() + 0.1);
#else
    part->view()->setTextSizeMultiplier(part->view()->textSizeMultiplier() + 0.1);
#endif
}

void WebKitBrowserExtension::zoomOut()
{
#if QT_VERSION >= 0x040500
    part->view()->setZoomFactor(part->view()->zoomFactor() - 0.1);
#else
    part->view()->setTextSizeMultiplier(part->view()->textSizeMultiplier() - 0.1);
#endif
}

void WebKitBrowserExtension::zoomNormal()
{
#if QT_VERSION >= 0x040500
    part->view()->setZoomFactor(1);
#else
    part->view()->setTextSizeMultiplier(1);
#endif
}

void WebKitBrowserExtension::toogleZoomTextOnly()
{
#if QT_VERSION >= 0x040500
    KConfigGroup cgHtml(KGlobal::config(), "HTML Settings");
    bool zoomTextOnly = cgHtml.readEntry( "ZoomTextOnly", false );
    cgHtml.writeEntry("ZoomTextOnly", !zoomTextOnly);
    KGlobal::config()->reparseConfiguration();

    part->view()->settings()->setAttribute(QWebSettings::ZoomTextOnly, !zoomTextOnly);
#endif
}

void WebKitBrowserExtension::slotSelectAll()
{
#if QT_VERSION >= 0x040500
    part->view()->page()->triggerAction(KWebPage::SelectAll);
#endif
}

void WebKitBrowserExtension::slotFrameInWindow()
{
    KParts::OpenUrlArguments args;// = d->m_khtml->arguments();
    args.metaData()["referrer"] = part->view()->contextMenuResult().linkText();
    args.metaData()["forcenewwindow"] = "true";
    emit createNewWindow(part->view()->contextMenuResult().linkUrl(), args);
}

void WebKitBrowserExtension::slotFrameInTab()
{
    KParts::OpenUrlArguments args;// = d->m_khtml->arguments();
    args.metaData()["referrer"] = part->view()->contextMenuResult().linkText();
    KParts::BrowserArguments browserArgs;//( d->m_khtml->browserExtension()->browserArguments() );
    browserArgs.setNewTab(true);
    emit createNewWindow(part->view()->contextMenuResult().linkUrl(), args, browserArgs);
}

void WebKitBrowserExtension::slotFrameInTop()
{
    KParts::OpenUrlArguments args;// = d->m_khtml->arguments();
    args.metaData()["referrer"] = part->view()->contextMenuResult().linkText();
    KParts::BrowserArguments browserArgs;//( d->m_khtml->browserExtension()->browserArguments() );
    browserArgs.frameName = "_top";
    emit openUrlRequest(part->view()->contextMenuResult().linkUrl(), args, browserArgs);
}

void WebKitBrowserExtension::slotSaveImageAs()
{
    QList<KUrl> urls;
    urls.append(part->view()->contextMenuResult().imageUrl());
    const int nbUrls = urls.count();
    for (int i = 0; i != nbUrls; i++) {
        QString file = KFileDialog::getSaveFileName(KUrl(), QString(), part->widget());
        KIO::NetAccess::file_copy(urls.at(i), file, part->widget());
    }
}

void WebKitBrowserExtension::slotSendImage()
{
    QStringList urls;
    urls.append(part->view()->contextMenuResult().imageUrl().path());
    const QString subject = part->view()->contextMenuResult().imageUrl().path();
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
    mimeData->setImageData(part->view()->contextMenuResult().pixmap());
    safeURL.populateMimeData(mimeData);
    QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);

    mimeData = new QMimeData;
    mimeData->setImageData(part->view()->contextMenuResult().pixmap());
    safeURL.populateMimeData(mimeData);
    QApplication::clipboard()->setMimeData(mimeData, QClipboard::Selection);
}

void WebKitBrowserExtension::slotCopyLinkLocation()
{
    KUrl safeURL(part->view()->contextMenuResult().linkUrl());
    safeURL.setPass(QString());
    // Set it in both the mouse selection and in the clipboard
    QMimeData* mimeData = new QMimeData;
    safeURL.populateMimeData(mimeData);
    QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);

    mimeData = new QMimeData;
    safeURL.populateMimeData(mimeData);
    QApplication::clipboard()->setMimeData(mimeData, QClipboard::Selection);
}

void WebKitBrowserExtension::slotSaveLinkAs()
{
    qobject_cast<WebPage*>(part->view()->page())->saveUrl(part->view()->contextMenuResult().linkUrl());
}

void WebKitBrowserExtension::slotViewDocumentSource()
{
    //TODO test http requests
    KUrl currentUrl(part->view()->page()->mainFrame()->url());
    bool isTempFile = false;
#if 0
    if (!(currentUrl.isLocalFile())/* && KHTMLPageCache::self()->isComplete(d->m_cacheId)*/) { //TODO implement
        KTemporaryFile sourceFile;
//         sourceFile.setSuffix(defaultExtension());
        sourceFile.setAutoRemove(false);
        if (sourceFile.open()) {
//             QDataStream stream (&sourceFile);
//             KHTMLPageCache::self()->saveData(d->m_cacheId, &stream);
            currentUrl = KUrl();
            currentUrl.setPath(sourceFile.fileName());
            isTempFile = true;
        }
    }
#endif

    KRun::runUrl(currentUrl, QLatin1String("text/plain"), part->view(), isTempFile);
}

#include "webkitpart.moc"
