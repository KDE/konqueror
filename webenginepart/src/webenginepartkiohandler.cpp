/*
 *  This file is part of the KDE project
 *    Copyright (C) 2018 Stefano Crocco <stefano.crocco@alice.it>
 *
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License as
 *    published by the Free Software Foundation; either version 2 of
 *    the License or (at your option) version 3 or any later version
 *    accepted by the membership of KDE e.V. (or its successor approved
 *    by the membership of KDE e.V.), which shall act as a proxy
 *    defined in Section 14 of version 3 of the license.
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Library General Public License for more details.
 *
 *    You should have received a copy of the GNU Library General Public License
 *    along with this library; see the file COPYING.LIB.  If not, write to
 *    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *    Boston, MA 02110-1301, USA.
 * 
*/

#include "webenginepartkiohandler.h"
#include "webengineparthtmlembedder.h"

#include <QMimeDatabase>
#include <QWebEngineUrlRequestJob>
#include <QDebug>
#include <QBuffer>

#include <KIO/StoredTransferJob>

void WebEnginePartKIOHandler::requestStarted(QWebEngineUrlRequestJob *req)
{
    m_queuedRequests << RequestJobPointer(req);
    processNextRequest();
}

WebEnginePartKIOHandler::WebEnginePartKIOHandler(QObject* parent):
    QWebEngineUrlSchemeHandler(parent), m_embedder(nullptr)
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
    if (m_mimeType.inherits("text/html") || m_mimeType.inherits("application/xhtml+xml")) {
        htmlEmbedder()->startEmbedding(m_data, m_mimeType.name());
    } else {
        emit ready();
    }
}

void WebEnginePartKIOHandler::kioJobFinished(KIO::StoredTransferJob* job)
{
    m_error = job->error() == 0 ? QWebEngineUrlRequestJob::NoError : QWebEngineUrlRequestJob::RequestFailed;
    m_errorMessage = isSuccessful() ? job->errorString() : QString();
    m_data = job->data();
    m_mimeType = QMimeDatabase().mimeTypeForData(m_data);
    processSlaveOutput();
}

WebEnginePartHtmlEmbedder * WebEnginePartKIOHandler::htmlEmbedder()
{
    if (!m_embedder) {
        m_embedder = new WebEnginePartHtmlEmbedder(this);
        connect(htmlEmbedder(), &WebEnginePartHtmlEmbedder::finished, this, &WebEnginePartKIOHandler::embedderFinished);
    }
    return m_embedder;
}
