/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2008 Michael Howell <mhowell123@gmail.com>
 * Copyright (C) 2009 Dawit Alemayehu <adawit@kde.org>
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
#ifndef KWEBPAGE_H
#define KWEBPAGE_H

#include <kdewebkit_export.h>

#include <QtWebKit/QWebPage>

class QUrl;
class QWebFrame;
class QNetworkReply;

/**
 * An enhanced QWebPage with integration into the KDE environment.
 *
 * @author Urs Wolfer <uwolfer @ kde.org>
 * @since 4.4
 */

class KDEWEBKIT_EXPORT KWebPage : public QWebPage
{
    Q_OBJECT
public:
    /**
     * Constructs an empty KWebPage with parent @p parent.
     */
    explicit KWebPage(QObject *parent = 0);

    /**
     * Destroys the KWebPage.
     */
    ~KWebPage();

    /**
     * Set @p allow to false if you don't want to allow showing external content,
     * so no external images for example. By default external content is fetched.
     */
    void setAllowExternalContent(bool allow);

    /**
     * Returns true if external content is fetched, @see setAllowExternalContent().
     */
    bool isExternalContentAllowed() const;

protected:
    /**
     * Reimplemented for internal reasons, the API is not affected.
     * @internal
     */
    virtual KWebPage *createWindow(WebWindowType type);

    virtual KWebPage *newWindow(WebWindowType type);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     * @internal
     */
    QString chooseFile(QWebFrame *frame, const QString &suggestedFile);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     * @internal
     */
    void javaScriptAlert(QWebFrame *frame, const QString &msg);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     * @internal
     */
    bool javaScriptConfirm(QWebFrame *frame, const QString &msg);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     * @internal
     */
    bool javaScriptPrompt(QWebFrame *frame, const QString &msg, const QString &defaultValue, QString *result);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     * @internal
     */
    QString userAgentForUrl(const QUrl& url) const;

    void setSessionMetaData(const QString& key, const QString& value);
    void setRequestMetaData(const QString& key, const QString& value);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     * @internal
     */
    bool acceptNavigationRequest (QWebFrame * frame, const QNetworkRequest & request, NavigationType type);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     * @internal
     */
    QObject *createPlugin(const QString &classId, const QUrl &url, const QStringList &paramNames, const QStringList &paramValues);

protected Q_SLOTS:
    virtual void slotHandleUnsupportedContent(QNetworkReply *reply);
    virtual void slotDownloadRequested(const QNetworkRequest &request);
    virtual void slotDownloadRequested(const QNetworkRequest &request, QNetworkReply *reply);

private:
    class KWebPagePrivate;
    KWebPagePrivate* const d;
};

#endif // KWEBPAGE_H
