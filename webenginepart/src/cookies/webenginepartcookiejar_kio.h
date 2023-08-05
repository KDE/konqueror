/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2018 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef WEBENGINEPARTCOOKIEJAR_KIO_H
#define WEBENGINEPARTCOOKIEJAR_KIO_H

#include <QObject>
#include <QNetworkCookie>
#include <QHash>
#include <QUrl>
#include <QWebEngineCookieStore>
#include <QStringList>
#include <QVariant>
#include <QVector>
#include <QDBusInterface>
#include <QSet>
#include <QtWebEngine/QtWebEngineVersion>

#include "kwebenginepartlib_export.h"

class QDebug;
class QWidget;
class QWebEngineProfile;
class QDBusPendingCallWatcher;

/**
 * @brief Class which takes care of synchronizing Chromium cookies from `QWebEngineCookieStore` with KIO
 */
class KWEBENGINEPARTLIB_EXPORT WebEnginePartCookieJarKIO : public QObject
{
    Q_OBJECT

public:
    
    /**
    * @brief Constructor
    * 
    * @param [in,out] prof the profile containing the store to synchronize with
    * @param parent the parent object
    * @note The `PersistentCookiePolicy` of the given profile will be set to `NoPersistentCookies` so that, on application startup, 
    * only cookies from KIO will be inside the store
    */
    WebEnginePartCookieJarKIO(QWebEngineProfile* prof, QObject* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~WebEnginePartCookieJarKIO() override;

private slots:
    
    /**
    * @brief Adds a cookie to KIO
    * 
    * This slot is called in response to `QWebEngineCookieStore::cookieAdded` signal.
    * 
    * @param cookie cookie the cookie to add
    * 
    * @internal KIO requires an URL when adding a cookie; unfortunately, the `cookieAdded` signal doesn't provide one. To solve this problem, an URL is created
    * by using the scheme of #m_lastRequestOrigin and the cookie's domain. Looking At the source code for `KCookieJar`, an URL built this way should work.
    * In case the cookie's domain is empty (which I believe shouldn't happen), #m_lastRequestOrigin is used.
    * 
    * This function also adds the cookie and its corresponding URL to #m_cookiesUrl
    */
    void addCookie(const QNetworkCookie &cookie);
    
    /**
    * @brief Removes the given cookie from KIO
    * 
    * This slot is called in response to `QWebEngineCookieStore::cookieRemoved` signal.
    * 
    * @param cookie the cookie to remove
    * 
    * @internal As for addCookie() there's a problem here, because `QWebEngineCookieStore::cookieRemoved` doesn't provide a fqdn for the cookie. This value is obtained 
    * from #m_cookiesUrl.
    */
    void removeCookie(const QNetworkCookie &cookie);
    
    /**
    * @brief Removes all session cookies from `KCookieJar`
    * 
    * This function doesn't remove cookies from the `QWebEngineCookieStore`, because it's meant to be called
    * when the application is closing, so those cookies will be destroyed automatically (because they're only stored
    * in memory)
    * 
    */
    void deleteSessionCookies();
    
    /**
     * @brief Slot called when the DBus call to `KCookieServer::deleteCookie` fails
     * 
     * It displays the error message on standard error
     * 
     * @param watcher the object describing the result of the DBus call
     */
    void cookieRemovalFailed(QDBusPendingCallWatcher *watcher);
    
private:
    
    /**
    * @brief Determine the window ID to pass to `KCookieServer::addCookies`
    *
    * There's no sure way to find the window which received a cookie, so this function chooses the active window
    * if there's one (according to `QApplication::activeWindow`) or the first widget in `QApplication::topLevelWidgets`
    * for which `QWidget::isWindow` is `true` and which doesn't have a parent.
    * 
    * @return the ID of the window determined as described above or 0 if no such a window can be found
    */
    static qlonglong findWinID();

    struct CookieWithUrl {
        QNetworkCookie cookie;
        QUrl url;
    };

    using CookieUrlList = QVector<CookieWithUrl>;
    using CookieList = QVector<QNetworkCookie>;

    /**
    * @brief An identifier for a cookie
    * 
    * The identifier is made by the cookie's name, domain and path
    */
    struct CookieIdentifier{
        
        /**
        * @brief Default constructor
        */
        CookieIdentifier(){}
        
        /**
        * @brief Constructor
        * 
        * @param cookie the cookie to create the identifier for
        */
        CookieIdentifier(const QNetworkCookie &cookie);
        
        /**
        * @brief Constructor
        * 
        * @param n the cookie's name
        * @param d the cookie's domain
        * @param p the cookie's path
        */
        CookieIdentifier(const QString &n, const QString &d, const QString &p);
       
        /**
         * Comparison operator
         * 
         * Two cookies are equal if all their fields are equal
         * @param other the identifier to compare this identifier to
         * @return `true` if the two identifiers' name, domain and path are equal and `false` if at least one of them is different
         */
        bool operator== (const CookieIdentifier &other) const {return name == other.name && domain == other.domain && path == other.path;}
        
        /**
        * @brief The name of the cookie
        * 
        */
        QString name;
        /**
        * @brief The domain of the cookie
        * 
        */
        QString domain;
        
        /**
        * @brief The path of the cookie
        * 
        */
        QString path;
        
    };
    
    friend QDebug operator<<(QDebug, const CookieIdentifier &);
    
    using CookieIdentifierList = QList<CookieIdentifier>;

    /**
    * @brief Checks whether a cookie with the given identifier is in the KCookieJar
    * 
    * @note If the _id_'s domain doesn't start with a dot, a dot is prepended to it. This is because
    * `KCookieJar` always wants a dot in front of the domain.
    * 
    * @param id the identifiere of the cookie
    * @param url the origin of the cookie
    * @return `true` if a cookie with the given identifier is in the KCookieJar and `false` otherwise
    */
    bool cookieInKCookieJar(const CookieIdentifier &id, const QUrl &url);
    
    /**
    * @brief The advice from `KCookieJar` for a URL
    * 
    * @param url The URL to get the advice for
    * @return The advice for _url_. It can be one of `"Accept"`, `"AcceptForSession"`, `"Reject"`, `"Ask"`, `"Dunno"` or an empty
    * string if an error happens while contacting the `KCookieServer`
    */
    QString askAdvice(const QUrl &url);
    
    /**
    * @brief Inserts all cookies contained in `KCookieJar` to the store
    * 
    * @note this function should only be called by the constructor
    * @note this function is asynchronous because it calls `QWebEngineCookieStore::setCookie`
    */
    void loadKIOCookies();
    
    /**
    * @brief Finds all cookies stored in `KCookieJar`
    * @return a list of the cookies in `KCookieJar`
    */
    CookieUrlList findKIOCookies();
    
    /**
    * @brief Enum describing the possible fields to pas to `KCookieServer::findCookies` using DBus.
    * 
    * The values are the same as those of `CookieDetails` in `kio/src/kioslaves/http/kcookiejar/kcookieserver.cpp`
    */ 
    enum class CookieDetails {domain=0, path=1, name=2, host=3, value=4, expirationDate=5, protocolVersion=6, secure=7};
    
    /**
    * @brief Parses the value returned by `KCookieServer::findCookies` for a single cookie
    * 
    * This function assumes that all possible data for the cookie is available (that is, that the list returned by 
    * `KCookieServer::findCookies` contains an entry for each value in #CookieDetails)
    * 
    * @param data: the data returned by `KCookieServer::findCookies`. It can contain data for more than one cookie, but only one will be parsed
    * @param start: the position in the list where the data for the cookie starts.
    * @return The cookie described by the data and its host
    */
    static CookieWithUrl parseKIOCookie(const QStringList &data, int start);
    
    /**
    * @brief Function used to filter cookies
    * 
    * In theory, this function should use the configuration chosen by the user in the Cookies KCM. However, this can't be done for several reasons:
    * - this function doesn't have the cookies details  and they're needed for the "Ask" policy
    * - this function doesn't know the URL of the cookie (even if `QWebEngineCookieStore::FilterRequest::origin` could be used as a substitute
    * - if the policy is "Ask" and the question was asked here, it would be asked again when adding the cookie to `KCookieJar`.
    * Because of these reasons, the only setting from the KCM which is applied here is whether to accept and reject cross domain cookies. Other settings
    * from the KCM will be enforced by the addCookies().
    * 
    * @param req the request to filter
    * @return ``false` for third party cookies if the user chose to block them in the KCM and `true` otherwise
    * @internal Besides filtering cookie requests, this function also stores the `origin` member of request in the #m_lastRequestOrigin.
    * @endinternal
    * @sa addCookie()
    */
    bool filterCookie(const QWebEngineCookieStore::FilterRequest &req);
    
    /**
    * @brief Removes the domain from the cookie if the domain doesn't start with a dot
    * 
    * The cookies managed by QWebEngine never have an empty domain; instead, their domain starts with a dot if it was explicitly given and doesn't start
    * with a dot in case it wasn't explicitly given. `KCookieServer::addCookies` and `KCookieServer::deleteCookie`, instead, require an empty domain if the
    * domain wasn't explicitly given. This function transforms a cookie of the first form to one of the second form.
    * 
    * If the cookie's domain starts with a dot, this function does nothing.
    * 
    * @param cookie the cookie to remove the domain from. This cookie is modified in place
    */
    static void removeCookieDomain(QNetworkCookie &cookie);
    
    /**
    * @brief Tries to construct an Url for the given cookie
    * 
    * The URL is meant to be passed to functions like `KCookieServer::addCookies` and is constructed from the cookie's domain
    * if it's not empty. If the domain is empty or it's only a dot, an empty URL is returned
    * 
    * @param cookie the cookie to create the URL for
    * @return the URL for the cookie or an empty URL if the cookie's domain is empty or it's only a dot
    */
    QUrl constructUrlForCookie(const QNetworkCookie &cookie) const;
    
    /**
    * @brief The `QWebEngineCookieStore` to synchronize KIO with
    */
    QWebEngineCookieStore* m_cookieStore;
    
    /**
    * @brief The DBus interface used to connect to KCookieServer
    */
    QDBusInterface m_cookieServer;
    
    /**
    * @brief The fields to pass to `KCookieStore::findCookies` via DBus
    */
    static const QVariant s_findCookieFields;
    
    /**
    * @brief Overload of `qHash` for a CookieIdentifier
    * 
    * @param id: the other identifier
    * @param seed: the seed
    * @return The hash value of the identifier
    */
    friend uint qHash(const CookieIdentifier &id, uint seed){return qHash(QStringList{id.name, id.domain, id.path}, seed);};
    
    /**
    * @brief A list of cookies which were added to the `QWebEngineCookieStore` but were rejected by KCookieJar and must
    *   be removed from the store 
    * 
    * When cookieRemoved() is called with one of those cookies, the cookie is removed from this list and no attempt is made to remove
    * the cookie from `KCookieJar` (because it's not there)
    */
    QVector<CookieIdentifier> m_pendingRejectedCookies;
    
    /**
    * @brief The IDs of all the windows which have session cookies
    */
    QSet<qlonglong> m_windowsWithSessionCookies;
    
    /**
    * @brief A list of cookies loaded from KCookieServer when this instance is created
    */
    CookieList m_cookiesLoadedFromKCookieServer;
    
#ifdef BUILD_TESTING
    QList<QNetworkCookie> m_testCookies;
    friend class TestWebEnginePartCookieJarKIO;
#endif
    
};

/**
* @brief Overload of operator `<<` to allow a WebEnginePartCookieJarKIO::CookieIdentifier to be written to a `QDebug`
* 
* @param deb the debug object
* @param id the identifier to write
* @return the debug object
*/
QDebug operator<<(QDebug deb, const WebEnginePartCookieJarKIO::CookieIdentifier &id);

#endif // WEBENGINEPARTCOOKIEJAR_KIO_H
