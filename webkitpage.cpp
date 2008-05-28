/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Dirk Mueller <mueller@kde.org>
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

#include "webkitpage.h"
#include "webkitpart.h"

//Use default khtml settings (not necessary to duplicate it)
#include <khtmldefaults.h>

#include <KDE/KParts/GenericFactory>
#include <KDE/KAboutData>
#include <KDE/KFileDialog>
#include <KDE/KInputDialog>
#include <KDE/KMessageBox>
#include <KDE/KProtocolManager>
#include <KDE/KGlobalSettings>

#include <QWebFrame>
#include <QtNetwork/QNetworkReply>

#include "knetworkaccessmanager.h"

WebPage::WebPage(WebKitPart *wpart, QWidget *parent)
    : QWebPage(parent), m_part(wpart)
{
#if 0
    connect(this, SIGNAL(unsupportedContent(QNetworkReply *)),
            this, SLOT(slotHandleUnsupportedContent(QNetworkReply *)));
    setForwardUnsupportedContent(true);
#endif
    setNetworkAccessManager(new KNetworkAccessManager(this));
}

bool WebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request,
                                      NavigationType type)
{
    kDebug() << "acceptNavigationRequest";
    return true;
}

QString WebPage::chooseFile(QWebFrame *parentFrame, const QString &suggestedFile)
{
    return KFileDialog::getOpenFileName(suggestedFile);
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
    *result = KInputDialog::getText(i18n("JavaScript"), msg, defaultValue, &ok);
    return ok;
}

QString WebPage::userAgentForUrl(const QUrl& _url) const
{
    KUrl url(_url);
    QString host = url.isLocalFile() ? "localhost" : url.host();
    QString userAgent = KProtocolManager::userAgentForHost(host);
    if (userAgent != KProtocolManager::userAgentForHost(QString())) {
        return userAgent;
    }
    return QWebPage::userAgentForUrl(url);
}

void WebPage::slotHandleUnsupportedContent(QNetworkReply *reply)
{
    //TODO
    kDebug() << " title :" << reply->url().toString();
    kDebug() << " error :" << reply->errorString();

}

QObject *WebPage::createPlugin(const QString &classid, const QUrl &url, const QStringList &paramNames, const QStringList &paramValues)
{
    kDebug() << " create Plugin requested :";
    kDebug() << " classid :" << classid;
    kDebug() << " url :" << url;
    kDebug() << "paramNames :" << paramNames << " paramValues ;" << paramValues;
    return 0;
}
