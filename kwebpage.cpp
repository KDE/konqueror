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
#include "network/knetworkaccessmanager.h"

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

#include <QWebFrame>
#include <QtNetwork/QNetworkReply>

class KWebPage::KWebPagePrivate
{
public:
    KWebPagePrivate() {}
};

KWebPage::KWebPage(QObject *parent)
    : QWebPage(parent), d(new KWebPage::KWebPagePrivate())
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

    settings()->setWebGraphic(QWebSettings::MissingPluginGraphic, KIcon("preferences-plugin").pixmap(32, 32));
    settings()->setWebGraphic(QWebSettings::MissingImageGraphic, KIcon("image-missing").pixmap(32, 32));
    settings()->setWebGraphic(QWebSettings::DefaultFrameIconGraphic, KIcon("applications-internet").pixmap(32, 32));

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

void KWebPage::slotHandleUnsupportedContent(QNetworkReply *reply)
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

QObject *KWebPage::createPlugin(const QString &classid, const QUrl &url, const QStringList &paramNames, const QStringList &paramValues)
{
    kDebug() << "create Plugin requested:";
    kDebug() << "classid:" << classid;
    kDebug() << "url:" << url;
    kDebug() << "paramNames:" << paramNames << " paramValues:" << paramValues;
    return 0;
}

void KWebPage::slotDownloadRequested(const QNetworkRequest &request)
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
