/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2008 Michael Howell <mhowell123@gmail.com>
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

// Own
#include "kwebpage.h"

// Local
#include "kwebview.h"
#include "networkaccessmanager_p.h"
#include "knetworkcookiejar.h"
#include "kwebpluginfactory.h"

// KDE
#include <kaction.h>
#include <kfiledialog.h>
#include <kinputdialog.h>
#include <kmessagebox.h>
#include <kprotocolmanager.h>
#include <kjobuidelegate.h>
#include <krun.h>
#include <kshell.h>
#include <kstandarddirs.h>
#include <kstandardshortcut.h>
#include <kurl.h>
#include <kdebug.h>
#include <klocalizedstring.h>
#include <kparts/browserrun.h>
#include <kio/accessmanager.h>
#include <kio/job.h>

// Qt
#include <QtGui/QTextDocument>
#include <QtGui/QPaintEngine>
#include <QtWebKit/QWebFrame>
#include <QtUiTools/QUiLoader>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkCookieJar>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusReply>

#define QL1(x)  QLatin1String(x)

class KWebPage::KWebPagePrivate
{
public:
    KWebPagePrivate() {}
    static QString getFileNameForDownload(const QNetworkRequest &request, QNetworkReply *reply);
};

QString KWebPage::KWebPagePrivate::getFileNameForDownload(const QNetworkRequest &request, QNetworkReply *reply)
{
    QString fileName = KUrl(request.url()).fileName();
    if (reply && reply->hasRawHeader("Content-Disposition")) { // based on code from arora, downloadmanger.cpp
        const QString value = QL1(reply->rawHeader("Content-Disposition"));
        const int pos = value.indexOf(QL1("filename="));
        if (pos != -1) {
            QString name = value.mid(pos + 9);
            if (name.startsWith(QLatin1Char('"')) && name.endsWith(QLatin1Char('"')))
                name = name.mid(1, name.size() - 2);
            fileName = name;
        }
    }
    return fileName;
}

KWebPage::KWebPage(QObject *parent)
         :QWebPage(parent), d(new KWebPage::KWebPagePrivate())
{  
    // KDE KParts integration for <embed> tag...
    setPluginFactory(new KWebPluginFactory(this));

    // KDE IO (KIO) integration...
    setNetworkAccessManager(new KDEPrivate::NetworkAccessManager(this));

    // KDE Cookiejar (KCookieJar) integration...
    WId winId = view()->window()->winId();
    KNetworkCookieJar *cookiejar = new KNetworkCookieJar;
    cookiejar->setWinId(winId);
    networkAccessManager()->setCookieJar(cookiejar);
    setSessionMetaData(QL1("window-id"), QString::number(winId));

    action(Back)->setIcon(KIcon("go-previous"));
    action(Back)->setShortcut(KStandardShortcut::back().primary());

    action(Forward)->setIcon(KIcon("go-next"));
    action(Forward)->setShortcut(KStandardShortcut::forward().primary());

    action(Reload)->setIcon(KIcon("view-refresh"));
    action(Reload)->setShortcut(KStandardShortcut::reload().primary());

    action(Stop)->setIcon(KIcon("process-stop"));
    action(Stop)->setShortcut(Qt::Key_Escape);

    action(Cut)->setIcon(KIcon("edit-cut"));
    action(Cut)->setShortcut(KStandardShortcut::cut().primary());

    action(Copy)->setIcon(KIcon("edit-copy"));
    action(Copy)->setShortcut(KStandardShortcut::copy().primary());

    action(Paste)->setIcon(KIcon("edit-paste"));
    action(Paste)->setShortcut(KStandardShortcut::paste().primary());

    action(Undo)->setIcon(KIcon("edit-undo"));
    action(Undo)->setShortcut(KStandardShortcut::undo().primary());

    action(Redo)->setIcon(KIcon("edit-redo"));
    action(Redo)->setShortcut(KStandardShortcut::redo().primary());

    action(InspectElement)->setIcon(KIcon("view-process-all"));
    action(OpenLinkInNewWindow)->setIcon(KIcon("window-new"));
    action(OpenFrameInNewWindow)->setIcon(KIcon("window-new"));
    action(OpenImageInNewWindow)->setIcon(KIcon("window-new"));
    action(CopyLinkToClipboard)->setIcon(KIcon("edit-copy"));
    action(CopyImageToClipboard)->setIcon(KIcon("edit-copy"));
    action(ToggleBold)->setIcon(KIcon("format-text-bold"));
    action(ToggleItalic)->setIcon(KIcon("format-text-italic"));
    action(ToggleUnderline)->setIcon(KIcon("format-text-underline"));
    action(DownloadLinkToDisk)->setIcon(KIcon("document-save"));
    action(DownloadImageToDisk)->setIcon(KIcon("document-save"));

    settings()->setWebGraphic(QWebSettings::MissingPluginGraphic, KIcon("preferences-plugin").pixmap(32, 32));
    settings()->setWebGraphic(QWebSettings::MissingImageGraphic, KIcon("image-missing").pixmap(32, 32));
    settings()->setWebGraphic(QWebSettings::DefaultFrameIconGraphic, KIcon("applications-internet").pixmap(32, 32));

    setForwardUnsupportedContent(true);

    connect(this, SIGNAL(downloadRequested(const QNetworkRequest &)),
            this, SLOT(slotDownloadRequest(const QNetworkRequest &)));
    connect(this, SIGNAL(unsupportedContent(QNetworkReply *)),
            this, SLOT(slotUnsupportedContent(QNetworkReply *)));
}

KWebPage::~KWebPage()
{
    delete d;
}

void KWebPage::setAllowExternalContent(bool allow)
{
    KIO::AccessManager *manager = qobject_cast<KIO::AccessManager*>(networkAccessManager());
    if (manager)
        manager->setExternalContentAllowed(allow);
}

bool KWebPage::isExternalContentAllowed() const
{
    KIO::AccessManager *manager = qobject_cast<KIO::AccessManager*>(networkAccessManager());
    if (manager)
        return manager->isExternalContentAllowed();
    return true;
}

void KWebPage::setSessionMetaData(const QString &key, const QString &value)
{
    KDEPrivate::NetworkAccessManager *manager = qobject_cast<KDEPrivate::NetworkAccessManager*>(networkAccessManager());
    if (manager)
        manager->sessionMetaData()[key] = value;
}

void KWebPage::setRequestMetaData(const QString &key, const QString &value)
{
    KDEPrivate::NetworkAccessManager *manager = qobject_cast<KDEPrivate::NetworkAccessManager*>(networkAccessManager());
    if (manager)
        manager->requestMetaData()[key] = value;
}

QString KWebPage::chooseFile(QWebFrame *frame, const QString &suggestedFile)
{
    return KFileDialog::getOpenFileName(suggestedFile, QString(), frame->page()->view());
}

void KWebPage::javaScriptAlert(QWebFrame *frame, const QString &msg)
{
    KMessageBox::error(frame->page()->view(), msg, i18n("JavaScript"));
}

bool KWebPage::javaScriptConfirm(QWebFrame *frame, const QString &msg)
{
    return (KMessageBox::warningYesNo(frame->page()->view(), msg, i18n("JavaScript"),
                                      KStandardGuiItem::ok(), KStandardGuiItem::cancel())
            == KMessageBox::Yes);
}

bool KWebPage::javaScriptPrompt(QWebFrame *frame, const QString &msg, const QString &defaultValue, QString *result)
{
    bool ok = false;

    QString text = KInputDialog::getText(i18n("JavaScript"), msg, defaultValue, &ok, frame->page()->view());

    if (result)
      *result = text;

    return ok;
}

QString KWebPage::userAgentForUrl(const QUrl& _url) const
{
    const KUrl url(_url);
    QString userAgent = KProtocolManager::userAgentForHost((url.isLocalFile() ? "localhost" : url.host()));

    if (userAgent == KProtocolManager::defaultUserAgent())
        userAgent = QWebPage::userAgentForUrl(_url);
    else {
        const int index = userAgent.indexOf("KHTML/");
        if (index > -1) {
          QString webKitUserAgent = QWebPage::userAgentForUrl(_url);
          userAgent = userAgent.left(index);
          webKitUserAgent = webKitUserAgent.mid(webKitUserAgent.indexOf("AppleWebKit/"));
          webKitUserAgent = webKitUserAgent.left(webKitUserAgent.indexOf(')') + 1);
          userAgent += webKitUserAgent;
          userAgent.remove("compatible; ");
        }
    }

    //kDebug() << userAgent;
    return userAgent;
}

bool KWebPage::acceptNavigationRequest(QWebFrame * frame, const QNetworkRequest & request, NavigationType type)
{
    kDebug() << "url: " << request.url() << ", type: " << type << ", frame: " << frame;   

    /*
      If the navigation request is from the main frame, set the cross-domain
      meta-data value to the current url for proper integration with KCookieJar...
    */
    if (frame == mainFrame()) {
        const QString scheme = request.url().scheme();
        if (QString::compare(QString::fromUtf8("about"), scheme, Qt::CaseInsensitive) != 0 &&
            QString::compare(QString::fromUtf8("file"), scheme, Qt::CaseInsensitive) != 0) {
            setSessionMetaData(QL1("cross-domain"), request.url().toString());
        }
    }

    return QWebPage::acceptNavigationRequest(frame, request, type);
}

QObject *KWebPage::createPlugin(const QString &classId, const QUrl &url, const QStringList &paramNames, const QStringList &paramValues)
{
    kDebug() << "create Plugin requested:";
    kDebug() << "classid:" << classId;
    kDebug() << "url:" << url;
    kDebug() << "paramNames:" << paramNames << " paramValues:" << paramValues;

    QUiLoader loader;
    return loader.createWidget(classId, view());
}

bool KWebPage::authorizedRequest(const QUrl &) const
{
    return true;
}

void KWebPage::slotUnsupportedContent(QNetworkReply *reply)
{
    kDebug() << "url:" << reply->url();
    kDebug() << "location:" << reply->header(QNetworkRequest::LocationHeader).toString();
    kDebug() << "error:" << reply->error();

    if (reply->url().isValid()) {
        KParts::BrowserRun::AskSaveResult res = KParts::BrowserRun::askEmbedOrSave(
                                                    reply->url(),
                                                    reply->header(QNetworkRequest::ContentTypeHeader).toString(),
                                                    d->getFileNameForDownload(reply->request(), reply));
        switch (res) {
        case KParts::BrowserRun::Save:
            slotDownloadRequest(reply->request(), reply);
            return;
        case KParts::BrowserRun::Cancel:
            return;
        default: // Open
            break;
        }
    }
}

void KWebPage::slotDownloadRequest(const QNetworkRequest &request)
{
    slotDownloadRequest(request, 0);
}

void KWebPage::slotDownloadRequest(const QNetworkRequest &request, QNetworkReply *reply)
{
    const KUrl url(request.url());
    kDebug() << url;

    const QString fileName = d->getFileNameForDownload(request, reply);

    // parts of following code are based on khtml_ext.cpp
    // DownloadManager <-> konqueror integration
    // find if the integration is enabled
    // the empty key  means no integration
    // only use download manager for non-local urls!
    bool downloadViaKIO = true;
    if (!url.isLocalFile()) {
        KConfigGroup cfg = KSharedConfig::openConfig("konquerorrc", KConfig::NoGlobals)->group("HTML Settings");
        const QString downloadManger = cfg.readPathEntry("DownloadManager", QString());
        if (!downloadManger.isEmpty()) {
            // then find the download manager location
            kDebug() << "Using: " << downloadManger << " as Download Manager";
            QString cmd = KStandardDirs::findExe(downloadManger);
            if (cmd.isEmpty()) {
                QString errMsg = i18n("The Download Manager (%1) could not be found in your $PATH.", downloadManger);
                QString errMsgEx = i18n("Try to reinstall it. \n\nThe integration with Konqueror will be disabled.");
                KMessageBox::detailedSorry(view(), errMsg, errMsgEx);
                cfg.writePathEntry("DownloadManager", QString());
                cfg.sync();
            } else {
                downloadViaKIO = false;
                cmd += ' ' + KShell::quoteArg(url.url());
                kDebug() << "Calling command" << cmd;
                KRun::runCommand(cmd, view());
            }
        }
    }

    if (downloadViaKIO) {
        const QString destUrl = KFileDialog::getSaveFileName(url.fileName(), QString(), view());
        if (destUrl.isEmpty()) return;
        KIO::Job *job = KIO::file_copy(url, KUrl(destUrl), -1, KIO::Overwrite);

        QVariant attr = request.attribute(QNetworkRequest::User);
        if (attr.isValid() && attr.type() == QVariant::Map) {
            KIO::MetaData metaData (attr.toMap());
            job->setMetaData(metaData);
        }
        job->addMetaData("MaxCacheSize", "0"); // Don't store in http cache.
        job->addMetaData("cache", "cache"); // Use entry from cache if available.
        job->uiDelegate()->setAutoErrorHandlingEnabled(true);
    }
}
