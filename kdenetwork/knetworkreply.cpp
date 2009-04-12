/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Alex Merry <alex.merry @ kdemail.net>
 * Copyright (C) 2008 - 2009 Urs Wolfer <uwolfer @ kde.org>
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
#include <KLocale>
#include <QSslConfiguration>

#include <QTimer>

class KNetworkReply::KNetworkReplyPrivate
{
public:
    KNetworkReplyPrivate(KNetworkReply *qq)
    : q(qq)
    , m_kioJob(0)
    , m_data()
    , m_metaDataRead(false)
    {}

    void _k_redirection(KIO::Job *job, const KUrl &url);
    void _k_percent(KJob *job, unsigned long percent);
    void _k_permanentRedirection(KIO::Job *job, const KUrl &fromUrl, const KUrl &toUrl);

    KNetworkReply *q;

    KIO::Job *m_kioJob;
    QByteArray m_data;
    bool m_metaDataRead;
};

KNetworkReply::KNetworkReply(const QNetworkAccessManager::Operation &op, const QNetworkRequest &request, KIO::Job *kioJob, QObject *parent)
    : QNetworkReply(parent), d(new KNetworkReply::KNetworkReplyPrivate(this))

{
    d->m_kioJob = kioJob;
    setRequest(request);
    setOpenMode(QIODevice::ReadOnly);
    setUrl(request.url());
    setOperation(op);

    if (!request.sslConfiguration().isNull()) {
        setSslConfiguration(request.sslConfiguration());
        kDebug() << "QSslConfiguration not supported (currently).";
    }

    if (!kioJob) { // a blocked request
        setError(QNetworkReply::OperationCanceledError, i18n("Blocked request."));
        QTimer::singleShot(0, this, SIGNAL(finished()));
    } else {
        connect(kioJob, SIGNAL(redirection(KIO::Job*, const KUrl&)), SLOT(_k_redirection(KIO::Job*, const KUrl&)));
        connect(kioJob, SIGNAL(permanentRedirection(KIO::Job*, const KUrl&, const KUrl&)),
            SLOT(_k_permanentRedirection(KIO::Job*, const KUrl&, const KUrl&)));
        connect(kioJob, SIGNAL(percent(KJob*, unsigned long)), SLOT(_k_percent(KJob*, unsigned long)));
        connect(kioJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
            SLOT(appendData(KIO::Job *, const QByteArray &)));
        connect(kioJob, SIGNAL(result(KJob *)), SLOT(jobDone(KJob *)));
        connect(kioJob, SIGNAL(mimetype(KIO::Job *, const QString&)),
            SLOT(setMimeType(KIO::Job *, const QString&)));
    }
}

KNetworkReply::~KNetworkReply()
{
    delete d;
}

void KNetworkReply::abort()
{
    if (d->m_kioJob) {
        d->m_kioJob->kill();
        d->m_kioJob->deleteLater();
    }
}

qint64 KNetworkReply::bytesAvailable() const
{
    return QNetworkReply::bytesAvailable() + d->m_data.length();
}

qint64 KNetworkReply::readData(char *data, qint64 maxSize)
{
//     kDebug();
    const qint64 length = qMin(qint64(d->m_data.length()), maxSize);
    if (length) {
        qMemCopy(data, d->m_data.constData(), length);
        d->m_data.remove(0, length);
    }

    return length;
}

void KNetworkReply::appendData(KIO::Job *kioJob, const QByteArray &data)
{
//     kDebug();

    if (!d->m_metaDataRead) {
        const QString responseCode = kioJob->queryMetaData("responsecode");
        if (!responseCode.isEmpty()) 
            setAttribute(QNetworkRequest::HttpStatusCodeAttribute, responseCode.toInt());

        const QString headers = kioJob->queryMetaData("HTTP-Headers");
        if (!headers.isEmpty()) {
            QStringList headerList = headers.split('\n');
            Q_FOREACH(const QString &header, headerList) {
                const QStringList headerPair = header.split(": ");
                if (headerPair.size() == 2) {
//                     kDebug() << headerPair.at(0) << headerPair.at(1);
                    setRawHeader(headerPair.at(0).toUtf8(), headerPair.at(1).toUtf8());
                }
            }
        }
        d->m_metaDataRead = true;
    }

    d->m_data += data;
    emit readyRead();
}

void KNetworkReply::setMimeType(KIO::Job *kioJob, const QString &mimeType)
{
    Q_UNUSED(kioJob);

//     kDebug() << mimeType;
    setHeader(QNetworkRequest::ContentTypeHeader, mimeType.toUtf8());
}

void KNetworkReply::jobDone(KJob *kJob)
{
    switch (kJob->error())
    {
        case 0:
            setError(QNetworkReply::NoError, errorString());
            kDebug() << "0 -> QNetworkReply::NoError";
            break;
        case KIO::ERR_COULD_NOT_CONNECT:
            setError(QNetworkReply::ConnectionRefusedError, errorString());
            kDebug() << "KIO::ERR_COULD_NOT_CONNECT -> KIO::ERR_COULD_NOT_CONNECT";
            break;
        case KIO::ERR_UNKNOWN_HOST:
            setError(QNetworkReply::HostNotFoundError, errorString());
            kDebug() << "KIO::ERR_UNKNOWN_HOST -> QNetworkReply::HostNotFoundError";
            break;
        case KIO::ERR_SERVER_TIMEOUT:
            setError(QNetworkReply::TimeoutError, errorString());
            kDebug() << "KIO::ERR_SERVER_TIMEOUT -> QNetworkReply::TimeoutError";
            break;
        case KIO::ERR_USER_CANCELED:
        case KIO::ERR_ABORTED:
            setError(QNetworkReply::OperationCanceledError, errorString());
            kDebug() << "KIO::ERR_ABORTED -> QNetworkReply::OperationCanceledError";
            break;
        case KIO::ERR_UNKNOWN_PROXY_HOST:
            setError(QNetworkReply::ProxyNotFoundError, errorString());
            kDebug() << "KIO::UNKNOWN_PROXY_HOST -> QNetworkReply::ProxyNotFoundError";
            break;
        case KIO::ERR_ACCESS_DENIED:
            setError(QNetworkReply::ContentAccessDenied, errorString());
            kDebug() << "KIO::ERR_ACCESS_DENIED -> QNetworkReply::ContentAccessDenied";
            break;
        case KIO::ERR_WRITE_ACCESS_DENIED:
            setError(QNetworkReply::ContentOperationNotPermittedError, errorString());
            kDebug() << "KIO::ERR_WRITE_ACCESS_DENIED -> QNetworkReply::ContentOperationNotPermittedError";
            break;
        case KIO::ERR_NO_CONTENT:
        case KIO::ERR_DOES_NOT_EXIST:
            setError(QNetworkReply::ContentNotFoundError, errorString());
            kDebug() << "KIO::ERR_DOES_NOT_EXIST -> QNetworkReply::ContentNotFoundError";
            break;
        case KIO::ERR_COULD_NOT_AUTHENTICATE:
            setError(QNetworkReply::AuthenticationRequiredError, errorString());
            kDebug() << kJob->error();
            break;
        case KIO::ERR_UNSUPPORTED_PROTOCOL:
        case KIO::ERR_NO_SOURCE_PROTOCOL:
            setError(QNetworkReply::ProtocolUnknownError, errorString());
            kDebug() << kJob->error();
            break;
        case KIO::ERR_UNSUPPORTED_ACTION:
            setError(QNetworkReply::ProtocolInvalidOperationError, errorString());
            kDebug() << kJob->error();
            break;
        default:
            setError(QNetworkReply::UnknownNetworkError, errorString());
            kDebug() << kJob->error();
    }

    emit finished();
}

void KNetworkReply::KNetworkReplyPrivate::_k_redirection(KIO::Job* job, const KUrl& url)
{
    job->kill();
    m_kioJob = 0;
    q->setAttribute(QNetworkRequest::RedirectionTargetAttribute, QUrl(url));
    emit q->finished();
}

void KNetworkReply::KNetworkReplyPrivate::_k_percent(KJob *job, unsigned long percent)
{
    qulonglong kiloBytes = job->totalAmount(KJob::Bytes) / 2046;
    emit q->downloadProgress(kiloBytes / ((double)percent / 100), kiloBytes);
}

void KNetworkReply::KNetworkReplyPrivate::_k_permanentRedirection(KIO::Job *job, const KUrl &fromUrl, const KUrl &toUrl)
{
    Q_UNUSED(fromUrl);
    _k_redirection(job, toUrl);
}

#include "knetworkreply.moc"
