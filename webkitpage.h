/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Dirk Mueller <mueller@kde.org>
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
#ifndef WEBKITPAGE_H
#define WEBKITPAGE_H

#include <qwebpage.h>
#include "webkitkde_export.h"
class QWebFrame;
class WebKitBrowserExtension;
class WebKitPart;

class WEBKITKDE_EXPORT WebPage : public QWebPage
{
    Q_OBJECT
public:
    WebPage(WebKitPart *wpart, QWidget *parent);

protected:
    virtual bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request,
            NavigationType type);
    virtual QString chooseFile(QWebFrame *parentFrame, const QString &suggestedFile);
    virtual void javaScriptAlert(QWebFrame *frame, const QString &msg);
    virtual bool javaScriptConfirm(QWebFrame *frame, const QString &msg);
    virtual bool javaScriptPrompt(QWebFrame *frame, const QString &msg, const QString &defaultValue, QString *result);
    virtual QString userAgentForUrl(const QUrl& url) const;


    void initGlobalSettings();
    virtual QObject *createPlugin(const QString &classid, const QUrl &url, const QStringList &paramNames, const QStringList &paramValues);

protected Q_SLOTS:
    void slotHandleUnsupportedContent(QNetworkReply *);

private:
    WebKitPart *m_part;
};

#endif // WEBKITPAGE_H
