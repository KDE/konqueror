/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2018 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "kiohandler.h"

#include <QMimeDatabase>
#include <QBuffer>

#include <KIO/StoredTransferJob>

using namespace WebEngine;

void KIOHandler::requestStarted(QWebEngineUrlRequestJob *req)
{
    m_queuedRequests << RequestJobPointer(req);
    processNextRequest();
}

KIOHandler::KIOHandler(QObject* parent):
    QWebEngineUrlSchemeHandler(parent)
{
    connect(this, &KIOHandler::ready, this, &KIOHandler::sendReply);
}

void KIOHandler::sendReply()
{
    if (m_currentRequest) {
        if (isSuccessful()) {
            if (m_redirectUrl.isValid()) {
                m_currentRequest->redirect(m_redirectUrl);
            } else {
                QBuffer *buf = new QBuffer;
                buf->open(QBuffer::ReadWrite);
                buf->write(m_data);
                buf->seek(0);
                connect(buf, &QIODevice::aboutToClose, buf, &QObject::deleteLater);
                m_currentRequest->reply(m_mimeType.name().toUtf8(), buf);
            }
        } else {
            m_currentRequest->fail(m_error);
        }
        m_currentRequest.clear();
    }
    processNextRequest();
}

void KIOHandler::processNextRequest()
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
    // QWebEngineUrlRequestJob seems to not support redirect & content at the same time
    job->setRedirectionHandlingEnabled(false);
    connect(job, &KIO::StoredTransferJob::result, this, [this, job](){kioJobFinished(job);});
}

void KIOHandler::embedderFinished(const QString& html)
{
    m_data = html.toUtf8();
    emit ready();
}

void KIOHandler::processSlaveOutput()
{
    emit ready();
}

void KIOHandler::createDataFromErrorString(KIO::StoredTransferJob* job)
{
    if (job->error() == KIO::ERR_WORKER_DEFINED && job->errorString().contains("<html>")) {
        m_data = job->data();
    } else if (job->error() != 0 && !job->errorString().isEmpty()) {
        QString html = QString("<html><body><h1>Error</h1>%1</body></html>").arg(job->errorString());
        m_data = html.toUtf8();
    }
}

void KIOHandler::kioJobFinished(KIO::StoredTransferJob* job)
{
    QMimeDatabase db;
    //If the job reported an error and job->errorString is not empty, don't report a failure but use the string
    //as reply. The error string reported by the job will be most likely more informative than the generic error
    //message produced by QtWebEngine.
    if (job->error() == 0) {
        m_error = QWebEngineUrlRequestJob::NoError;
        m_mimeType = db.mimeTypeForName(job->mimetype());
        m_data = job->data();
        m_redirectUrl = job->redirectUrl();
    } else {
        createDataFromErrorString(job);
        m_mimeType = db.mimeTypeForName("text/html");
        m_error = m_data.isEmpty() ? QWebEngineUrlRequestJob::RequestFailed : QWebEngineUrlRequestJob::NoError;
        m_redirectUrl.clear();
    }
    processSlaveOutput();
}
