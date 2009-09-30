/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
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
#include "webkitpart.h"
#include "websslinfo.h"
#include "webview.h"
#include "sslinfodialog_p.h"

#include <kdewebkit/kwebpage.h>
#include <kdewebkit/settings/webkitsettings.h>

#include <KDE/KParts/GenericFactory>
#include <KDE/KParts/BrowserRun>
#include <KDE/KAboutData>
#include <KDE/KAction>
#include <KDE/KFileDialog>
#include <KDE/KInputDialog>
#include <KDE/KMessageBox>
#include <KDE/KProtocolManager>
#include <KDE/KGlobalSettings>
#include <KDE/KJobUiDelegate>
#include <KDE/KRun>
#include <KDE/KShell>
#include <KDE/KStandardDirs>
#include <KDE/KStandardShortcut>
#include <KDE/KAuthorized>
#include <KIO/Job>
#include <KIO/AccessManager>

#include <QtWebKit/QWebFrame>
#include <QtGui/QTextDocument>
#include <QtNetwork/QNetworkReply>

#define QL1(x)  QLatin1String(x)
typedef QPair<QString, QString> StringPair;

// Sanitizes the "mailto:" url, e.g. strips out any "attach" parameters.
static QUrl sanitizeMailToUrl(const QUrl &url, QStringList& files) {
    QUrl sanitizedUrl;    
    
    // NOTE: This is necessary to ensure we can properly use QUrl's query
    // related APIs to process 'mailto:' urls of form 'mailto:foo@bar.com'.
    if (url.hasQuery())
      sanitizedUrl = url;
    else
      sanitizedUrl = QUrl(url.scheme() + QL1(":?") + url.path());

    QListIterator<StringPair> it (sanitizedUrl.queryItems());
    sanitizedUrl.setEncodedQuery(QByteArray());    // clear out the query componenet

    while (it.hasNext()) {
        StringPair queryItem = it.next();
        if (queryItem.first.contains(QChar('@')) && queryItem.second.isEmpty()) {
            queryItem.second = queryItem.first;
            queryItem.first = "to";
        } else if (QString::compare(queryItem.first, QL1("attach"), Qt::CaseInsensitive) == 0) {
            files << queryItem.second;
            continue;
        }        
        sanitizedUrl.addQueryItem(queryItem.first, queryItem.second);
    }

    return sanitizedUrl;
}

// Converts QNetworkReply::Error to KIO::Error...
// NOTE: This probably needs to be moved somewhere more convenient
// in the future. Perhaps KIO::AccessManager itself ???
static int convertErrorCode(QNetworkReply* reply)
{
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
        case QNetworkReply::UnknownNetworkError:{
#if KDE_IS_VERSION(4,3,2)
            QVariant attr = reply->attribute(static_cast<QNetworkRequest::Attribute>(KIO::AccessManager::KioError));
            if (attr.isValid() && attr.type() == QVariant::Int)
              return attr.toInt();
            else
#endif
              return KIO::ERR_UNKNOWN;
        }
        case QNetworkReply::NoError:
        default:
            return 0;
    }
}

static bool domainSchemeMatch(const QUrl& u1, const QUrl& u2)
{
  if (u1.scheme() != u2.scheme())
    return false;

  QStringList u1List = u1.host().split(QChar('.'), QString::SkipEmptyParts);
  QStringList u2List = u2.host().split(QChar('.'), QString::SkipEmptyParts);

  if (qMin(u1List.count(), u2List.count()) < 2) {
      return false;  // better safe than sorry...
  }

  while (u1List.count() > 2)
      u1List.removeFirst();

  while (u2List.count() > 2)
      u2List.removeFirst();

  return (u1List == u2List);
}

class SslInfo : public WebSslInfo
{
public:
    SslInfo() {}
    SslInfo(const WebSslInfo& other) : WebSslInfo(other) {}

    SslInfo& operator = (const WebSslInfo& other)
    {
        WebSslInfo::operator =(other);
        return *this;
    }

    inline void clear () { reset(); }

    void setup (const QVariant& attr, const QUrl& url)
    {
        if (attr.isValid() && attr.type() == QVariant::Map) {
            QMap<QString,QVariant> metaData = attr.toMap();
            if (metaData.value("ssl_in_use", false).toBool()) {
                setUrl(url);
                setCertificateChain(metaData.value("ssl_peer_chain").toByteArray());
                setPeerAddress(metaData.value("ssl_peer_ip").toString());
                setParentAddress(metaData.value("ssl_parent_ip").toString());
                setProtocol(metaData.value("ssl_protocol_version").toString());
                setCiphers(metaData.value("ssl_cipher").toString());
                setCertificateErrors(metaData.value("ssl_cert_errors").toString());
                setUsedCipherBits(metaData.value("ssl_cipher_used_bits").toString());
                setSupportedCipherBits(metaData.value("ssl_cipher_bits").toString());
            }
        }
    }
};

class WebPage::WebPagePrivate
{
public:
   WebPagePrivate():part(0) {}

    QUrl workingUrl;
    SslInfo sslInfo;
    WebKitPart *part;
};

WebPage::WebPage(WebKitPart *part, QWidget *parent)
        :KWebPage(parent), d (new WebPagePrivate)
{
    d->part = part;
    setSessionMetaData("ssl_activate_warnings", "TRUE");
    connect(this, SIGNAL(geometryChangeRequested(const QRect &)),
            this, SLOT(slotGeometryChangeRequested(const QRect &)));
    connect(this, SIGNAL(windowCloseRequested()),
            this, SLOT(slotWindowCloseRequested()));
    connect(this, SIGNAL(statusBarMessage(const QString &)),
            this, SLOT(slotStatusBarMessage(const QString &)));
    connect(networkAccessManager(), SIGNAL(finished(QNetworkReply*)),
            this, SLOT(slotRequestFinished(QNetworkReply*)));
}

WebPage::~WebPage()
{
    delete d;
}

void WebPage::saveUrl(const KUrl &url)
{
    slotDownloadRequested(QNetworkRequest(url));
}

void WebPage::setupSslDialog(KSslInfoDialog &dlg) const
{
    if (isSecurePage()) {
        dlg.setSslInfo(d->sslInfo.certificateChain(),
                       d->sslInfo.peerAddress().toString(),
                       mainFrame()->url().host(),
                       d->sslInfo.protocol(),
                       d->sslInfo.ciphers(),
                       d->sslInfo.usedChiperBits(),
                       d->sslInfo.supportedChiperBits(),
                       KSslInfoDialog::errorsFromString(d->sslInfo.certificateErrors()));
    }
}

bool WebPage::isSecurePage() const
{
    return d->sslInfo.isValid();
}

bool WebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request,
                                      NavigationType type)
{
    if (frame) {      
        /*
          NOTE: At this point, the only way the request and part urls are equal is
          if the navigation request was generated by through a call to the part's
          openUrl function, i.e. a request from the embedding application.
        */
        if (d->part->url() == request.url()) {
            d->sslInfo.setup(request.attribute(QNetworkRequest::User), request.url());
        } else {
            const QString proto = d->part->url().scheme().toLower();
            if (proto == "https" || proto == "webdavs")
                setRequestMetaData("ssl_was_in_use", "TRUE");

            // Check whether or not the request is authorized...
            if (!authorizedRequest(request, type))
              return false;
        }

        // Sanitize and handle "mailto:" url ourselves...
        if (QString::compare(request.url().scheme(), QL1("mailto"), Qt::CaseInsensitive) == 0) {
            QStringList files;
            QUrl url (sanitizeMailToUrl(request.url(), files));

            switch (type) {
                case QWebPage::NavigationTypeLinkClicked:
                    if (!files.isEmpty() && KMessageBox::warningContinueCancelList(0,
                                                                                   i18n("<qt>Do you want to allow this site to attach "
                                                                                        "the following files to the email message ?</qt>"),
                                                                                   files, i18n("Email Attachment Confirmation"),
                                                                                   KGuiItem(i18n("&Allow attachements")),
                                                                                   KGuiItem(i18n("&Ignore attachements")), QL1("WarnEmailAttachment")) == KMessageBox::Continue) {

                        Q_FOREACH(const QString& file, files) {
                            url.addQueryItem(QL1("attach"), file); // Re-add back the attachments...
                        }
                    }
                    break;
                case QWebPage::NavigationTypeFormSubmitted:
                case QWebPage::NavigationTypeFormResubmitted:
                    if (!files.isEmpty()) {
                      KMessageBox::information(0, i18n("This site attempted to attach a file from your "
                                                       "computer in the form submission. The attachment "
                                                       "was removed for your protection."),
                                               i18n("KDE"), "InfoTriedAttach");
                    }
                    break;
                default:
                     break;
            }

            kDebug() << "Emitting openUrlRequest " << url;
            emit d->part->browserExtension()->openUrlRequest(url);
            return false;
        }

        setRequestMetaData("main_frame_request", (frame->parentFrame() ? "FALSE" : "TRUE"));

    } else {
        kDebug() << "open in new window" << request.url();

        // if frame is NULL, it means a new window is requested so we enforce
        // the user's preferred choice here from the settings...
        switch (WebKitSettings::self()->windowOpenPolicy(request.url().host())) {
            case WebKitSettings::KJSWindowOpenDeny:
                return false;
            case WebKitSettings::KJSWindowOpenAsk:
                // TODO: Implement this without resotring to showing a KMessgaeBox. Perhaps
                // check how it is handled in other browers (FF3, Chrome) ?
                break;
            case WebKitSettings::KJSWindowOpenSmart:
                if (type != QWebPage::NavigationTypeLinkClicked)
                    return false;
            case WebKitSettings::KJSWindowOpenAllow:
            default:
                break;
        }
    }

    d->workingUrl = request.url();
    return KWebPage::acceptNavigationRequest(frame, request, type);
}

KWebPage *WebPage::newWindow(WebWindowType type)
{
    kDebug() << type;
    KParts::ReadOnlyPart *part = 0;
    KParts::OpenUrlArguments args;
    args.metaData()["referrer"] = mainFrame()->url().toString();
    //if (type == WebModalDialog) //TODO: correct behavior?
        args.metaData()["forcenewwindow"] = "true";

    emit d->part->browserExtension()->createNewWindow(KUrl("about:blank"), args,
                                                     KParts::BrowserArguments(),
                                                     KParts::WindowArgs(), &part);
    WebKitPart *webKitPart = qobject_cast<WebKitPart*>(part);
    if (!webKitPart) {
        if (part)
            kDebug() << "got NOT a WebKitPart but a" << part->metaObject()->className();
        else
            kDebug() << "part is null";

        return 0;
    }
    return webKitPart->view()->page();
}

void WebPage::slotGeometryChangeRequested(const QRect &rect)
{
    const QString host = mainFrame()->url().host();

    if (WebKitSettings::self()->windowMovePolicy(host) == WebKitSettings::KJSWindowMoveAllow) { // Why doesn't this work?
        emit d->part->browserExtension()->moveTopLevelWidget(rect.x(), rect.y());
    }

    int height = rect.height();
    int width = rect.width();

    // parts of following code are based on kjs_window.cpp
    // Security check: within desktop limits and bigger than 100x100 (per spec)
    if (width < 100 || height < 100) {
        kDebug() << "Window resize refused, window would be too small (" << width << "," << height << ")";
        return;
    }

    QRect sg = KGlobalSettings::desktopGeometry(view());

    if (width > sg.width() || height > sg.height()) {
        kDebug() << "Window resize refused, window would be too big (" << width << "," << height << ")";
        return;
    }

    if (WebKitSettings::self()->windowResizePolicy(host) == WebKitSettings::KJSWindowResizeAllow) {
        kDebug() << "resizing to " << width << "x" << height;
        emit d->part->browserExtension()->resizeTopLevelWidget(width, height);
    }

    // If the window is out of the desktop, move it up/left
    // (maybe we should use workarea instead of sg, otherwise the window ends up below kicker)
    const int right = view()->x() + view()->frameGeometry().width();
    const int bottom = view()->y() + view()->frameGeometry().height();
    int moveByX = 0;
    int moveByY = 0;
    if (right > sg.right())
        moveByX = - right + sg.right(); // always <0
    if (bottom > sg.bottom())
        moveByY = - bottom + sg.bottom(); // always <0
    if ((moveByX || moveByY) 
      && WebKitSettings::self()->windowMovePolicy(host) == WebKitSettings::KJSWindowMoveAllow) {
        emit d->part->browserExtension()->moveTopLevelWidget(view()->x() + moveByX, view()->y() + moveByY);
    }
}

void WebPage::slotWindowCloseRequested()
{
    emit d->part->browserExtension()->requestFocus(d->part);
    if (KMessageBox::questionYesNo(view(),
                                   i18n("Close window ?"), i18n("Confirmation Required"),
                                   KStandardGuiItem::close(), KStandardGuiItem::cancel())
        == KMessageBox::Yes) {
        d->part->deleteLater();
        d->part = 0;
    }
}

void WebPage::slotStatusBarMessage(const QString &message)
{
    if (WebKitSettings::self()->windowStatusPolicy(mainFrame()->url().host()) == WebKitSettings::KJSWindowStatusAllow) {
        d->part->setStatusBarTextProxy(message);
    }
}

void WebPage::slotHandleUnsupportedContent(QNetworkReply *reply)
{
    const KUrl url(reply->request().url());
    kDebug() << url;
    KParts::OpenUrlArguments args;
    Q_FOREACH (const QByteArray &headerName, reply->rawHeaderList()) {
        args.metaData().insert(QString(headerName), QString(reply->rawHeader(headerName)));
    }
    emit d->part->browserExtension()->openUrlRequest(url, args, KParts::BrowserArguments());
}

void WebPage::slotRequestFinished(QNetworkReply *reply)
{
    Q_ASSERT(reply);

    QUrl replyUrl (reply->url());

    if (replyUrl == d->workingUrl) {
        if (d->sslInfo.isValid() && !domainSchemeMatch(replyUrl, d->sslInfo.url()))
            d->sslInfo.clear();

        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (statusCode > 300 && statusCode < 304) {
            kDebug() << "Redirected to " << replyUrl;
            emit d->part->browserExtension()->setLocationBarUrl(KUrl(replyUrl).prettyUrl());
        } else {
            // Handle any error messages...
            const int code = convertErrorCode(reply);
            switch (code) {
                case 0:
                    break;
                case KIO::ERR_USER_CANCELED: // Do nothing if request is cancelled.
                    kDebug() << "User aborted request!";
                    emit loadAborted(QUrl());
                    return;
                // Handle the user clicking on a link that refers to a directory
                // Since KIO cannot automatically convert a GET request to a LISTDIR one.
                case KIO::ERR_IS_DIRECTORY:
                    emit loadAborted(replyUrl);
                    return;
                default:
                    emit loadError(code, reply->errorString());
                    return;
            }

            if (!d->sslInfo.isValid())
#if KDE_IS_VERSION(4,3,2)
                d->sslInfo.setup(reply->attribute(static_cast<QNetworkRequest::Attribute>(KIO::AccessManager::MetaData)), reply->url());
#else
                d->sslInfo.setup(reply->attribute(QNetworkRequest::User), reply->url());
#endif

            emit loadMainPageFinished();
        }
    }
}

bool WebPage::authorizedRequest(const QNetworkRequest &req, NavigationType type) const
{
    QString buttonText;
    QString title (i18n("Security Alert"));
    QString message (i18n("<qt>Access by untrusted page to<br/><b>%1</b><br/> denied.</qt>",
                          Qt::escape(KUrl(req.url()).prettyUrl())));

    KUrl linkUrl (req.url());    

    switch (type) {
        case QWebPage::NavigationTypeLinkClicked:
            message = i18n("<qt>This untrusted page links to<br/><b>%1</b>."
                           "<br/>Do you want to follow the link?</qt>").arg(linkUrl.url());
            title = i18n("Security Warning");
            buttonText = i18n("Follow");
            break;
        case QWebPage::NavigationTypeFormSubmitted:
        case QWebPage::NavigationTypeFormResubmitted:
            if (!checkFormData(req))
                return false;
            break;
        default:
            break;
    }

    // Check whether the request is authorized or not...
    if (!KAuthorized::authorizeUrlAction("redirect", mainFrame()->url(), linkUrl)) {
        int response = KMessageBox::Cancel;
        if (message.isEmpty()) {
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
    const QString scheme (req.url().scheme().toLower());

    if (d->sslInfo.isValid() && scheme != "https" && scheme != "mailto" &&
        (KMessageBox::warningContinueCancel(0,
                                           i18n("Warning: This is a secure form "
                                                "but it is attempting to send "
                                                "your data back unencrypted.\n"
                                                "A third party may be able to "
                                                "intercept and view this "
                                                "information.\nAre you sure you "
                                                "wish to continue?"),
                                           i18n("Network Transmission"),
                                           KGuiItem(i18n("&Send Unencrypted")))  == KMessageBox::Cancel)) {

        return false;
    }


    if ((scheme == QL1("mailto")) &&
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
