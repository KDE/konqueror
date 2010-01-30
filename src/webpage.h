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

#include <kwebpage.h>

#include <QtCore/QUrl>
#include <QtCore/QDebug>


class KUrl;
class WebSslInfo;
class KWebKitPart;
class QVariant;
class QWebFrame;

struct WebFrameState
{
  QUrl url;
  int scrollPosX;
  int scrollPosY;
  bool handled;

  WebFrameState() : scrollPosX(0), scrollPosY(0), handled(false) {}

  inline friend QDebug& operator<< (QDebug& stream, const WebFrameState &frameState) {
      stream << frameState.url << frameState.scrollPosX << frameState.scrollPosY;
      return stream;
  }
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
     * Returns the frames state for @p frameName.
     */
    WebFrameState frameState(const QString& frameName) const;

    /**
     * Sets the cached page SSL information to @p info.
     *
     * @see WebSslInfo
     */
    void setSslInfo (const WebSslInfo &info);

    /**
     * Saves the frame state information for @p frameName.
     *
     * @param frameName     the frame name.
     * @param frameState    the frame state information.
     */
    void saveFrameState (const QString &frameName, const WebFrameState &frameState);

    /**
     * Restore the frame state from the saved
     *
     * @param frameName     the frame name.
     */
    void restoreFrameState(const QString &frameName);

    /**
     * Restores the states of all the frames in the page.
     */
    void restoreAllFrameState();

    /**
     * Reimplemented for internal reasons. The API is not affected.
     *
     * @internal
     * @see KWebPage::downloadRequest.
     */
    void downloadRequest(const QNetworkRequest &request);

Q_SIGNALS:
     /**
      * This signal is emitted whenever a navigation request completes...
      *
      * The @p url is the requested url or in case of an error a special error
      * url of form error:/?err=<code>&errText=<text>#<request-url>. The second
      * parameter @p frame is the QWebFrame which orignated the request in the.
      */
    void navigationRequestFinished(const KUrl& url, QWebFrame *frame);

    /**
     * This signal is emitted whenever a user cancels/aborts a load resource
     * request.
     */
    void loadAborted(const KUrl &url);

    /**
     * This signal is emitted whenever status message is received from javascript
     * and the user's configuration allows it to be set.
     */
    void jsStatusBarMessage(const QString &);

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
    void slotUnsupportedContent(QNetworkReply *reply);
    void slotRequestFinished(QNetworkReply *reply);
    void slotGeometryChangeRequested(const QRect &rect);
    void slotWindowCloseRequested();
    void slotStatusBarMessage(const QString &message);

private:
    bool checkLinkSecurity(const QNetworkRequest &req, NavigationType type) const;
    bool checkFormData(const QNetworkRequest &req) const;
    bool handleMailToUrl (const QUrl& , NavigationType type) const;
    void setPageJScriptPolicy(const QUrl &url);

private:
    class WebPagePrivate;
    WebPagePrivate* const d;
};

#endif // WEBPAGE_H
