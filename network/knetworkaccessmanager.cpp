/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2007 Trolltech ASA
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

#include "knetworkaccessmanager.h"

#include "network/knetworkreply.h"

#include <QNetworkRequest>
#include <QNetworkReply>

#include <KDebug>
#include <kio/job.h>

Q_DECLARE_METATYPE(KNetworkReply *);

KNetworkAccessManager::KNetworkAccessManager(QObject *parent)
    : QNetworkAccessManager(parent)
{
}

QNetworkReply *KNetworkAccessManager::createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
{
    return QNetworkAccessManager::createRequest(op, req, outgoingData); //TODO: remove

    KIO::Job *kioJob = 0;

    KNetworkReply *reply = new KNetworkReply(req, kioJob, this);

    switch (op) {
        case HeadOperation: {
            kDebug() << "HeadOperation:" << req.url();
            break;
        }
        case GetOperation: {
            kDebug() << "GetOperation:" << req.url();

            kioJob = KIO::get(req.url(), KIO::NoReload, KIO::HideProgressInfo);

            break;
        }
        case PutOperation: {
            kDebug() << "PutOperation:" << req.url();
            break;
        }
        case PostOperation: {
            kDebug() << "PostOperation:" << req.url();

            kioJob = KIO::http_post(req.url(), outgoingData->readAll(), KIO::HideProgressInfo);

            break;
        }
        default:
            kDebug() << "Unknown operation";
            return 0;
    }

    kioJob->setProperty("KNetworkReply", QVariant::fromValue(reply));

    connect(kioJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
        this, SLOT(forwardJobData(KIO::Job *, const QByteArray &)));

    return reply;
}

void KNetworkAccessManager::forwardJobData(KIO::Job *kioJob, const QByteArray &data)
{
    kDebug();
    KNetworkReply *job = kioJob->property("KNetworkReply").value<KNetworkReply *>();
    if (!job)
        return;

    job->write(data);
}

#include "knetworkaccessmanager.moc"
