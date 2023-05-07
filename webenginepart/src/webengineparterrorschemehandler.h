/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2018 Stefano Crocco <posta@stefanocrocco.it>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef WEBENGINEPARTERRORSCHEMEHANDLER_H
#define WEBENGINEPARTERRORSCHEMEHANDLER_H

#include <QWebEngineUrlSchemeHandler>
#include <QUrl>
#include <QString>

#include <KIO/Global>

class QBuffer;

class WebEnginePartErrorSchemeHandler : public QWebEngineUrlSchemeHandler{
    
    Q_OBJECT
    
public:
    
    WebEnginePartErrorSchemeHandler(QObject *parent = nullptr);
    
    ~WebEnginePartErrorSchemeHandler() override {}
    
    void requestStarted(QWebEngineUrlRequestJob * job) override;
    
private:
    
    struct ErrorInfo{
        int code= KIO::ERR_UNKNOWN;
        QString text;
        QUrl requestUrl;
    };
    
    ErrorInfo parseErrorUrl(const QUrl& url);
    
    void writeErrorPage(QBuffer *buf, const ErrorInfo &info);
    
    QString readWarningIconData() const;
    
    const QString m_warningIconData;
};

#endif //WEBENGINEPARTERRORSCHEMEHANDLER_H
