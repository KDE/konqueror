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
            m_currentRequest->fail(QWebEngineUrlRequestJob::UrlInvalid);
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

void WebEnginePartKIOHandler::kioJobFinished(KIO::StoredTransferJob* job)
{
    //Try to get information from the job even in case it reports errors, so that we can avoid using QWebEngineUrlRequestJob::fail.
    //The reason is that calling fail displays a generic error message provided by QtWebEngine. Using QWebEngineUrlRequestJob::fail can
    //be avoided in two situations:
    //- job->errorString() is not empty
    //- job->data() is not empty and the mimetype is valid
    //In the first case, the error string will be wrapped inside a minimal html page and it'll be used as reply. If the error code is
    //KIO::ERR_SLAVE_DEFINED, the string can be rich text: in this case, it won't be wrapped.
    //In the second case we use job->data() as reply, even if it may contain incomplete data. The reason is that some ioslaves (for example
    //the `man` ioslave, since the d5f2beb2c13c8b3a202b4979027ad5430f007705 commit) report the error message inside job->data() instead of
    //job->errorString().
    m_error = QWebEngineUrlRequestJob::NoError;
    QMimeDatabase db;
    m_mimeType = db.mimeTypeForName(job->mimetype());
    m_data = job->data();

    if (job->error() != 0) {
        m_mimeType = db.mimeTypeForName("text/html");
        if (!job->errorString().isEmpty()) {
            if (job->error() == KIO::ERR_SLAVE_DEFINED && job->errorString().contains("<html>")) {
                m_data = job->errorString().toUtf8();
            } else {
                QString html = QString("<html><body><h1>Error</h1>%1</body></html>").arg(job->errorString());
                m_data = html.toUtf8();
            }
        } else if (m_data.isEmpty() || !m_mimeType.isValid()) {
            m_error = QWebEngineUrlRequestJob::RequestFailed;
        }
    }
    processSlaveOutput();
}
