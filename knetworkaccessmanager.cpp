/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
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

#include <QNetworkRequest>

#include <KDebug>

KNetworkAccessManager::KNetworkAccessManager(QObject *parent)
    : QNetworkAccessManager(parent)
{
}

QNetworkReply *KNetworkAccessManager::createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
{
    switch (op) {
        case HeadOperation: {
            kDebug() << "HeadOperation:" << req.url();
            break;
        }
        case GetOperation: {
            kDebug() << "GetOperation:" << req.url();
            break;
        }
        case PutOperation: {
            kDebug() << "PutOperation:" << req.url();
            break;
        }
        case PostOperation: {
            kDebug() << "PostOperation:" << req.url();
            break;
        }
        default:
            kDebug() << "Unknown operation";
    }
    return QNetworkAccessManager::createRequest(op, req, outgoingData);
}

#include "knetworkaccessmanager.moc"
