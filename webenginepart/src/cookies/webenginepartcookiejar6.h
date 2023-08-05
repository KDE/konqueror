/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2018 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef WEBENGINEPARTCOOKIEJAR_H
#define WEBENGINEPARTCOOKIEJAR_H

#include "interfaces/cookiejar.h"

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
 * @brief Class which enforces the Cookie user settings and allows access to the cookies in the store
 *
 * @note All functions which access the cookie store are asynchronous
 */
class KWEBENGINEPARTLIB_EXPORT WebEnginePartCookieJar6 : public KonqInterfaces::CookieJar
{
    Q_OBJECT

public:
    
    /**
    * @brief Constructor
    * 
    * @param [in,out] prof the profile containing the store to synchronize with
    * @param parent the parent object
    */
    WebEnginePartCookieJar6(QWebEngineProfile* prof, QObject* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~WebEnginePartCookieJar6() override;

    /**
     * @brief A set with all cookies in the cookie store
     * @return a set with all cookies in the cookie store
     * @note This function is synchronous, as it doesn't access the store but returns the cookies already inserted in #m_cookies.
     * It is possible that a cookie has been added to the store but the store hasn't yet emitted the corresponding `cookieAdded()`
     * signal: in this case, that cookie won't be included in the returned value.
     */
    QSet<QNetworkCookie> cookies() const override;

public slots:

    /**
     * @brief Asks the store to remove all cookies
     *
     * @note This will also reset the choices made by the user for all cookies
     */
    void removeAllCookies() override;

    /**
     * @brief Asks the store to remove all cookies from a domain
     * @param domain the domain whose cookies should be removed. A leading dot is ignored
     * @note This will also reset the choices made by the user for the removed cookies
     */
    void removeCookiesWithDomain(const QString &domain) override;

    /**
     * @brief Asks the store to remove the given cookies
     *
     * If you need to remove multiple cookies, it's better to call this rather than
     * calling removeCookie() multiple times, as this will only update the cookie advice
     * file once (if needed)
     * @note This will also reset the choices made by the user for the removed cookies
     */
    void removeCookies(const QVector<QNetworkCookie> & cookies) override;

    /**
     * @brief Asks the store to remove the given cookie
     * @param cookie the cookie to remove
     * @param origin only remove the cookie if it originates from this URL
     * @note This will also reset the choices made by the user for the removed cookie
     */
    void removeCookie(const QNetworkCookie &cookie, const QUrl &origin=QUrl()) override;

    /**
     * @brief Asks the cookie store to remove all session cookies
     */
    void removeSessionCookies() override;

private slots:

    /**
     * @brief Read the cookie configuration settings from the configuration file
     *
     * If cookies have been disabled, the cookie store is emptied. All other
     * configuration changes won't be applied to cookies already in the store.
     * They will be applied next time the cookie jar is created (most likely, when
     * the application is next started).
     */
    void applyConfiguration();

    /**
     * @brief Slot called in response to the `QWebEngineCookieStore::cookieAdded` signal
     *
     * It checks whether the policy chosen by the user allows accepting the cookie and
     * removes it if it doesn't. If, instead, the cookie can be accepted, it adds it
     * to #m_cookies.
     * @note Theoretically, most of what this method does should be done by filterCookie().
     * Unfortunately, that function doesn't have access to the cookie details it would
     * need, so we need to do it here. This means that even cookies which should be rejected
     * are actually added and then removed.
    */
    void handleCookieAdditionToStore(const QNetworkCookie &cookie);

    /**
    * @brief Removes the given cookie from the list of cookies.
    *
    * This slot is called in response to `QWebEngineCookieStore::cookieRemoved` signal.
    * 
    * @param cookie the cookie to remove
    */
    void removeCookieFromSet(const QNetworkCookie &cookie);

    /**
     * @brief The path of the file where to save the choices made by the user for single cookies
     * @return The path of the file where to save the choices made by the user for single cookies
     */
    static QString cookieAdvicePath();

    /**
     * @brief Saves to file the choices the user made for individual cookies
     */
    void saveCookieAdvice();

    /**
     * @brief Reads from file the choices the user made for individual cookies
     */
    void readCookieAdvice();
    
private:
    
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

        QString name; ///< The name of the cookie
        QString domain; ///< The domain of the cookie
        QString path; ///< The path of the cookie
        
    };

    /**
     * @brief Decides what to do with a cookie according to the policy chosen by the user
     *
     * This method compares the cookie with the settings in #m_policy and returns the appropriate action.
     * If needed, it calls askCookieQuestion() to ask the user what to do.
     * @param cookie the cookie
     * @return the CookieAdvice describing what to do with the cookie
     */
    CookieAdvice decideCookieAction(const QNetworkCookie cookie);

    /**
     * @brief Asks the user what to do with a given cookie
     *
     * It displays a dialog where the user can choose whether to accept the cookie, reject it or accept it
     * only for the current session.
     *
     * In the dialog, the user can also decide to apply his choice to all cookies or to the cookies from
     * the same domain. If he does, then the configuration file is changed accordingly.
     * @param cookie the cookie
     * @return what to do with the cookie
     */
    CookieAdvice askCookieQuestion(const QNetworkCookie cookie);

    /**
     * @brief Updates the policy in the configuration file
     *
     * This is only called if, when asked what to do about a cookie, the user chooses to apply its choice
     * to all cookies or to the cookies from the same domain.
     */
    void writeConfig();
    
    friend QDebug operator<<(QDebug, const CookieIdentifier &);
    friend QDataStream& operator>>(QDataStream &ds, WebEnginePartCookieJar6::CookieIdentifier &id);
    friend QDataStream& operator<<(QDataStream &ds, const WebEnginePartCookieJar6::CookieIdentifier &id);

    using CookieIdentifierList = QList<CookieIdentifier>;

    /**
    * @brief Decides whether to accept or not a cookie
    *
    * In theory, this function should use the policy chosen by the user to decide whether to accept or reject the cookie.
    * However, to do so, it would need details about the cookie which aren't provided by @p req. Because of this, this
    * function only rejects the cookie in two circumstances:
    * - if cookies are completely disabled
    * - if it's a third party cookie and the policy forbids accepting third party cookies
    * In all other cases, this function accepts the cookie, leaving to handleCookieAdditionToStore() the task of removing
    * it if needed.
    * @note It's not clear whether accepting the cookie and then removing it has the same effect as rejecting it in the first
    * place. However, it's the best that can be done without reducing the range of policies the user can choose from.
    * 
    * @param req the request to filter
    * @return `false` if cookies are disabled or if @p cookie is a third party cookie and third party cookies have been disabled
    * and `true` otherwise
    * @sa handleCookieAdditionToStore()
    */
    bool filterCookie(const QWebEngineCookieStore::FilterRequest &req);

    /**
    * @brief The `QWebEngineCookieStore` to use
    */
    QWebEngineCookieStore* m_cookieStore;
    
    /**
    * @brief Overload of `qHash` for a CookieIdentifier
    * 
    * @param id: the other identifier
    * @param seed: the seed
    * @return The hash value of the identifier
    */
    friend uint qHash(const CookieIdentifier &id, uint seed){return qHash(QStringList{id.name, id.domain, id.path}, seed);};
    
    /**
     * @brief The cookies stored in #m_cookieStore
     *
     * This is needed to implement CookieJar::cookies(), since `QWebEngineCookieStore` doesn't allow to retrieve the list of
     * cookies.
     */
    QSet<QNetworkCookie> m_cookies;


    /**
     * @brief Struct describing the cookie policy chosen by the user
     */
    struct CookiePolicy {
        bool cookiesEnabled = true; ///< Whether cookies are enabled or should all be rejected
        bool rejectThirdPartyCookies = true; ///< Whether third party cookies should all be rejected
        bool acceptSessionCookies = true; ///< Whether session cookies should be accepted by default
        CookieAdvice defaultPolicy = CookieAdvice::Accept; ///< What to do for cookies which don't match any other rule
        /**
         * @brief What to do for cookies belonging to specific domains
         *
         * Each entry has the domain as key and the action to carry out for cookies of that domain as value
         */
        QHash<QString, CookieAdvice> domainExceptions;
        /**
         * @brief What to do for specific cookies
         *
         * Each entry has the cookie identifier as key and the action as value
         */
        QHash<CookieIdentifier, CookieAdvice> cookieExceptions;
    };
    CookiePolicy m_policy; ///< The policy to apply to cookies
};

/**
* @brief Overload of `qHash` for a `QNetworkCookie`
*
* @param cookie: the cookie
* @param seed: the seed
* @return The hash value of the cookie
*/
uint qHash(const QNetworkCookie &cookie, uint seed);

/**
* @brief Overload of operator `<<` to allow a WebEnginePartCookieJar6::CookieIdentifier to be written to a `QDebug`
* 
* @param deb the debug object
* @param id the identifier to write
* @return the debug object
*/
QDebug operator<<(QDebug deb, const WebEnginePartCookieJar6::CookieIdentifier &id);

/**
 * @brief override of operator `>>` allowing to read a CookieIdentifier from a `QDataStream`
 * @param ds the `QDataStream` to read the CookieIdentifier from
 * @param id the CookieIdentifier to read to @p ds
 * @return @p ds
 */
QDataStream& operator>>( QDataStream &ds, WebEnginePartCookieJar6::CookieIdentifier &id);

/**
 * @brief override of operator `<<` allowing to write a CookieIdentifier to a `QDataStream`
 * @param ds the `QDataStream` to write the CookieIdentifier to
 * @param id the CookieIdentifier to write to @p ds
 * @return @p ds
 */
QDataStream& operator<< (QDataStream &ds, const WebEnginePartCookieJar6::CookieIdentifier &id);


#endif // WEBENGINEPARTCOOKIEJAR_H
