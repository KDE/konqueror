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
#include "webview.h"
#include "webkitglobal.h"
#include "network/knetworkaccessmanager.h"
#include "settings/webkitsettings.h"

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

WebPage::WebPage(WebKitPart *wpart, QWidget *parent)
    : QWebPage(parent)
    , m_part(wpart)
{
    setNetworkAccessManager(new KNetworkAccessManager(this));
    
    action(QWebPage::Back)->setIcon(KIcon("go-previous"));
    action(QWebPage::Back)->setShortcut(KStandardShortcut::back().primary());

    action(QWebPage::Forward)->setIcon(KIcon("go-next"));
    action(QWebPage::Forward)->setShortcut(KStandardShortcut::forward().primary());

    action(QWebPage::Reload)->setIcon(KIcon("view-refresh"));
    action(QWebPage::Reload)->setShortcut(KStandardShortcut::reload().primary());

    action(QWebPage::Stop)->setIcon(KIcon("process-stop"));
    action(QWebPage::Stop)->setShortcut(Qt::Key_Escape);

    action(QWebPage::Cut)->setIcon(KIcon("edit-cut"));
    action(QWebPage::Cut)->setShortcut(KStandardShortcut::cut().primary());

    action(QWebPage::Copy)->setIcon(KIcon("edit-copy"));
    action(QWebPage::Copy)->setShortcut(KStandardShortcut::copy().primary());

    action(QWebPage::Paste)->setIcon(KIcon("edit-paste"));
    action(QWebPage::Paste)->setShortcut(KStandardShortcut::paste().primary());

    action(QWebPage::Undo)->setIcon(KIcon("edit-undo"));
    action(QWebPage::Undo)->setShortcut(KStandardShortcut::undo().primary());

    action(QWebPage::Redo)->setIcon(KIcon("edit-redo"));
    action(QWebPage::Redo)->setShortcut(KStandardShortcut::redo().primary());

    action(QWebPage::InspectElement)->setIcon(KIcon("view-process-all"));
    action(QWebPage::OpenLinkInNewWindow)->setIcon(KIcon("window-new"));
    action(QWebPage::OpenFrameInNewWindow)->setIcon(KIcon("window-new"));
    action(QWebPage::OpenImageInNewWindow)->setIcon(KIcon("window-new"));
    action(QWebPage::CopyLinkToClipboard)->setIcon(KIcon("edit-copy"));
    action(QWebPage::CopyImageToClipboard)->setIcon(KIcon("edit-copy"));
    action(QWebPage::ToggleBold)->setIcon(KIcon("format-text-bold"));
    action(QWebPage::ToggleItalic)->setIcon(KIcon("format-text-italic"));
    action(QWebPage::ToggleUnderline)->setIcon(KIcon("format-text-underline"));
    action(QWebPage::DownloadLinkToDisk)->setIcon(KIcon("document-save"));
    action(QWebPage::DownloadImageToDisk)->setIcon(KIcon("document-save"));

    connect(this, SIGNAL(geometryChangeRequested(const QRect &)),
            this, SLOT(slotGeometryChangeRequested(const QRect &)));
    connect(this, SIGNAL(windowCloseRequested()),
            this, SLOT(slotWindowCloseRequested()));
    connect(this, SIGNAL(downloadRequested(const QNetworkRequest &)),
            this, SLOT(slotDownloadRequested(const QNetworkRequest &)));
    setForwardUnsupportedContent(true);
    connect(this, SIGNAL(unsupportedContent(QNetworkReply *)),
            this, SLOT(slotHandleUnsupportedContent(QNetworkReply *)));
    connect(this, SIGNAL(statusBarMessage(const QString &)),
            this, SLOT(slotStatusBarMessage(const QString &)));
}

bool WebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request,
                                      NavigationType type)
{
    kDebug() << "acceptNavigationRequest" << request.url();

    return true;
}

QString WebPage::chooseFile(QWebFrame *frame, const QString &suggestedFile)
{
    return KFileDialog::getOpenFileName(suggestedFile, QString(), frame->page()->view());
}

void WebPage::javaScriptAlert(QWebFrame *frame, const QString &msg)
{
    KMessageBox::error(frame->page()->view(), msg, i18n("JavaScript"));
}

bool WebPage::javaScriptConfirm(QWebFrame *frame, const QString &msg)
{
    return (KMessageBox::warningYesNo(frame->page()->view(), msg, i18n("JavaScript"), KStandardGuiItem::ok(), KStandardGuiItem::cancel())
            == KMessageBox::Yes);
}

bool WebPage::javaScriptPrompt(QWebFrame *frame, const QString &msg, const QString &defaultValue, QString *result)
{
    bool ok = false;
    *result = KInputDialog::getText(i18n("JavaScript"), msg, defaultValue, &ok, frame->page()->view());
    return ok;
}

QString WebPage::userAgentForUrl(const QUrl& _url) const
{
    KUrl url(_url);
    QString host = url.isLocalFile() ? "localhost" : url.host();

    QString userAgent = KProtocolManager::userAgentForHost(host);
    int indexOfKhtml = userAgent.indexOf("KHTML/");
    if (indexOfKhtml == -1) // not a KHTML user agent, so no need to "update" it
        return userAgent;
    userAgent = userAgent.left(indexOfKhtml);

    QString webKitUserAgent = QWebPage::userAgentForUrl(url);
    webKitUserAgent = webKitUserAgent.mid(webKitUserAgent.indexOf("AppleWebKit/"));
    webKitUserAgent = webKitUserAgent.left(webKitUserAgent.indexOf(')') + 1);
    userAgent += webKitUserAgent;
    return userAgent;
}

void WebPage::slotHandleUnsupportedContent(QNetworkReply *reply)
{
    KUrl url(reply->request().url());
    kDebug() << "title:" << url;
    kDebug() << "error:" << reply->errorString();

    KParts::BrowserRun::AskSaveResult res = KParts::BrowserRun::askEmbedOrSave(
                                                url,
                                                reply->header(QNetworkRequest::ContentTypeHeader).toString(),
                                                url.fileName());
    switch (res) {
    case KParts::BrowserRun::Save:
        slotDownloadRequested(reply->request());
        return;
    case KParts::BrowserRun::Cancel:
        return;
    default: // Open
        break;
    }
}

QObject *WebPage::createPlugin(const QString &classid, const QUrl &url, const QStringList &paramNames, const QStringList &paramValues)
{
    kDebug() << "create Plugin requested:";
    kDebug() << "classid:" << classid;
    kDebug() << "url:" << url;
    kDebug() << "paramNames:" << paramNames << " paramValues:" << paramValues;
    return 0;
}

QWebPage *WebPage::createWindow(WebWindowType type)
{
    if (WebKitGlobal::settings()->windowOpenPolicy(mainFrame()->url().host()) != WebKitSettings::KJSWindowOpenDeny)
        return 0;

    kDebug() << type;
    KParts::ReadOnlyPart *part = 0;
    KParts::OpenUrlArguments args;
    //if (type == QWebPage::WebModalDialog) //TODO: correct behavior?
        args.metaData()["forcenewwindow"] = "true";

    emit m_part->browserExtension()->createNewWindow(KUrl("about:blank"), args,
                                                     KParts::BrowserArguments(),
                                                     KParts::WindowArgs(), &part);
    WebKitPart *webKitPart = qobject_cast<WebKitPart*>(part);
    if (!webKitPart) {
        kDebug() << "got NOT a WebKitPart but a" << part->metaObject()->className();
        return 0;
    }
    return webKitPart->view()->page();
}

void WebPage::slotGeometryChangeRequested(const QRect &rect)
{
    const QString host = mainFrame()->url().host();

    if (WebKitGlobal::settings()->windowMovePolicy(host) == WebKitSettings::KJSWindowMoveAllow) {
        emit m_part->browserExtension()->moveTopLevelWidget(rect.x(), rect.y());
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

    if (WebKitGlobal::settings()->windowResizePolicy(host) == WebKitSettings::KJSWindowResizeAllow) {
        kDebug() << "resizing to " << width << "x" << height;
        emit m_part->browserExtension()->resizeTopLevelWidget(width, height);
    }

    // If the window is out of the desktop, move it up/left
    // (maybe we should use workarea instead of sg, otherwise the window ends up below kicker)
    int right = view()->x() + view()->frameGeometry().width();
    int bottom = view()->y() + view()->frameGeometry().height();
    int moveByX = 0;
    int moveByY = 0;
    if (right > sg.right())
        moveByX = - right + sg.right(); // always <0
    if (bottom > sg.bottom())
        moveByY = - bottom + sg.bottom(); // always <0
    if ((moveByX || moveByY) &&
      WebKitGlobal::settings()->windowMovePolicy(host) == WebKitSettings::KJSWindowMoveAllow) {
        emit m_part->browserExtension()->moveTopLevelWidget(view()->x() + moveByX, view()->y() + moveByY);
    }
}

void WebPage::slotWindowCloseRequested()
{
    emit m_part->browserExtension()->requestFocus(m_part);
    if (KMessageBox::questionYesNo(view(),
                                   i18n("Close window?"), i18n("Confirmation Required"),
                                   KStandardGuiItem::close(), KStandardGuiItem::cancel())
      == KMessageBox::Yes) {
        m_part->deleteLater();
        m_part = 0;
    }
}

void WebPage::slotDownloadRequested(const QNetworkRequest &request)
{
    KUrl url(request.url());
    kDebug() << url;

    // parts of following code are based on khtml_ext.cpp
    // DownloadManager <-> konqueror integration
    // find if the integration is enabled
    // the empty key  means no integration
    // only use download manager for non-local urls!
    bool downloadViaKIO = true;
    if (!url.isLocalFile()) {
        KConfigGroup cfg = KSharedConfig::openConfig("konquerorrc", KConfig::NoGlobals)->group("HTML Settings");
        QString downloadManger = cfg.readPathEntry("DownloadManager", QString());
        if (!downloadManger.isEmpty()) {
            // then find the download manager location
            kDebug() << "Using: " << downloadManger << " as Download Manager";
            QString cmd = KStandardDirs::findExe(downloadManger);
            if (cmd.isEmpty()) {
                QString errMsg = i18n("The Download Manager (%1) could not be found in your $PATH.", downloadManger);
                QString errMsgEx = i18n("Try to reinstall it. \n\nThe integration with Konqueror will be disabled.");
                KMessageBox::detailedSorry(view(), errMsg, errMsgEx);
                cfg.writePathEntry("DownloadManager", QString());
                cfg.sync ();
            } else {
                downloadViaKIO = false;
                cmd += ' ' + KShell::quoteArg(url.url());
                kDebug() << "Calling command" << cmd;
                KRun::runCommand(cmd, view());
            }
        }
    }

    if (downloadViaKIO) {
        QString destUrl = KFileDialog::getOpenFileName(url.fileName(), QString(), view());
        KIO::Job *job = KIO::file_copy(url, KUrl(destUrl), -1, KIO::Overwrite);
        //job->setMetaData(metadata); //TODO: add metadata from request
        job->addMetaData("MaxCacheSize", "0"); // Don't store in http cache.
        job->addMetaData("cache", "cache"); // Use entry from cache if available.
        job->uiDelegate()->setAutoErrorHandlingEnabled(true);
    }
}

void WebPage::slotStatusBarMessage(const QString &message)
{
    if (WebKitGlobal::settings()->windowStatusPolicy(mainFrame()->url().host()) == WebKitSettings::KJSWindowStatusAllow) {
        m_part->setStatusBarTextProxy(message);
    }
}
