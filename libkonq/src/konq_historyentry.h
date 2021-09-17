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

class LIBKONQ_EXPORT KonqHistoryEntry
{
public:
    KonqHistoryEntry();
    KonqHistoryEntry(const KonqHistoryEntry &other);
    ~KonqHistoryEntry();

    KonqHistoryEntry &operator=(const KonqHistoryEntry &entry);

    QUrl url;
    QString typedUrl;
    QString title;
    quint32 numberOfTimesVisited;
    QDateTime firstVisited;
    QDateTime lastVisited;

    // Necessary for QList (on Windows)
    bool operator==(const KonqHistoryEntry &entry) const;

    enum Flags { NoFlags = 0, MarshalUrlAsStrings = 1 };
    void load(QDataStream &s, Flags flags);
    void save(QDataStream &s, Flags flags) const;

private:
    class Private;
    Private *d;
};

#ifdef MAKE_KONQ_LIB
KDE_DUMMY_QHASH_FUNCTION(KonqHistoryEntry)
#endif

Q_DECLARE_METATYPE(KonqHistoryEntry)

class LIBKONQ_EXPORT KonqHistoryList : public QList<KonqHistoryEntry>
{
public:
    /**
     * Finds an entry by URL and return an iterator to it.
     * If no matching entry is found, end() is returned.
     */
    iterator findEntry(const QUrl &url);

    /**
     * Finds an entry by URL and return an iterator to it.
     * If no matching entry is found, end() is returned.
     */
    const_iterator constFindEntry(const QUrl &url) const;

    /**
     * Finds an entry by URL and removes it
     */
    void removeEntry(const QUrl &url);
};

#endif /* KONQ_HISTORYENTRY_H */

