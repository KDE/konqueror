/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2009 Dawit Alemayehu <adawit @ kde.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "networkaccessmanager.h"
#include "settings/webkitsettings.h"

#include <KDE/KDebug>
#include <KDE/KLocalizedString>
#include <KDE/KProtocolInfo>
#include <KDE/KRun>

#include <QTimer>
#include <QWidget>
#include <QNetworkReply>
#include <QWebFrame>
#include <QWebElementCollection>


#define QL1S(x) QLatin1String(x)
#define HIDABLE_ELEMENTS   QL1S("audio,img,embed,object,iframe,frame,video")

/* Null network reply */
class NullNetworkReply : public QNetworkReply
{
public:
    NullNetworkReply(const QNetworkRequest &req, QObject* parent = 0)
        :QNetworkReply(parent)
    {
        setRequest(req);
        setUrl(req.url());
        setHeader(QNetworkRequest::ContentLengthHeader, 0);
        setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
        setError(QNetworkReply::ContentAccessDenied, i18n("Blocked by ad filter"));
        setAttribute(QNetworkRequest::User, QNetworkReply::ContentAccessDenied);
        QTimer::singleShot(0, this, SIGNAL(finished()));
    }

    virtual void abort() {}
    virtual qint64 bytesAvailable() const { return 0; }

protected:
    virtual qint64 readData(char*, qint64) {return -1;}
};

namespace KDEPrivate {

MyNetworkAccessManager::MyNetworkAccessManager(QObject *parent)
                       : KIO::AccessManager(parent)
{
}

static bool blockRequest(QNetworkAccessManager::Operation op, const QUrl& requestUrl)
{
   if (op != QNetworkAccessManager::GetOperation)
       return false;

   if (!WebKitSettings::self()->isAdFilterEnabled())
       return false;

   if (!WebKitSettings::self()->isAdFiltered(requestUrl.toString()))
       return false;

   kDebug() << "*** REQUEST BLOCKED: URL" << requestUrl << "RULE" << WebKitSettings::self()->adFilteredBy(requestUrl.toString());
   return true;
}

QNetworkReply *MyNetworkAccessManager::createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
{
    if (!blockRequest(op, req.url())) {
        if (KProtocolInfo::isHelperProtocol(req.url())) {
            (void) new KRun(req.url(), qobject_cast<QWidget*>(req.originatingObject()));
            return new NullNetworkReply(req, this);
        }
        return KIO::AccessManager::createRequest(op, req, outgoingData);
    }

    QWebFrame* frame = qobject_cast<QWebFrame*>(req.originatingObject());
    if (frame) {
        if (!m_blockedRequests.contains(frame))
            connect(frame, SIGNAL(loadFinished(bool)), this, SLOT(slotFinished(bool)));
        m_blockedRequests.insert(frame, req.url());
    }

    return new NullNetworkReply(req, this);
}

static void hideBlockedElements(const QUrl& url, QWebElementCollection& collection)
{
    for (QWebElementCollection::iterator it = collection.begin(); it != collection.end(); ++it) {
        const QUrl baseUrl ((*it).webFrame()->baseUrl());
        QString src = (*it).attribute(QL1S("src"));
        if (src.isEmpty())
            src = (*it).evaluateJavaScript(QL1S("this.src")).toString();
        if (src.isEmpty())
            continue;
        const QUrl resolvedUrl (baseUrl.resolved(src));
        if (url == resolvedUrl) {
            //kDebug() << "*** HIDING ELEMENT: " << (*it).tagName() << resolvedUrl;
            (*it).removeFromDocument();
        }
    }
}

void MyNetworkAccessManager::slotFinished(bool ok)
{
    if (!ok)
        return;

    if(!WebKitSettings::self()->isAdFilterEnabled())
        return;

    if(!WebKitSettings::self()->isHideAdsEnabled())
        return;

    QWebFrame* frame = qobject_cast<QWebFrame*>(sender());
    if (!frame)
        return;

    QList<QUrl> urls = m_blockedRequests.values(frame);
    if (urls.isEmpty())
        return;

   QWebElementCollection collection = frame->findAllElements(HIDABLE_ELEMENTS);
   if (frame->parentFrame())
        collection += frame->parentFrame()->findAllElements(HIDABLE_ELEMENTS);

    Q_FOREACH(const QUrl& url, urls)
        hideBlockedElements(url, collection);
}

}

#include "networkaccessmanager.moc"
