/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KONQINTERFACES_COOKIEJAR_H
#define KONQINTERFACES_COOKIEJAR_H

#include "libkonq_export.h"

#include "konqsettings.h"

#include <QObject>
#include <QSet>
#include <QUrl>
#include <QVector>
#include <QNetworkCookie>

class KConfigGroup;

namespace KonqInterfaces {

/**
 * @brief Abstract class describing a Konqueror cookie jar, that is an object which
 * stores and manipulates cookies
 */
class LIBKONQ_EXPORT CookieJar : public QObject
{
    Q_OBJECT

public:

    /**
     * Default constructor
     *
     * @param parent the parent object
     */
    CookieJar(QObject* parent=nullptr);
    virtual ~CookieJar(); ///< Destructor

    /**
     * @brief A set of all cookies in the cookie jar
     * @return A set of all cookies in the cookie jar
     */
    virtual QSet<QNetworkCookie> cookies() const = 0;

    using CookieAdvice = Konq::Settings::CookieAdvice;

    /**
     * @brief Helper function to write a `KConfig` entry from a CookieAdvice
     *
     * It's a shortcut for
     * @code
     * grp.writeEntry(key, static_cast<int>(advice));
     * @endcode
     * @param grp the group where the entry should be written
     * @param key the key of the entry
     * @param advice the value to write
     */
    static void writeAdviceConfigEntry(KConfigGroup &grp, const char* key, CookieAdvice advice);

    /**
     * @brief Adds an exception to the default cookie policy for a given domain
     * @param domain the domain of the cookie
     * @param advice how to handle cookies coming from @p domain
     */

    virtual void addDomainException(const QString &domain, CookieAdvice advice) = 0;

    /**
     * @brief Adds an exception to the default cookie policy for a given cookie
     * @param name the name of the cookie
     * @param domain the domain of the cookie
     * @param path the path of the cookie
     * @param advice how to handle cookies coming from @p domain
     */
    virtual void addCookieException(const QString &name, const QString &domain, const QString &path, CookieAdvice advice) = 0;

    /**
     * @brief How to handle cookies from the given domain
     *
     * @param domain the domain
     * @return how to handle cookies from @p domain. If the user chose a specific handling of cookies from @p domain,
     * the corresponding value is returned, otherwise the default behavior is returned
     */
    virtual CookieAdvice adviceForDomain(const QString &domain) const = 0;

    /**
     * @brief How to handle a specific cookie
     * @param name the name of the cookie
     * @param domain the domain of the cookie
     * @param path the path of the cookie
     * @return how to handle the given cookie. If the user chose a specific way to handle that cookie, the corresponding
     * value is returned. Otherwise the return value will be the same as calling adviceForDomain with @p domain
     */
    virtual CookieAdvice adviceForCookie(const QString &name, const QString &domain, const QString &path) const = 0;

    /**
     * @brief Whether or not cookies are globally enabled
     * @return whether or not cookies are globally enabled
     */
    virtual bool areCookiesEnabled() const = 0;

public slots:
    /**
     * @brief Removes all cookies from the cookie jar
     *
     * It's not required that implementations be synchronous: this function can return immediately but remove cookies at a later time
     */
    virtual void removeAllCookies() = 0;

    /**
     * @brief Removes all cookies with a given domain from the cookie jar
     *
     * It's not required that implementations be synchronous: this function can return immediately but remove cookies at a later time
     * @param domain the domain of the cookies to remove
     */
    virtual void removeCookiesWithDomain(const QString &domain) = 0;

    /**
     * @brief Removes the given cookies from the cookie jar
     *
     * The default implementation simply calls removeCookie() for each of the given cookies.
     *
     * When needing to remove more than one cookie, it's better to call this rather than calling removeCookie() multiple times, as
     * other implementations of this function can be more optimized.
     *
     * It's not required that implementations be synchronous: this function can return immediately but remove cookies at a later time
     * @param cookies the cookie to remove
     */
    virtual void removeCookies(const QVector<QNetworkCookie> &cookies);

    /**
     * @brief Removes a given cookie from the cookie jar
     *
     * It's not required that implementations be synchronous: this function can return immediately but remove cookies at a later time
     * @param cookie the cookie to remove
     */
    virtual void removeCookie(const QNetworkCookie &cookie, const QUrl &origin=QUrl()) = 0;

    /**
     * @brief Removes all session cookies from the cookie jar
     *
     * Session cookies are cookies without an expiry date. They're supposed to exist only until the application is closed
     *
     * It's not required that implementations be synchronous: this function can return immediately but remove cookies at a later time
     */
    virtual void removeSessionCookies() = 0;
};

}

#endif // KONQINTERFACES_COOKIEJAR_H
