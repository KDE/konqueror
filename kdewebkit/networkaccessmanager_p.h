/*
 * This file is part of the KDE project.
 *
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
#ifndef NETWORKACCESSMANAGER_P_H
#define NETWORKACCESSMANAGER_P_H

#include <kdewebkit_export.h>

#include <kio/accessmanager.h>

namespace KDEPrivate {

 /**
  * Reimplementation of KIO::AccessManager to provide content filtering support
  * and a convenience function for storing persistent and temporary KIO meta data.
  *
  * @author Dawit Alemayehu <adawit @ kde.org>
  */
class KDEWEBKIT_EXPORT NetworkAccessManager : public KIO::AccessManager
{
  Q_OBJECT

public:
    NetworkAccessManager(QObject *parent);
    ~NetworkAccessManager() {}

    /**
     * Sets the cookiejar's window id to @p id.
     *
     * This is a convenience function that allows you to set the cookiejar's
     * window id. Note that this function does nothing unless the cookiejar in
     * use is of type KIO::Integration::CookieJar.
     *
     * By default the cookiejar's window id is set to false. Make sure you call
     * this function and set the window id to its proper value when create an
     * instance of this object. Otherwise, the KDE cookiejar will not be able
     * to properly manage session based cookies.
     *
     * @see KIO::Integration::CookieJar::setWindowId.
     * @since 4.4
     */
    void setCookieJarWindowId(qlonglong id);

    /**
     * Returns the cookiejar's window id.
     *
     * This is a convenience function that returns the window id associated
     * with the cookiejar. Note that this function will return a 0 if the
     * cookiejar is not of type KIO::Integration::CookieJar or a window id
     * has not yet been set.
     *
     * @see KIO::Integration::CookieJar::windowId.
     * @since 4.4
     */
    qlonglong cookieJarWindowid() const;

    /**
     * Returns a reference to the temporary meta data container.
     *
     * See kdelibs/kio/DESIGN.metadata for list of supported KIO meta data.
     *
     * This convenience function allows you to set per request KIO meta data
     * that will be removed after it has been sent once.
     */
    KIO::MetaData& requestMetaData();

    /**
     * Returns a reference to the persistent meta data container.
     *
     * See kdelibs/kio/DESIGN.metadata for list of supported KIO meta data.
     *
     * This convenience function allows you to set per session KIO meta data
     * that will be sent with every request.
     *
     * Unlike the above method the meta data values set using the reference
     * returned by this function are not deleted after they have been sent once.
     */
    KIO::MetaData& sessionMetaData();

protected:
    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see KIO::AccessManager::createRequest.
     * @internal
     */
    virtual QNetworkReply *createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData = 0);

private:
    class NetworkAccessManagerPrivate;
    NetworkAccessManagerPrivate* d;
};

}

#endif // NETWORKACCESSMANAGER_P_H
