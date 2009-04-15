/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2008 Michael Howell <mhowell123@gmail.com>
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

#include <kdemacros.h>
#include <KDE/KUrl>

#include <QtWebKit/QWebPage>

class QWebFrame;

class KDE_EXPORT KWebPage : public QWebPage
{
    Q_OBJECT
public:
    KWebPage(QObject *parent);
    ~KWebPage();
    /**
     * Set @p allow to false if you don't want to allow showing external content,
     * so no external images for example. By default external content is fetched.
     */
    void setAllowExternalContent(bool allow);

    /**
     * returns if external content is fetched, see setAllowExternalContent().
     */
    bool allowExternalContent() const;

protected:
    virtual KWebPage *createWindow(WebWindowType type);
    virtual KWebPage *newWindow(WebWindowType type);
    QString chooseFile(QWebFrame *frame, const QString &suggestedFile);
    void javaScriptAlert(QWebFrame *frame, const QString &msg);
    bool javaScriptConfirm(QWebFrame *frame, const QString &msg);
    bool javaScriptPrompt(QWebFrame *frame, const QString &msg, const QString &defaultValue, QString *result);
    QString userAgentForUrl(const QUrl& url) const;

    QObject *createPlugin(const QString &classId, const QUrl &url, const QStringList &paramNames, const QStringList &paramValues);

protected Q_SLOTS:
    virtual void slotHandleUnsupportedContent(QNetworkReply *reply);
    virtual void slotDownloadRequested(const QNetworkRequest &request);

private:
    class KWebPagePrivate;
    KWebPagePrivate* const d;
};

#endif // KWEBPAGE_H
