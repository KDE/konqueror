/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
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
#ifndef WEBPAGE_H
#define WEBPAGE_H

#include "websslinfo.h"

#include <KDE/KWebPage>
#include <KDE/KParts/BrowserExtension>

#include <QtCore/QUrl>
#include <QtCore/QDebug>
#include <QtCore/QMultiHash>

class KUrl;
class WebSslInfo;
class KWebKitPart;
class QVariant;
class QWebFrame;


/**
 * This is a fake implementation of QWebPage used to adapt QWebPage's API for
 * creating new window with that of KPart's.
 *
 * The KPart API for creating window requires all the information about the new
 * window be present before hand. Unfortunately the QWebPage::createWindow function
 * does not provide any information except for the window type. As such, this class
 * is designed and used to collect all of the necessary information such as window
 * name, size and position before invoking the specified KPart's create new window
 * properly.
 */
class NewWindowAdapterPage : public QWebPage
{
    Q_OBJECT
public:
    NewWindowAdapterPage(KParts::ReadOnlyPart* part, WebWindowType type, QObject* parent = 0);
    virtual ~NewWindowAdapterPage();

protected:
    virtual bool acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, NavigationType type);

private Q_SLOTS:
    void slotGeometryChangeRequested(const QRect& rect);
    void slotMenuBarVisibilityChangeRequested(bool visible);
    void slotStatusBarVisibilityChangeRequested(bool visible);
    void slotToolBarVisibilityChangeRequested(bool visible);

private:
    KUrl m_requestUrl;
    KParts::WindowArgs m_windowArgs;
    WebWindowType m_type;
    QPointer<KParts::ReadOnlyPart> m_part;
};


class WebPage : public KWebPage
{
    Q_OBJECT
public:
    WebPage(KWebKitPart *wpart, QWidget *parent);
    ~WebPage();

    /**
     * Returns the SSL information for the current page.
     *
     * @see WebSslInfo.
     */
    const WebSslInfo& sslInfo() const;

    /**
     * Sets the page's SSL information to @p other.
     *
     * @see WebSslInfo
     */
    void setSslInfo (const WebSslInfo &other);

    /**
     * Reimplemented for internal reasons. The API is not affected.
     *
     * @internal
     * @see KWebPage::downloadRequest.
     */
    void downloadRequest(const QNetworkRequest &request);

    /**
     * Returns the error page associated with the KIO error @p code.
     *
     * @param text the error message.
     * @param url the url where the error was encountered.
     *
     * @return html error page.
     */
    QString errorPage(int code, const QString& text, const KUrl& url) const;

    /**
     * Re-implemented to handle ErrorPageExtension.
     *
     * @see QWebPage::extension()
     */
    bool extension(Extension extension, const ExtensionOption *option, ExtensionReturn *output);

    /**
     * Re-implemented to handle ErrorPageExtension.
     *
     * @see QWebPage::supportsExtension()
     */
    bool supportsExtension(Extension extension) const;

Q_SIGNALS:
    /**
     * This signal is emitted whenever a user cancels/aborts a load resource
     * request.
     */
    void loadAborted(const KUrl &url);

protected:
    /**
     * Reimplemented for internal reasons, the API is not affected.
     * @internal
     */
    virtual QWebPage* createWindow(WebWindowType type);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     * @internal
     */
    virtual bool acceptNavigationRequest(QWebFrame * frame, const QNetworkRequest & request, NavigationType type);

protected Q_SLOTS:
    void slotRequestFinished(QNetworkReply *reply);

private:
    bool checkLinkSecurity(const QNetworkRequest &req, NavigationType type) const;
    bool checkFormData(const QNetworkRequest &req) const;
    bool handleMailToUrl (const QUrl& , NavigationType type) const;
    void setPageJScriptPolicy(const QUrl &url);

private:
    enum WebPageSecurity { PageUnencrypted, PageEncrypted, PageMixed };

    int m_kioErrorCode;
    bool m_ignoreError;
    bool m_ignoreHistoryNavigationRequest;

    WebSslInfo m_sslInfo;
    QVector<QUrl> m_requestQueue;
    QPointer<KWebKitPart> m_part;
};

#endif // WEBPAGE_H
