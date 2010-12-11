/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2008 - 2010 Urs Wolfer <uwolfer @ kde.org>
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

#include "webpage.h"

#include "fakepage.h"
#include "kwebkitpart.h"
#include "websslinfo.h"
#include "webview.h"
#include "sslinfodialog_p.h"
#include "networkaccessmanager.h"
#include "settings/webkitsettings.h"

#include <KDE/KMessageBox>
#include <KDE/KGlobalSettings>
#include <KDE/KGlobal>
#include <KDE/KLocale>
#include <KDE/KRun>
#include <KDE/KShell>
#include <KDE/KStandardDirs>
#include <KDE/KAuthorized>
#include <KDE/KDebug>
#include <KIO/Job>
#include <KIO/AccessManager>

#include <QtCore/QFile>
#include <QtGui/QApplication>
#include <QtGui/QTextDocument> // Qt::escape
#include <QtNetwork/QNetworkReply>

#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebElement>
#include <QtWebKit/QWebHistory>
#include <QtWebKit/QWebHistoryItem>
#include <QtWebKit/QWebSecurityOrigin>

#define QL1S(x)  QLatin1String(x)
#define QL1C(x)  QLatin1Char(x)

typedef QPair<QString, QString> StringPair;


// Sanitizes the "mailto:" url, e.g. strips out any "attach" parameters.
static QUrl sanitizeMailToUrl(const QUrl &url, QStringList& files) {
    QUrl sanitizedUrl;

    // NOTE: This is necessary to ensure we can properly use QUrl's query
    // related APIs to process 'mailto:' urls of form 'mailto:foo@bar.com'.
    if (url.hasQuery())
      sanitizedUrl = url;
    else
      sanitizedUrl = QUrl(url.scheme() + QL1S(":?") + url.path());

    QListIterator<StringPair> it (sanitizedUrl.queryItems());
    sanitizedUrl.setEncodedQuery(QByteArray());    // clear out the query componenet

    while (it.hasNext()) {
        StringPair queryItem = it.next();
        if (queryItem.first.contains(QL1C('@')) && queryItem.second.isEmpty()) {
            queryItem.second = queryItem.first;
            queryItem.first = "to";
        } else if (QString::compare(queryItem.first, QL1S("attach"), Qt::CaseInsensitive) == 0) {
            files << queryItem.second;
            continue;
        }
        sanitizedUrl.addQueryItem(queryItem.first, queryItem.second);
    }

    return sanitizedUrl;
}

static int errorCodeFromReply(QNetworkReply* reply)
{
    // First check if there is a KIO error code sent back and use that,
    // if not attempt to convert QNetworkReply's NetworkError to KIO::Error.
    QVariant attr = reply->attribute(static_cast<QNetworkRequest::Attribute>(KIO::AccessManager::KioError));
    if (attr.isValid() && attr.type() == QVariant::Int)
        return attr.toInt();

    switch (reply->error()) {
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
        case QNetworkReply::UnknownNetworkError:
            return KIO::ERR_UNKNOWN;
        case QNetworkReply::NoError:
        default:
            break;
    }

    return 0;
}

// Returns true if the scheme and domain of the two urls match...
static bool domainSchemeMatch(const QUrl& u1, const QUrl& u2)
{
    if (u1.scheme() != u2.scheme())
        return false;

    QStringList u1List = u1.host().split(QL1C('.'), QString::SkipEmptyParts);
    QStringList u2List = u2.host().split(QL1C('.'), QString::SkipEmptyParts);

    if (qMin(u1List.count(), u2List.count()) < 2)
        return false;  // better safe than sorry...

    while (u1List.count() > 2)
        u1List.removeFirst();

    while (u2List.count() > 2)
        u2List.removeFirst();

    return (u1List == u2List);
}

class WebPage::WebPagePrivate
{
public:
    WebPagePrivate()
      :kioErrorCode(0),
       ignoreError(false),      
       userRequestedCreateWindow(false),
       ignoreHistoryNavigationRequest(true) {}

    enum WebPageSecurity { PageUnencrypted, PageEncrypted, PageMixed };

    WebSslInfo sslInfo;
    QVector<QUrl> requestQueue;
    QPointer<KWebKitPart> part;

    int kioErrorCode;    
    bool ignoreError;
    bool userRequestedCreateWindow;
    bool ignoreHistoryNavigationRequest;
};

WebPage::WebPage(KWebKitPart *part, QWidget *parent)
        :KWebPage(parent, (KWebPage::KPartsIntegration|KWebPage::KWalletIntegration)),
         d (new WebPagePrivate)
{
    d->part = part;

    // FIXME: Need a better way to handle request filtering than to inherit
    // KIO::Integration::AccessManager...
    KDEPrivate::MyNetworkAccessManager *manager = new KDEPrivate::MyNetworkAccessManager(this);
    if (parent && parent->window())
        manager->setCookieJarWindowId(parent->window()->winId());
    setNetworkAccessManager(manager);

    setSessionMetaData(QL1S("ssl_activate_warnings"), QL1S("TRUE"));

    // Set font sizes accordingly...
    if (view())
        WebKitSettings::self()->computeFontSizes(view()->logicalDpiY());

    setForwardUnsupportedContent(true);

    // Tell QWebSecurityOrigin that man:/ and info:/ are local resources...
    QWebSecurityOrigin::addLocalScheme(QL1S("man"));
    QWebSecurityOrigin::addLocalScheme(QL1S("info"));

    connect(this, SIGNAL(downloadRequested(const QNetworkRequest &)),
            this, SLOT(downloadRequest(const QNetworkRequest &)));
    connect(this, SIGNAL(unsupportedContent(QNetworkReply *)),
            this, SLOT(slotUnsupportedContent(QNetworkReply *)));
    connect(networkAccessManager(), SIGNAL(finished(QNetworkReply *)),
            this, SLOT(slotRequestFinished(QNetworkReply *)));
}

WebPage::~WebPage()
{
    delete d;
}

const WebSslInfo& WebPage::sslInfo() const
{
    return d->sslInfo;
}

void WebPage::setSslInfo (const WebSslInfo& info)
{
    d->sslInfo = info;
}

bool WebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type)
{
    QUrl reqUrl (request.url());
    const bool isMainFrameRequest = (frame == mainFrame());
    /*
      NOTE: We use a dynamic QObject property called "NavigationTypeUrlEntered"
      to distinguish between requests generated by user entering a url vs those
      that were generated programatically through javascript.
    */
    const bool isTypedUrl = property("NavigationTypeUrlEntered").toBool();

    // Handle "mailto:" url here...
    if (handleMailToUrl(reqUrl, type))
      return false;

    if (isMainFrameRequest && isTypedUrl)
      setProperty("NavigationTypeUrlEntered", QVariant());

    if (frame) {
        // inPage requests are those generarted within the current page through
        // link clicks, javascript queries, and button clicks (form submission).
        bool inPageRequest = true;

        switch (type) {
        case QWebPage::NavigationTypeFormSubmitted:
        case QWebPage::NavigationTypeFormResubmitted:
            if (!checkFormData(request))
                return false;
            break;
        case QWebPage::NavigationTypeBackOrForward:
            // NOTE: This is necessary because restoring QtWebKit's history causes
            // it to navigate to the last item. Unfortunately that causes
            if (d->ignoreHistoryNavigationRequest) {
                d->ignoreHistoryNavigationRequest = false;
                //kDebug() << "Rejected history navigation to" << history()->currentItem().url();
                return false;
            }
            /*
            kDebug() << "Navigating to item (" << history()->currentItemIndex()
                << "of" << history()->count() << "):" << history()->currentItem().url();
            */
            inPageRequest = false;
            break;
        case QWebPage::NavigationTypeReload:
            inPageRequest = false;
            break;
        case QWebPage::NavigationTypeOther:
            inPageRequest = !isTypedUrl;
            if (d->ignoreHistoryNavigationRequest)
                d->ignoreHistoryNavigationRequest = false;
            break;
        default:
            break;
        }

        if (inPageRequest) {
            if (!checkLinkSecurity(request, type))
                return false;

            if (d->sslInfo.isValid())
                setRequestMetaData(QL1S("ssl_was_in_use"), QL1S("TRUE"));
        }

        if (isMainFrameRequest) {
            setRequestMetaData(QL1S("main_frame_request"), QL1S("TRUE"));
            if (d->sslInfo.isValid() && !domainSchemeMatch(request.url(), d->sslInfo.url()))
                d->sslInfo = WebSslInfo();
        } else {
            setRequestMetaData(QL1S("main_frame_request"), QL1S("FALSE"));
        }

        // Insert the request into the queue...
        reqUrl.setUserInfo(QString());
        d->requestQueue << reqUrl;
    }

    return KWebPage::acceptNavigationRequest(frame, request, type);
}

QWebPage *WebPage::createWindow(WebWindowType type)
{
    KParts::OpenUrlArguments uargs;
    KParts::BrowserArguments bargs;
    KParts::WindowArgs wargs;

    // Mark this action as one generated by javascript...
    uargs.setActionRequestedByUser(false);

    switch (type) {
#if QTWEBKIT_VERSION >= QTWEBKIT_VERSION_CHECK(2, 2, 0)
    case WebDialog:
#endif      
    case WebModalDialog:
        bargs.setForcesNewWindow(true);
        break;
    case WebBrowserWindow:
    default:
        bargs.setForcesNewWindow(false);      
        break;
    }
    
    KParts::ReadOnlyPart *part = 0;
    d->part->browserExtension()->createNewWindow(KUrl(), uargs, bargs, KParts::WindowArgs(), &part);
    
    KWebKitPart* webkitpart = qobject_cast<KWebKitPart*>(part);
    if (webkitpart) {
        // If type is a dialog, make sure the scroll bars are also off by default...
        switch (type) {
        case WebModalDialog: {
            // TODO: Investigate whether we really want to allow modal dialogs!!
            // See the "modal" section @ https://developer.mozilla.org/en/DOM/window.open
            QWidget* mainWidget = part->widget() ? part->widget()->window() : 0;
            if (mainWidget)
                mainWidget->setWindowModality(Qt::ApplicationModal);
        }
        case WebBrowserWindow:
        default:
            break;
        }        
        return webkitpart->view()->page();
    }

    /*
     The workaround below is intended to address the situation where the returned
     KPart is NOT a KWebKitPart. Since we cannot return an instance of QWebPage in
     those instances, we simply create a fake QWebPage to intercept the request url
     and call the new part's openUrl function. See BR# 253708 and BR# 258367.
    */
    if (part)
       return new FakePage(part, type, this);
    
    return 0;
}

void WebPage::slotUnsupportedContent(QNetworkReply *reply)
{
    Q_ASSERT (reply);

    const KIO::MetaData metaData = reply->attribute(static_cast<QNetworkRequest::Attribute>(KIO::AccessManager::MetaData)).toMap();
    bool hasContentDisposition;
    if (metaData.isEmpty())
        hasContentDisposition = reply->hasRawHeader("Content-Disposition");
    else
        hasContentDisposition = metaData.contains(QL1S("content-disposition-filename"));

    if (hasContentDisposition) {
        downloadResponse(reply);
        return;
    }

    if (reply->request().originatingObject() == this->mainFrame()) {
        reply->abort();
        KParts::OpenUrlArguments args;
        QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
        if (!contentType.isEmpty()) {
            if (contentType.contains(QL1C(';')))
                contentType.truncate(contentType.indexOf(QL1C(';')));
            args.setMimeType(contentType);
        }
        emit d->part->browserExtension()->openUrlRequest(reply->url(), args, KParts::BrowserArguments());
    }
}

void WebPage::downloadRequest(const QNetworkRequest &request)
{
    const KUrl url(request.url());

    // Integration with a download manager...
    if (!url.isLocalFile()) {
        KConfigGroup cfg = KSharedConfig::openConfig("konquerorrc", KConfig::NoGlobals)->group("HTML Settings");
        const QString downloadManger = cfg.readPathEntry("DownloadManager", QString());

        if (!downloadManger.isEmpty()) {
            // then find the download manager location
            //kDebug() << "Using: " << downloadManger << " as Download Manager";
            QString cmd = KStandardDirs::findExe(downloadManger);
            if (cmd.isEmpty()) {
                QString errMsg = i18n("The download manager (%1) could not be found in your installation.", downloadManger);
                QString errMsgEx = i18n("Try to reinstall it and make sure that it is available in $PATH. \n\nThe integration will be disabled.");
                KMessageBox::detailedSorry(view(), errMsg, errMsgEx);
                cfg.writePathEntry("DownloadManager", QString());
                cfg.sync();
            } else {
                cmd += ' ' + KShell::quoteArg(url.url());
                //kDebug() << "Calling command" << cmd;
                KRun::runCommand(cmd, view());
                return;
            }
        }
    }

    KWebPage::downloadRequest(request);
}

void WebPage::slotRequestFinished(QNetworkReply *reply)
{
    Q_ASSERT(reply);
    
    QUrl url (reply->request().url());
    const int index = d->requestQueue.indexOf(url);
    if (index == -1)
        return;

    d->requestQueue.remove(index);
    QWebFrame* frame = qobject_cast<QWebFrame *>(reply->request().originatingObject());
    if (!frame)
        return;
    
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();    
    // Only deal with non-redirect responses...
    if (statusCode > 299 && statusCode < 400) {
        d->sslInfo.restoreFrom(reply->attribute(static_cast<QNetworkRequest::Attribute>(KIO::AccessManager::MetaData)),
                               reply->url());
        return;
    }

    const int errCode = errorCodeFromReply(reply);
    const bool isMainFrameRequest = (frame == mainFrame()); 
    // Handle any error...
    switch (errCode) {
        case 0:
            if (isMainFrameRequest) {
                d->sslInfo.restoreFrom(reply->attribute(static_cast<QNetworkRequest::Attribute>(KIO::AccessManager::MetaData)),
                                        reply->url());
                setPageJScriptPolicy(reply->url());
            }
            break;
        case KIO::ERR_ABORTED:
        case KIO::ERR_USER_CANCELED: // Do nothing if request is cancelled/aborted
            //kDebug() << "User aborted request!";
            d->ignoreError = true;
            emit loadAborted(QUrl());
            return;
        // Handle the user clicking on a link that refers to a directory
        // Since KIO cannot automatically convert a GET request to a LISTDIR one.
        case KIO::ERR_IS_DIRECTORY:
            d->ignoreError = true;
            emit loadAborted(reply->url());
            return;
        default:
            // Make sure the saveFrameStateRequested signal is emitted so
            // the page can restored properly.
            if (isMainFrameRequest)
                emit saveFrameStateRequested(frame, 0);

            d->ignoreError = false;
            d->kioErrorCode = errCode;
            break;
    }

    if (isMainFrameRequest) {
        WebPagePrivate::WebPageSecurity security;
        if (d->sslInfo.isValid())
            security = WebPagePrivate::PageEncrypted;
        else
            security = WebPagePrivate::PageUnencrypted;

        emit d->part->browserExtension()->setPageSecurity(security);
    }
}

bool WebPage::checkLinkSecurity(const QNetworkRequest &req, NavigationType type) const
{
    // Check whether the request is authorized or not...
    if (!KAuthorized::authorizeUrlAction("redirect", mainFrame()->url(), req.url())) {

        //kDebug() << "*** Failed security check: base-url=" << mainFrame()->url() << ", dest-url=" << req.url();
        QString buttonText, title, message;

        int response = KMessageBox::Cancel;
        KUrl linkUrl (req.url());

        if (type == QWebPage::NavigationTypeLinkClicked) {
            message = i18n("<qt>This untrusted page links to<br/><b>%1</b>."
                           "<br/>Do you want to follow the link?</qt>", linkUrl.url());
            title = i18n("Security Warning");
            buttonText = i18nc("follow link despite of security warning", "Follow");
        } else {
            title = i18n("Security Alert");
            message = i18n("<qt>Access by untrusted page to<br/><b>%1</b><br/> denied.</qt>",
                           Qt::escape(linkUrl.prettyUrl()));
        }

        if (buttonText.isEmpty()) {
            KMessageBox::error( 0, message, title);
        } else {
            // Dangerous flag makes the Cancel button the default
            response = KMessageBox::warningContinueCancel(0, message, title,
                                                          KGuiItem(buttonText),
                                                          KStandardGuiItem::cancel(),
                                                          QString(), // no don't ask again info
                                                          KMessageBox::Notify | KMessageBox::Dangerous);
        }

        return (response == KMessageBox::Continue);
    }

    return true;
}

bool WebPage::checkFormData(const QNetworkRequest &req) const
{
    const QString scheme (req.url().scheme());

    if (d->sslInfo.isValid() &&
        !scheme.compare(QL1S("https")) && !scheme.compare(QL1S("mailto")) &&
        (KMessageBox::warningContinueCancel(0,
                                           i18n("Warning: This is a secure form "
                                                "but it is attempting to send "
                                                "your data back unencrypted.\n"
                                                "A third party may be able to "
                                                "intercept and view this "
                                                "information.\nAre you sure you "
                                                "want to send the data unencrypted?"),
                                           i18n("Network Transmission"),
                                           KGuiItem(i18n("&Send Unencrypted")))  == KMessageBox::Cancel)) {

        return false;
    }


    if (scheme.compare(QL1S("mailto")) == 0 &&
        (KMessageBox::warningContinueCancel(0, i18n("This site is attempting to "
                                                    "submit form data via email.\n"
                                                    "Do you want to continue?"),
                                            i18n("Network Transmission"),
                                            KGuiItem(i18n("&Send Email")),
                                            KStandardGuiItem::cancel(),
                                            "WarnTriedEmailSubmit") == KMessageBox::Cancel)) {
        return false;
    }

    return true;
}

bool WebPage::handleMailToUrl (const QUrl &url, NavigationType type) const
{
    if (QString::compare(url.scheme(), QL1S("mailto"), Qt::CaseInsensitive) == 0) {
        QStringList files;
        QUrl mailtoUrl (sanitizeMailToUrl(url, files));

        switch (type) {
            case QWebPage::NavigationTypeLinkClicked:
                if (!files.isEmpty() && KMessageBox::warningContinueCancelList(0,
                                                                               i18n("<qt>Do you want to allow this site to attach "
                                                                                    "the following files to the email message?</qt>"),
                                                                               files, i18n("Email Attachment Confirmation"),
                                                                               KGuiItem(i18n("&Allow attachments")),
                                                                               KGuiItem(i18n("&Ignore attachments")), QL1S("WarnEmailAttachment")) == KMessageBox::Continue) {

                   // Re-add the attachments...
                    QStringListIterator filesIt (files);
                    while (filesIt.hasNext()) {
                        mailtoUrl.addQueryItem(QL1S("attach"), filesIt.next());
                    }
                }
                break;
            case QWebPage::NavigationTypeFormSubmitted:
            case QWebPage::NavigationTypeFormResubmitted:
                if (!files.isEmpty()) {
                    KMessageBox::information(0, i18n("This site attempted to attach a file from your "
                                                     "computer in the form submission. The attachment "
                                                     "was removed for your protection."),
                                             i18n("Attachment Removed"), "InfoTriedAttach");
                }
                break;
            default:
                 break;
        }

        //kDebug() << "Emitting openUrlRequest with " << mailtoUrl;
        emit d->part->browserExtension()->openUrlRequest(mailtoUrl);
        return true;
    }

    return false;
}

void WebPage::setPageJScriptPolicy(const QUrl &url)
{
    const QString hostname (url.host());
    settings()->setAttribute(QWebSettings::JavascriptEnabled,
                             WebKitSettings::self()->isJavaScriptEnabled(hostname));

    const WebKitSettings::KJSWindowOpenPolicy policy = WebKitSettings::self()->windowOpenPolicy(hostname);
    settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows,
                             (policy != WebKitSettings::KJSWindowOpenDeny &&
                              policy != WebKitSettings::KJSWindowOpenSmart));
}

bool WebPage::extension(Extension extension, const ExtensionOption *option, ExtensionReturn *output)
{
    if (extension == QWebPage::ErrorPageExtension && !d->ignoreError)
    {
        const QWebPage::ErrorPageExtensionOption *extOption = static_cast<const QWebPage::ErrorPageExtensionOption*>(option);
        //kDebug() << extOption->domain << extOption->error << extOption->errorString;
        if (extOption->domain == QWebPage::QtNetwork) {
            QWebPage::ErrorPageExtensionReturn *extOutput = static_cast<QWebPage::ErrorPageExtensionReturn*>(output);
            extOutput->content = errorPage(d->kioErrorCode, extOption->errorString, extOption->url).toUtf8();
            extOutput->baseUrl = extOption->url;
            return true;
        }
    }

    return KWebPage::extension(extension, option, output);
}

bool WebPage::supportsExtension(Extension extension) const
{
    //kDebug() << extension;
    if (extension == QWebPage::ErrorPageExtension)
        return true;

    return KWebPage::supportsExtension(extension);
}

QString WebPage::errorPage(int code, const QString& text, const KUrl& reqUrl) const
{
  QString errorName, techName, description;
  QStringList causes, solutions;

  QByteArray raw = KIO::rawErrorDetail( code, text, &reqUrl );
  QDataStream stream(raw);

  stream >> errorName >> techName >> description >> causes >> solutions;

  QString url, protocol, datetime;
  url = reqUrl.url();
  protocol = reqUrl.protocol();
  datetime = KGlobal::locale()->formatDateTime( QDateTime::currentDateTime(), KLocale::LongDate );

  QString filename (KStandardDirs::locate ("data", QL1S("kwebkitpart/error.html")));
  QFile file( filename );
  if ( !file.open( QIODevice::ReadOnly ) )
    return i18n("<html><body><h3>Unable to display error message</h3>"
                "<p>The error template file <em>error.html</em> could not be "
                "found.</p></body></html>");

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
