/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef KONQURL_H
#define KONQURL_H

#include <QLatin1String>
#include <QString>
#include <QUrl>

/**
 * @brief Namespace for all things related to URLs with the `konq` scheme
 */
namespace KonqUrl {
    
    /**
     * @brief Enum class describing the different known URLs with the `konq` scheme
     *
     * Unlike other for schemes like `http` or `file`, there are only a small number
     * of recognized paths for `konq` URLs and they correspond to the values in
     * this enum class.
     */
    enum class Type{
        NoPath, //!< The `konq:` URL
        Blank, //!< The `konq:blank` URL
        Konqueror, //!< The `konq:konqueror` URL
        Launch, //!< The `konq:konqueror/launch` URL
        Specs, //!< The `konq:konqueror/specs` URL
        Intro, //!< The `konq:konqueror/intro` URL
        Tips, //!< The `konq:konqueror/tips` URL
        Plugins, //!< The `konq:plugins` URL
        SpeedDial, //!< The `konq:speedial` URL
        Temp //!< A special URL used when creating a new window or tab
    };
    
    /**
     * @brief The scheme for `konq` URLs
     *
     * @returns `"konq"`
     */
    QLatin1String scheme();

    /**
     * @brief The `konq` URL of a given type as a string
     *
     * @param type the type of `konq` URL
     * @return the string representation of the `konq` URL of type @p type
     */
    QLatin1String string(Type type = Type::NoPath);

    /**
     * @brief The `konq` URL of a given type
     *
     * This is a convenience function which is equivalent to calling `QUrl(KonqUrl::string(type))`.
     *
     * @param type the type of `konq` URL
     * @return an URL with the `konq` scheme and type @p type
     */
    QUrl url(Type type = Type::NoPath);

    /**
     * @brief Whether the given URL has the `konq` scheme
     *
     * This is a convenience function which is equivalent to calling `url.scheme() == KonqUrl::scheme()`
     *
     * @param url the URL to test
     * @return `true` if the scheme of @p url is `konq` and false otherwise
     */
    bool hasKonqScheme(const QUrl &url);

    /**
     * @brief Whether the given URL can be a `konq` URL
     *
     * @param url a string representation of the URL to test
     * @return `true` if @p url starts with `konq:` and `false` otherwise
     */
    bool canBeKonqUrl(const QString &url);

    /**
     * @brief Whether the given URL is a `konq` URL with a path whose first element is allowed for a `konq` URL
     *
     * @param url the URL to test
     * @return `true` if the URL is `konq:blank`, `konq:plugins` or if it starts with `konq:konqueror` or `konq:speedial`
     */
    bool hasKnownPathRoot(const QString &url);

    /**
     * @brief Whether the given URL is a valid `konq` URL different from `konq:blank`
     *
     * @param url the URL to test
     * @return `true` if @p url is a valid `konq` URL and it's not `konq:blank` and `false` otherwise
     * @note `konq:` is different from `konq:blank`, so this will return `true` if @p url is `konq:`
     */
    bool isValidNotBlank(const QUrl &url);

    /**
     * @brief Overload of isValidNotBlank(const QUrl&)
     */
    bool isValidNotBlank(const QString &url);

    /**
     * @brief Whether the given URL is `konq:blank`
     *
     * This is a convenience function equivalent to calling `url == string(KonqUrl::Type::Blank)`
     *
     * @param url the URL to test
     * @return `true` if @p url is `konq:blank` and `false` otherwise
     */
    bool isKonqBlank(const QUrl &url);

    /**
     * @brief Overload of isKonqBlank(const QUrl &)
     */
    bool isKonqBlank(const QString &url);
};

#endif // KONQURL_H
