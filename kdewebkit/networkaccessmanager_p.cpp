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

#include "kwebpage.h"
#include "networkaccessmanager_p.h"

#include <kdebug.h>
#include <kio/accessmanager.h>

#include <QtCore/QTimer>
#include <QtCore/QPointer>
#include <QtNetwork/QNetworkReply>

/* Null network reply */
class NullNetworkReply : public QNetworkReply
{
public:
    NullNetworkReply() {
        setHeader(QNetworkRequest::ContentLengthHeader, 0);
        setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
        QTimer::singleShot(0, this, SIGNAL(finished()));
    }
    virtual void abort() {}
    virtual qint64 bytesAvailable() const {
        return 0;
    }
protected:
    virtual qint64 readData(char* data, qint64) {
        qMemCopy(data, "\0", 1); return 0;
    }
};

namespace KDEPrivate {

class NetworkAccessManager::NetworkAccessManagerPrivate
{
 public:
    QPointer<KWebPage> page;

    KIO::MetaData requestMetaData;
    KIO::MetaData sessionMetaData;
};

NetworkAccessManager::NetworkAccessManager(QObject *parent)
                                 :KIO::AccessManager(parent),
                                  d(new KDEPrivate::NetworkAccessManager::NetworkAccessManagerPrivate)
{
    d->page = qobject_cast<KWebPage*>(parent);
}

KIO::MetaData& NetworkAccessManager::requestMetaData()
{
    return d->requestMetaData;
}

KIO::MetaData& NetworkAccessManager::sessionMetaData()
{
    return d->sessionMetaData;
}

QNetworkReply *NetworkAccessManager::createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
{
    if (d->page && !d->page->authorizedRequest(req.url())) {
        kDebug() << "*** BLOCKED UNAUTHORIZED REQUEST => " << req.url();
        return new NullNetworkReply();
    }

    QNetworkRequest request(req);
    KIO::MetaData metaData;

    const QNetworkRequest::Attribute attr = static_cast<QNetworkRequest::Attribute>(KIO::AccessManager::MetaData);
    const QVariant value = req.attribute(attr);

    // Preserve the meta-data already set, if any...
    if (value.isValid() && value.type() == QVariant::Map)
        metaData += value.toMap();

    // Add the request meta data if any...
    if (!d->requestMetaData.isEmpty())
        metaData += d->requestMetaData;

    // Add the session meta data if any...
    if (!d->sessionMetaData.isEmpty())
        metaData += d->sessionMetaData;

    // Set the meta data to be sent off to
    if (!metaData.isEmpty())
        request.setAttribute(attr, metaData.toVariant());

    // Clear the per request meta data...
    d->requestMetaData.clear();

    return KIO::AccessManager::createRequest(op, request, outgoingData);
}
}

#include "networkaccessmanager_p.moc"
