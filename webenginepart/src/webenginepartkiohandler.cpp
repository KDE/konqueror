/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2018 Stefano Crocco <stefano.crocco@alice.it>
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

#include "webenginepartkiohandler.h"

#include <QMimeDatabase>
#include <QBuffer>
#include <QtWebEngine/QtWebEngineVersion>

#include <KIO/StoredTransferJob>

void WebEnginePartKIOHandler::requestStarted(QWebEngineUrlRequestJob *req)
{
    m_queuedRequests << RequestJobPointer(req);
    processNextRequest();
}

WebEnginePartKIOHandler::WebEnginePartKIOHandler(QObject* parent):
    QWebEngineUrlSchemeHandler(parent)
{
    connect(this, &WebEnginePartKIOHandler::ready, this, &WebEnginePartKIOHandler::sendReply);
}

void WebEnginePartKIOHandler::sendReply()
{
    if (m_currentRequest) {
        if (isSuccessful()) {
            QBuffer *buf = new QBuffer;
            buf->open(QBuffer::ReadWrite);
            buf->write(m_data);
            buf->seek(0);
            connect(buf, &QIODevice::aboutToClose, buf, &QObject::deleteLater); 
            m_currentRequest->reply(m_mimeType.name().toUtf8(), buf);
        } else {
            m_currentRequest->fail(m_error);
        }
        m_currentRequest.clear();
    }
    processNextRequest();
}

void WebEnginePartKIOHandler::processNextRequest()
{
    if (m_currentRequest) {
        return;
    }
    
    while (!m_currentRequest && !m_queuedRequests.isEmpty()) {
        m_currentRequest = m_queuedRequests.takeFirst();
    }
    
    if (!m_currentRequest) {
        return;
    }
    KIO::StoredTransferJob *job =  KIO::storedGet(m_currentRequest ->requestUrl(), KIO::NoReload, KIO::HideProgressInfo);
    connect(job, &KIO::StoredTransferJob::result, this, [this, job](){kioJobFinished(job);});
}

void WebEnginePartKIOHandler::embedderFinished(const QString& html)
{
    m_data = html.toUtf8();
    emit ready();
}

void WebEnginePartKIOHandler::processSlaveOutput()
{
    emit ready();
}

void WebEnginePartKIOHandler::createDataFromErrorString(KIO::StoredTransferJob* job)
{
    if (job->error() == KIO::ERR_SLAVE_DEFINED && job->errorString().contains("<html>")) {
        m_data = job->data();
    } else if (job->error() != 0 && !job->errorString().isEmpty()) {
        QString html = QString("<html><body><h1>Error</h1>%1</body></html>").arg(job->errorString());
        m_data = html.toUtf8();
    }
}

void WebEnginePartKIOHandler::kioJobFinished(KIO::StoredTransferJob* job)
{
    QMimeDatabase db;
    //If the job reported an error and job->errorString is not empty, don't report a failure but use the string
    //as reply. The error string reported by the job will be most likely more informative than the generic error
    //message produced by QtWebEngine.
    if (job->error() == 0) {
        m_error = QWebEngineUrlRequestJob::NoError;
        m_mimeType = db.mimeTypeForName(job->mimetype());
        m_data = job->data();
    } else {
        createDataFromErrorString(job);
        m_mimeType = db.mimeTypeForName("text/html");
        m_error = m_data.isEmpty() ? QWebEngineUrlRequestJob::RequestFailed : QWebEngineUrlRequestJob::NoError;
    }
    processSlaveOutput();
}
