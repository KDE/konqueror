/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2008 Michael Howell <mhowell123@gmail.com>
 * Copyright (C) 2009 Dawit Alemayehu <adawit @ kde.org>
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

class KWebWallet;
class KUrl;

/**
 * @short An enhanced QWebPage that provides integration into the KDE environment.
 *
 * This is a convenience class that provides full integration with KDE
 * technologies such as KIO for network request handling, KCookiejar for cookie
 * handling and KWebPluginFactory for embeded non-html content handling using
 * KDE KParts.
 *
 * @author Urs Wolfer <uwolfer @ kde.org>
 * @author Dawit Alemayehu <adawit @ kde.org>
 *
 * @since 4.4
 */

class KDEWEBKIT_EXPORT KWebPage : public QWebPage
{
    Q_OBJECT
public:
    /**
     * Constructs an empty KWebPage with parent @p parent.
     *
     * @param parent    the parent object.
     * @param windowId  the id of the window that contains this object.
     *
     * @see KIO::Intergation::CookieJar
     */
    explicit KWebPage(QObject *parent = 0, qlonglong windowId = 0);

    /**
     * Destroys the KWebPage.
     */
    ~KWebPage();

    /**
     * Returns true if access to remote content is allowed.
     *
     * By default access to remote content is allowed.
     *
     * @see setAllowExternalContent()
     * @see KIO::AccessManager::isExternalContentAllowed()
     */
    bool isExternalContentAllowed() const;

    /**
     * Set @p allow to false if you want to prevent access to remote content.
     *
     * If this function is set to false, only resources on the local system
     * can be accessed through this class. By default fetching external content
     * is allowed.
     *
     * @see isExternalContentAllowed()
     * @see KIO::AccessManager::setAllowExternalContent(bool)
     */
    void setAllowExternalContent(bool allow);

    /**
     * Returns true if access to the requested @p url is authorized.
     *
     * You should reimplement this function if you want to add features such as
     * content filtering or ad blocking. The default implementation simply
     * returns true.
     *
     * @param url the url to be authorized.
     * @return true in this default implementation.
     */
    virtual bool authorizedRequest(const QUrl &url) const;

    /**
     * Returns true KWallet used to store form data.
     */
    KWebWallet *wallet() const;

    /**
     * Sets the @ref KWebWallet that is used to store form data.
     *     
     * NOTE: KWebPage takes ownership of the KWebWallet object.
     *
     * KWebPage will set the parent of the wallet object passed to itself, so
     * that the wallet is deleted when this object is deleted as well. If you
     * do not want that to happen, you should set the wallet's parent to 0 after
     * calling this function.
     *
     * To disable wallet intgreation simply call this function with NULL.
     */
    void setWallet(KWebWallet* wallet);

public Q_SLOTS:
    /**
     * Downloads @p request using KIO.
     *
     * This slot first prompts the user where to put/save the requested
     * resource and then downloads using KIO::file_copy.
     */
    void downloadRequest(const QNetworkRequest &request);

    /**
     * An overload of the @ref downloadRequest slot with a QUrl parameter.
     */
    void downloadRequest(const KUrl &url);

protected:
    /**
     * Returns the value of the permanent (per session) meta data for the given @p key.
     *
     * @see KIO::MetaData
     */
    QString sessionMetaData(const QString &key) const;

    /**
     * Returns the value of the temporary (per request) meta data for the given @p key.
     *
     * @see KIO::MetaData
     */
    QString requestMetaData(const QString &key) const;

    /**
     * Set meta data that will be sent to KIO slave with every request.
     *
     * Note that meta data set using this function will be sent with
     * every request.
     *
     * @see KIO::MetaData
     */
    void setSessionMetaData(const QString &key, const QString &value);

    /**
     * Set meta data that will be sent to KIO slave with the first request.
     *
     * Note that a meta data set using this function will be deleted after
     * it has been sent the first time.
     *
     * @see KIO::MetaData
     */
    void setRequestMetaData(const QString &key, const QString &value);

    /**
     * Remove session meta data associated with @p key.
     */
    void removeSessionMetaData(const QString &key);

    /**
     * Remove request meta data associated with @p key.
     */
    void removeRequestMetaData(const QString &key);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWebPage::userAgentForUrl.
     * @internal
     */
    virtual QString userAgentForUrl(const QUrl& url) const;

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWebPage::acceptNavigationRequest.
     * @internal
     */
    virtual bool acceptNavigationRequest(QWebFrame * frame, const QNetworkRequest & request, NavigationType type);

private:
    class KWebPagePrivate;
    KWebPagePrivate* const d;
};

#endif // KWEBPAGE_H
