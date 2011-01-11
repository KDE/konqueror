/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2009 Dawit Alemayehu <adawit @ kde.org>
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

#include "networkaccessmanager.h"
#include "settings/webkitsettings.h"

#include <KDE/KDebug>
#include <KDE/KLocalizedString>

#include <QtCore/QTimer>
#include <QtNetwork/QNetworkReply>
#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebElementCollection>


#define QL1S(x) QLatin1String(x)

/* Null network reply */
class NullNetworkReply : public QNetworkReply
{
public:
    NullNetworkReply(const QNetworkRequest &req) {
        setRequest(req);
        setUrl(req.url());
        setHeader(QNetworkRequest::ContentLengthHeader, 0);
        setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
        setError(QNetworkReply::ContentAccessDenied, i18n("Blocked by ad filter"));
        setAttribute(QNetworkRequest::User, QNetworkReply::ContentAccessDenied);
        QTimer::singleShot(0, this, SIGNAL(finished()));
    }
    virtual void abort() {}
    virtual qint64 bytesAvailable() const {
        return 0;
    }
protected:
    virtual qint64 readData(char*, qint64) {
        return -1;
    }
};

namespace KDEPrivate {

MyNetworkAccessManager::MyNetworkAccessManager(QObject *parent)
                       : KIO::AccessManager(parent)
{
    connect (this, SIGNAL(finished(QNetworkReply*)), this, SLOT(slotFinished(QNetworkReply*)));
}

static bool blockRequest(QNetworkAccessManager::Operation op, const QUrl& requestUrl)
{
   if (op != QNetworkAccessManager::GetOperation)
       return false;
   if (!WebKitSettings::self()->isAdFilterEnabled())
       return false;
   if (!WebKitSettings::self()->isAdFiltered(requestUrl.toString()))
       return false;
   return true;
}

QNetworkReply *MyNetworkAccessManager::createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
{
    if (!blockRequest(op, req.url()))
        return KIO::AccessManager::createRequest(op, req, outgoingData);

    //kDebug() << "*** BLOCKED UNAUTHORIZED REQUEST => " << req.url();
    m_blockedUrls.append(req.url());
    return new NullNetworkReply(req);
}

static void hideBlockableElements(const QUrl& url, QWebElementCollection& collection)
{
    for (QWebElementCollection::iterator it = collection.begin(); it != collection.end(); ++it) {
        const QUrl resolvedUrl((*it).webFrame()->baseUrl().resolved((*it).attribute(QL1S("src"))));
        if (url != resolvedUrl)
	   continue;
        //kDebug() << "*** HIDING ELEMENT: " << (*it).tagName() << (*it).attribute(QL1S("src"));
        (*it).removeFromDocument();
    }
}

void MyNetworkAccessManager::slotFinished(QNetworkReply* reply)
{
    if (!reply)
        return;

    if(!WebKitSettings::self()->isHideAdsEnabled())
        return;

    if (!m_blockedUrls.contains(reply->url()))
        return;

    QWebFrame* frame = qobject_cast<QWebFrame*>(reply->request().originatingObject());
    if (!frame)
        return;

    // Always use the parent frame to check for blockable elements because
    // the originating object itself might be a blocked iframe!
    while (frame->parentFrame())
        frame = frame->parentFrame();
    
    QWebElementCollection collection = frame->findAllElements(QL1S("img[src],embed[src],object[src],iframe[src]"));
    hideBlockableElements(reply->url(), collection);
}

}

#include "networkaccessmanager.moc"
