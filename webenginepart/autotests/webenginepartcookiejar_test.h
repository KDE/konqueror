/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2018 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef TESTWEBENGINEPARTCOOKIEJAR_H
#define TESTWEBENGINEPARTCOOKIEJAR_H

#include <QObject>
#include <QDateTime>
#include <QDBusError>

class QWebEngineCookieStore;
class WebEnginePartCookieJar;
class QNetworkCookie;
class QWebEngineProfile;
class QDBusInterface;

class TestWebEnginePartCookieJar : public QObject
{
    Q_OBJECT
    
private:
    
    struct CookieData {
        QString name;
        QString value;
        QString domain;
        QString path;
        QString host;
        QDateTime expiration;
        bool secure;
        
        QNetworkCookie cookie() const;
    };
    
private Q_SLOTS:

    void init();
    void initTestCase();
    void cleanup();
    void testCookieAddedToStoreAreAddedToKCookieServer_data();
    void testCookieAddedToStoreAreAddedToKCookieServer();
    void testCookieRemovedFromStoreAreRemovedFromKCookieServer_data();
    void testCookieRemovedFromStoreAreRemovedFromKCookieServer();
    void testPersistentCookiesAreAddedToStoreOnCreation();
    void testSessionCookiesAreNotAddedToStoreOnCreation();
    
private:
    
    /**
    * @brief Adds a cookie to KCookieServer
    * 
    * The cookie is supposed to be in `QWebEngineStore` "format", that is its domain must not be empty;
    * a domain not starting with a dot means that the domain field wasn't given in the `Set-Cookie` header.
    * 
    * @param _cookie the cookie to add
    * @param host the host where the cookie come from
    * @return QDBusError the error returned by DBus when adding the cookie. If no error occurred, this object
    * will be invalid
    */
    QDBusError addCookieToKCookieServer(const QNetworkCookie &_cookie, const QString &host);
    void deleteCookies(const QList<CookieData> &cookies);
    QList<CookieData> findTestCookies();
    
    QString m_cookieName;
    QWebEngineCookieStore *m_store;
    WebEnginePartCookieJar *m_jar;
    QWebEngineProfile *m_profile;
    QDBusInterface *m_server;
};

#endif // TESTWEBENGINEPARTCOOKIEJAR_H
