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
#ifndef NETWORKACCESSMANAGER_H
#define NETWORKACCESSMANAGER_H

#include <KDE/KIO/AccessManager>

#include <QMultiHash>

class QWebFrame;

namespace KDEPrivate {

 /**
  * Re-implemented for internal reasons. API remains unaffected.
  */
class MyNetworkAccessManager : public KIO::AccessManager
{
    Q_OBJECT

public:
    MyNetworkAccessManager(QObject *parent = 0);

protected:
    virtual QNetworkReply *createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData = 0);

private Q_SLOTS:
    void slotFinished(bool);

private:
    QMultiHash<QWebFrame*, QUrl> m_blockedRequests;
};

}

#endif // NETWORKACCESSMANAGER_P_H
