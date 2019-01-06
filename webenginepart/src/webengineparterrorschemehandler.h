/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2018 Stefano Crocco <posta@stefanocrocco.it>
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

#ifndef WEBENGINEPARTERRORSCHEMEHANDLER_H
#define WEBENGINEPARTERRORSCHEMEHANDLER_H

#include <QWebEngineUrlSchemeHandler>
#include <QUrl>
#include <QString>

class QBuffer;

class WebEnginePartErrorSchemeHandler : public QWebEngineUrlSchemeHandler{
    
    Q_OBJECT
    
public:
    
    WebEnginePartErrorSchemeHandler(QObject *parent = nullptr);
    
    ~WebEnginePartErrorSchemeHandler() override {}
    
    void requestStarted(QWebEngineUrlRequestJob * job) override;
    
private:
    
    struct ErrorInfo{
        int code;
        QString text;
        QUrl requestUrl;
    };
    
    ErrorInfo parseErrorUrl(const QUrl& url);
    
    void writeErrorPage(QBuffer *buf, const ErrorInfo &info);
    
    QString readWarningIconData() const;
    
    const QString m_warningIconData;
};

#endif //WEBENGINEPARTERRORSCHEMEHANDLER_H
