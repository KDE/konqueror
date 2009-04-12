/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2008 Michael Howell <mhowell123@gmail.com>
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

#include "kwebpage.h"

#include "kwebpluginfactory.h"
#include "settings/webkitsettings.h"

#include <KDE/KParts/GenericFactory>
#include <KDE/KParts/BrowserRun>
#include <KDE/KAction>
#include <KDE/KFileDialog>
#include <KDE/KInputDialog>
#include <KDE/KMessageBox>
#include <KDE/KProtocolManager>
#include <KDE/KJobUiDelegate>
#include <KDE/KRun>
#include <KDE/KShell>
#include <KDE/KStandardDirs>
#include <KDE/KStandardShortcut>
#include <KIO/Job>
#if KDE_IS_VERSION(4, 2, 70)
#include <KIO/AccessManager>
#else
#include <kdenetwork/knetworkaccessmanager.h>
#include <kdenetwork/knetworkreply.h>
#endif

#include <QWebFrame>
#include <QUiLoader>
#include <QtNetwork/QNetworkReply>

#if KDE_IS_VERSION(4, 2, 70)
class NullNetworkReply : public QNetworkReply
{
public:
    NullNetworkReply() { QTimer::singleShot(0, this, SIGNAL(finished())); }
    virtual void abort() {};
    virtual qint64 bytesAvailable() const { return -1; };
protected:
    virtual qint64 readData(char*, qint64) { return -1; };
};

class NetworkAccessManager : public KIO::AccessManager
{
public:
    NetworkAccessManager(QObject *parent) : KIO::AccessManager(parent) {}
protected:
    virtual QNetworkReply *createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData = 0)
    {
        if (WebKitSettings::self()->isAdFilterEnabled() && WebKitSettings::self()->isAdFiltered(req.url().toString())) {
            return new NullNetworkReply();
        }
        return KIO::AccessManager::createRequest(op, req, outgoingData);
    }
};
#else
class NetworkAccessManager : public KNetworkAccessManager
{
public:
    NetworkAccessManager(QObject *parent) : KNetworkAccessManager(parent) {}
protected:
    virtual QNetworkReply *createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData = 0)
    {
        if (WebKitSettings::self()->isAdFilterEnabled() && WebKitSettings::self()->isAdFiltered(req.url().toString())) {
            return new KNetworkReply(KNetworkAccessManager::Operation(), req, 0, this);
        }
        return KNetworkAccessManager::createRequest(op, req, outgoingData);
    }
};
#endif

class KWebPage::KWebPagePrivate
{
public:
    KWebPagePrivate() {}
};

KWebPage::KWebPage(QObject *parent)
    : QWebPage(parent), d(new KWebPage::KWebPagePrivate())
{
#if KDE_IS_VERSION(4, 2, 70)
    setNetworkAccessManager(new KIO::AccessManager(this));
#else
    setNetworkAccessManager(new KNetworkAccessManager(this));
#endif
    setPluginFactory(new KWebPluginFactory(pluginFactory(), this));

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

    const QString host = mainFrame()->url().host();

    connect(this, SIGNAL(downloadRequested(const QNetworkRequest &)),
            this, SLOT(slotDownloadRequested(const QNetworkRequest &)));
    setForwardUnsupportedContent(true);
    connect(this, SIGNAL(unsupportedContent(QNetworkReply *)),
            this, SLOT(slotHandleUnsupportedContent(QNetworkReply *)));
}

KWebPage::~KWebPage()
{
    delete d;
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
    return (KMessageBox::warningYesNo(frame->page()->view(), msg, i18n("JavaScript"), KStandardGuiItem::ok(), KStandardGuiItem::cancel())
            == KMessageBox::Yes);
}

bool KWebPage::javaScriptPrompt(QWebFrame *frame, const QString &msg, const QString &defaultValue, QString *result)
{
    bool ok = false;
    *result = KInputDialog::getText(i18n("JavaScript"), msg, defaultValue, &ok, frame->page()->view());
    return ok;
}

QString KWebPage::userAgentForUrl(const QUrl& _url) const
{
    const KUrl url(_url);
    const QString host = url.isLocalFile() ? "localhost" : url.host();

    QString userAgent = KProtocolManager::userAgentForHost(host);
    const int indexOfKhtml = userAgent.indexOf("KHTML/");
    if (indexOfKhtml == -1) // not a KHTML user agent, so no need to "update" it
        return userAgent;

    userAgent = userAgent.left(indexOfKhtml);

    QString webKitUserAgent = QWebPage::userAgentForUrl(url);
    webKitUserAgent = webKitUserAgent.mid(webKitUserAgent.indexOf("AppleWebKit/"));
    webKitUserAgent = webKitUserAgent.left(webKitUserAgent.indexOf(')') + 1);
    userAgent += webKitUserAgent;

    userAgent.remove("compatible; ");

    return userAgent;
}

void KWebPage::slotHandleUnsupportedContent(QNetworkReply *reply)
{
    const KUrl url(reply->request().url());
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

QObject *KWebPage::createPlugin(const QString &classId, const QUrl &url, const QStringList &paramNames, const QStringList &paramValues)
{
    kDebug() << "create Plugin requested:";
    kDebug() << "classid:" << classId;
    kDebug() << "url:" << url;
    kDebug() << "paramNames:" << paramNames << " paramValues:" << paramValues;

    QUiLoader loader;
    return loader.createWidget(classId, view());
}

void KWebPage::slotDownloadRequested(const QNetworkRequest &request)
{
    const KUrl url(request.url());
    kDebug() << url;

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
        const QString destUrl = KFileDialog::getSaveFileName(url.fileName(), QString(), view());
        if (destUrl.isEmpty()) return;
        KIO::Job *job = KIO::file_copy(url, KUrl(destUrl), -1, KIO::Overwrite);
        //job->setMetaData(metadata); //TODO: add metadata from request
        job->addMetaData("MaxCacheSize", "0"); // Don't store in http cache.
        job->addMetaData("cache", "cache"); // Use entry from cache if available.
        job->uiDelegate()->setAutoErrorHandlingEnabled(true);
    }
}

KWebPage *KWebPage::createWindow(WebWindowType type)
{
    if (WebKitSettings::self()->windowOpenPolicy(mainFrame()->url().host()) != WebKitSettings::KJSWindowOpenDeny)
        return 0;
    return newWindow(type);
}

KWebPage *KWebPage::newWindow(WebWindowType type)
{
    Q_UNUSED(type);
    return 0;
}

