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

#include <kdebug.h>

#include <QtCore/QTimer>
#include <QtNetwork/QNetworkReply>

/* Null network reply */
class NullNetworkReply : public QNetworkReply
{
public:
    NullNetworkReply(const QNetworkRequest &req) {
        setRequest(req);
        setUrl(req.url());
        setHeader(QNetworkRequest::ContentLengthHeader, 0);
        setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
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
                       :AccessManagerBase(parent)
{
}

QNetworkReply *MyNetworkAccessManager::createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
{
      if (WebKitSettings::self()->isAdFilterEnabled() && WebKitSettings::self()->isAdFiltered(req.url().toString())) {
          kDebug() << "*** BLOCKED UNAUTHORIZED REQUEST => " << req.url();
          return new NullNetworkReply(req);
      }

      return AccessManagerBase::createRequest(op, req, outgoingData);
}
}
