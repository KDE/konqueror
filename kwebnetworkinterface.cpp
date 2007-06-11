/*
 * This file is part of the KDE project.
 *
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
#include "kwebnetworkinterface.h"

#include <kio/job.h>
#include <qdebug.h>

Q_DECLARE_METATYPE(QWebNetworkJob *);
Q_DECLARE_METATYPE(KJob *);

KWebNetworkInterface::KWebNetworkInterface(QObject *parent)
    : QWebNetworkInterface(parent)
{
}

void KWebNetworkInterface::addJob(QWebNetworkJob *job)
{
    KIO::Job *kioJob = 0;
    QByteArray postData = job->postData();
    if (postData.isEmpty())
        kioJob = KIO::get(job->url());
    else
        kioJob = KIO::http_post(job->url(), postData);

    kioJob->addMetaData(metaDataForRequest(job->request()));

    kioJob->setProperty("qwebnetworkjob", QVariant::fromValue(job));
    m_jobs.insert(job, kioJob);

    connect(kioJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
            this, SLOT(forwardJobData(KIO::Job *, const QByteArray &)));
    connect(kioJob, SIGNAL(result(KJob *)),
            this, SLOT(forwardJobResult(KJob *)));
}

void KWebNetworkInterface::cancelJob(QWebNetworkJob *job)
{
    KJob *kjob = m_jobs.take(job);
    if (kjob)
        kjob->kill();
}

KIO::MetaData KWebNetworkInterface::metaDataForRequest(QHttpRequestHeader request)
{
    KIO::MetaData metaData;

    metaData.insert("PropagateHttpHeader", "true");

    metaData.insert("UserAgent", request.value("User-Agent"));
    request.removeValue("User-Agent");

    metaData.insert("accept", request.value("Accept"));
    request.removeValue("Accept");

    request.removeValue("content-length");
    request.removeValue("Connection");

    metaData.insert("customHTTPHeader", request.toString());
    return metaData;
}

void KWebNetworkInterface::forwardJobData(KIO::Job *kioJob, const QByteArray &data)
{
    QWebNetworkJob *job = kioJob->property("qwebnetworkjob").value<QWebNetworkJob *>();
    if (!job)
        return;

    QString headers = kioJob->queryMetaData("HTTP-Headers");
    if (!headers.isEmpty()) {
        job->setResponse(QHttpResponseHeader(kioJob->queryMetaData("HTTP-Headers")));
        emit started(job);
    }

    emit this->data(job, data);
}

void KWebNetworkInterface::forwardJobResult(KJob *kjob)
{
    QWebNetworkJob *job = kjob->property("qwebnetworkjob").value<QWebNetworkJob *>();
    if (!job)
        return;
    m_jobs.remove(job);
    emit finished(job, kjob->error());
}

#include "kwebnetworkinterface.moc"

