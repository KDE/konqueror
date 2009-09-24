/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Dirk Mueller <mueller@kde.org>
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
#include <KIO/Job>

#include <QWebFrame>
#include <QtNetwork/QNetworkReply>

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
            QVariant attr = reply->attribute(QNetworkRequest::UserMax);
            if (attr.isValid() && attr.type() == QVariant::Int)
              return attr.toInt();
            else
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

KSslInfoDialog *WebPage::sslDialog() const
{
    KSslInfoDialog *dlg = 0;

    if (isSecurePage()) {
        dlg = new KSslInfoDialog(0);
        dlg->setSslInfo(d->sslInfo.certificateChain(),
                        d->sslInfo.peerAddress().toString(),
                        mainFrame()->url().host(),
                        d->sslInfo.protocol(),
                        d->sslInfo.ciphers(),
                        d->sslInfo.usedChiperBits(),
                        d->sslInfo.supportedChiperBits(),
                        KSslInfoDialog::errorsFromString(d->sslInfo.certificateErrors()));
    }

    return dlg;
}

bool WebPage::isSecurePage() const
{
    return d->sslInfo.isValid();
}

bool WebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request,
                                      NavigationType type)
{
    if (frame) {
        // TODO: Check if we need to flag navigation from javascript as well.
        // Currently QtWebKit provides no way to distinguish such requests and
        // lumps them under QWebPage::NavigationTypeOther.
        if (type == QWebPage::NavigationTypeLinkClicked) {
            const QString proto = d->part->url().scheme().toLower();
            if (proto == "https" || proto == "webdavs")
                setRequestMetaData("ssl_was_in_use", "TRUE");
        }

        setRequestMetaData("main_frame_request", (frame->parentFrame() ? "FALSE" : "TRUE"));

        // Set up the SSL information that might have been obtained if and when
        // the container of this part did a stat to determine mime-type.
        // ahhh... the price one pays for overly flexiable architecture.
        if (type ==  QWebPage::NavigationTypeOther)
            d->sslInfo.setup(request.attribute(QNetworkRequest::User), request.url());

    } else {
        // if frame is NULL, it means a new window is requested so we enforce
        // the user's preferred choice here from the settings...
        switch (WebKitSettings::self()->windowOpenPolicy(request.url().host())) {
            case WebKitSettings::KJSWindowOpenDeny:
                return false;
            case WebKitSettings::KJSWindowOpenAsk:
                // TODO: Implement this without resotring to showing a KMessgaeBox. Perhaps
                // how FF3 does it ?
                break;
            case WebKitSettings::KJSWindowOpenSmart:
                if (type != QWebPage::NavigationTypeLinkClicked)
                    return false;
            case WebKitSettings::KJSWindowOpenAllow:
            default:
                break;
        }
    }

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
    QUrl frameUrl (mainFrame()->url());

    if (replyUrl == frameUrl) {
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
                d->sslInfo.setup(reply->attribute(QNetworkRequest::User), reply->url());

            emit loadMainPageFinished();
        }
    }
}
