/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2009 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef KONQ_HISTORYENTRY_H
#define KONQ_HISTORYENTRY_H

#include <QDateTime>
#include <QMetaType>
#include <QUrl>
#include "libkonq_export.h"

/**
 * @brief Class representing an entry in history
 *
 * An history entry contains information about when and how the user visited an URL
 * and information about the text the user entered (for example in the location bar)
 * to get to the URL and to the title of the resource associated with the URL.
 */
class LIBKONQ_EXPORT KonqHistoryEntry
{
public:
    KonqHistoryEntry(); //!< Default constructor

    /**
     * @brief Copy constructor
     *
     * @param other the entry to copy from
     */
    KonqHistoryEntry(const KonqHistoryEntry &other);

    ~KonqHistoryEntry(); //!< Destructor

    /**
     * @brief Assignement operator
     *
     * @param entry the other entry
     */
    KonqHistoryEntry &operator=(const KonqHistoryEntry &entry);

    QUrl url; //!< The URL which was visited
    QString typedUrl; //!< The text entered by the user and which resulted in @p url
    QString title; //!< The title of the entry
    quint32 numberOfTimesVisited; //!< The number of times the entry has been visited
    QDateTime firstVisited; //!< When the entry was first visited
    QDateTime lastVisited; //!< When the entry was last visited

    /**
     * @brief Comparison operator
     *
     * @param entry the entry to compare this to
     * @return `true` if this entry has all fields equal to @p entry and `false` otherwise
     *
     * @internal
     * This is (or was) only needed for `QList` on Windows
     * @endinternal
     */
    bool operator==(const KonqHistoryEntry &entry) const;

    /**
     * @brief Enum describing how to marshal the #url field when saving to a data stream
     */
    enum Flags {
        NoFlags = 0, //!< Marshal #url as a `QUrl`
        MarshalUrlAsStrings = 1 //!< Marshal #url as a `QString`
    };

    /**
     * @brief Loads an instance from a data stream
     *
     * @param s the data stream to load the instance from
     * @param flags how the url has been stored in the data stream
     */
    void load(QDataStream &s, Flags flags);

    /**
     * @brief Saves the instance to a data stream
     *
     * @param s the data stream to save the instance to
     * @param flags how to store #url in the data stream
     */
    void save(QDataStream &s, Flags flags) const;

private:
    class Private;
    Private *d; //!< Unused
};

#ifdef MAKE_KONQ_LIB
KDE_DUMMY_QHASH_FUNCTION(KonqHistoryEntry)
#endif

Q_DECLARE_METATYPE(KonqHistoryEntry)

/**
 * @brief List of KonqHistoryEntry
 *
 * It provides methods for finding and removing entries basing on their URL.
 */
class LIBKONQ_EXPORT KonqHistoryList : public QList<KonqHistoryEntry>
{
public:
    /**
     * @brief Finds an entry by URL
     *
     * @param url the URL to look for
     * @return an iterator to the first item whose URL is @p url or `end()` if
     * no such element is found
     */
    iterator findEntry(const QUrl &url);

    /**
     * @brief A const version of findEntry()
     *
     * @see findEntry()
     *
     * @param url the URL to look for
     * @return an iterator to the first item whose URL is @p url or `end()` if
     * no such element is found
     */
    const_iterator constFindEntry(const QUrl &url) const;

    /**
     * @brief Removes the entry with a given URL
     *
     * @param url the URL of the entry to remove
     */
    void removeEntry(const QUrl &url);
};

#endif /* KONQ_HISTORYENTRY_H */

