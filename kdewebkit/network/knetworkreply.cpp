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

#include "knetworkreply.h"

#include <KDebug>
#include <KIO/Job>

#include <QTimer>

KNetworkReply::KNetworkReply(const QNetworkRequest &request, KIO::Job *kioJob, QObject *parent)
    : QNetworkReply(parent)
    , m_kioJob(kioJob)
    , m_metaDataRead(false)
{
    setRequest(request);
    setOpenMode(QIODevice::ReadOnly);

    if (!kioJob) { // a blocked request
        QTimer::singleShot(0, this, SIGNAL(finished()));
    }
}

void KNetworkReply::abort()
{
    if (m_kioJob) {
        m_kioJob->kill();
        m_kioJob->deleteLater();
    }
}

qint64 KNetworkReply::bytesAvailable() const
{
    return QNetworkReply::bytesAvailable() + m_data.length();
}

qint64 KNetworkReply::readData(char *data, qint64 maxSize)
{
//     kDebug();
    qint64 length = qMin(qint64(m_data.length()), maxSize);
    if (length) {
        qMemCopy(data, m_data.constData(), length);
        m_data.remove(0, length);
    }

    return length;
}

void KNetworkReply::appendData(KIO::Job *kioJob, const QByteArray &data)
{
//     kDebug();

    if (!m_metaDataRead) {
        QString headers = kioJob->queryMetaData("HTTP-Headers");
        if (!headers.isEmpty()) {
            QStringList headerList = headers.split('\n');
            Q_FOREACH(const QString &header, headerList) {
                QStringList headerPair = header.split(": ");
                if (headerPair.size() == 2) {
//                     kDebug() << headerPair.at(0) << headerPair.at(1);
                    setRawHeader(headerPair.at(0).toUtf8(), headerPair.at(1).toUtf8());
                }
            }
        }
        m_metaDataRead = true;
    }

    m_data += data;
    emit readyRead();
}

void KNetworkReply::setMimeType(KIO::Job *kioJob, const QString &mimeType)
{
    Q_UNUSED(kioJob);

    kDebug() << mimeType;
    setHeader(QNetworkRequest::ContentTypeHeader, mimeType.toUtf8());
}

#include "knetworkreply.moc"
