/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2018 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "errorschemehandler.h"

#include <QBuffer>
#include <QByteArray>
#include <QWebEngineUrlRequestJob>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDataStream>
#include <QApplication>
#include <QLocale>
#include <QMimeType>
#include <QMimeDatabase>

#include <KIO/Job>
#include <KIconLoader>
#include <KLocalizedString>

#include "utils.h"

using namespace WebEngine;

ErrorSchemeHandler::ErrorSchemeHandler(QObject* parent):
    QWebEngineUrlSchemeHandler(parent), m_warningIconData(readWarningIconData())
{
}

QString ErrorSchemeHandler::readWarningIconData() const
{
    QString data;
    QString path = KIconLoader::global()->iconPath("dialog-warning", -KIconLoader::SizeHuge);
    if (path.isEmpty()) {
        return data;
    }
    QFile f(path);
    if (f.open(QIODevice::ReadOnly)) {
        QMimeDatabase db;
        QMimeType mime = db.mimeTypeForFile(f.fileName());
        data += QL1S("data:");
        data += mime.isValid() ? mime.name() : "application/octet-stream";
        data += QL1S(";base64,");
        data += f.readAll().toBase64();
    }
    return data;

}

void ErrorSchemeHandler::requestStarted(QWebEngineUrlRequestJob* job)
{
    QBuffer *buf = new QBuffer;
    buf->open(QBuffer::ReadWrite);
    connect(buf, &QIODevice::aboutToClose, buf, &QObject::deleteLater);
    ErrorInfo ei = parseErrorUrl(job->requestUrl());
    writeErrorPage(buf, ei);
    buf->seek(0);
    job->reply("text/html", buf);
}

ErrorSchemeHandler::ErrorInfo ErrorSchemeHandler::parseErrorUrl(const QUrl& url)
{
    ErrorInfo ei;
    ei.requestUrl = QUrl(url.fragment());
    if (ei.requestUrl.isValid()) {
        QString const query = url.query(QUrl::FullyDecoded);
        QRegularExpression const pattern("error=(\\d+)&errText=(.*)");
        QRegularExpressionMatch const match = pattern.match(query);
        int const error = match.captured(1).toInt();
        // error=0 isn't a valid error code, so 0 means it's missing from the URL
        if (error != 0) {
                ei.code = error;
        }
        ei.text = match.captured(2);
    }
    return ei;
}

void ErrorSchemeHandler::writeErrorPage(QBuffer* buf, const ErrorSchemeHandler::ErrorInfo& info)
{
    QString errorName, techName, description;
    QStringList causes, solutions;
    
    QByteArray raw = KIO::rawErrorDetail(info.code, info.text, &info.requestUrl);
    QDataStream stream(raw);

    stream >> errorName >> techName >> description >> causes >> solutions; 

    QFile file(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QL1S("webenginepart/error.html")));
    if (!file.open(QIODevice::ReadOnly)) {
        buf->write(i18n("<html><body><h3>Unable to display error message</h3>"
                    "<p>The error template file <em>error.html</em> could not be "
                    "found.</p></body></html>").toUtf8());
        return;
    }

    QString html(QL1S(file.readAll()));

    QString doc(QL1S("<h1>"));
    doc += i18n("The requested operation could not be completed");
    doc += QL1S("</h1><h2>");
    doc += errorName;
    doc += QL1S("</h2>");

    if (!techName.isEmpty()) {
        doc += QL1S("<h2>");
        doc += i18n("Technical Reason: %1", techName);
        doc += QL1S("</h2>");
    }

    doc += QL1S("<h3>");
    doc += i18n("Details of the Request:");
    doc += QL1S("</h3><ul><li>");
    // escape URL twice: once for i18n, and once for HTML.
    doc += i18n("URL: %1", info.requestUrl.toDisplayString().toHtmlEscaped().toHtmlEscaped());
    doc += QL1S("</li><li>");

    const QString protocol(info.requestUrl.scheme());
    if (!protocol.isEmpty()) {
        // escape protocol twice: once for i18n, and once for HTML.
        doc += i18n("Protocol: %1", protocol.toHtmlEscaped().toHtmlEscaped());
        doc += QL1S("</li><li>");
    }

    doc += i18n("Date and Time: %1",
                 QLocale().toString(QDateTime::currentDateTime(), QLocale::LongFormat));
    doc += QL1S("</li><li>");
    // escape info.text twice: once for i18n, and once for HTML.
    doc += i18n("Additional Information: %1", info.text.toHtmlEscaped().toHtmlEscaped());
    doc += QL1S("</li></ul><h3>");
    doc += i18n("Description:");
    doc += QL1S("</h3><p>");
    doc += description;
    doc += QL1S("</p>");

    if (!causes.isEmpty()) {
        doc += QL1S("<h3>");
        doc += i18n("Possible Causes:");
        doc += QL1S("</h3><ul><li>");
        doc += causes.join("</li><li>");
        doc += QL1S("</li></ul>");
    }

    if (!solutions.isEmpty()) {
        doc += QL1S("<h3>");
        doc += i18n("Possible Solutions:");
        doc += QL1S("</h3><ul><li>");
        doc += solutions.join("</li><li>");
        doc += QL1S("</li></ul>");
    }
    
    QString title(i18n("Error: %1", errorName));
    QString direction(QApplication::isRightToLeft() ? "rtl" : "ltr");
    buf->write(html.arg(title, direction, m_warningIconData, doc).toUtf8());
}

