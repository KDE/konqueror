/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2009 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "konq_historyentry.h"
#include <QDataStream>

KonqHistoryEntry::KonqHistoryEntry()
    : numberOfTimesVisited(1), d(nullptr)
{
}

KonqHistoryEntry::~KonqHistoryEntry()
{
}

KonqHistoryEntry::KonqHistoryEntry(const KonqHistoryEntry &other)
{
    operator=(other);
}

KonqHistoryEntry &KonqHistoryEntry::operator=(const KonqHistoryEntry &other)
{
    url = other.url;
    typedUrl = other.typedUrl;
    title = other.title;
    numberOfTimesVisited = other.numberOfTimesVisited;
    firstVisited = other.firstVisited;
    lastVisited = other.lastVisited;
    d = nullptr;
    return *this;
}

bool KonqHistoryEntry::operator==(const KonqHistoryEntry &entry) const
{
    return url == entry.url &&
           typedUrl == entry.typedUrl &&
           title == entry.title &&
           numberOfTimesVisited == entry.numberOfTimesVisited &&
           firstVisited == entry.firstVisited &&
           lastVisited == entry.lastVisited;
}

void KonqHistoryEntry::load(QDataStream &s, Flags flags)
{
    if (flags & MarshalUrlAsStrings) {
        QString urlStr;
        s >> urlStr;
        url = QUrl(urlStr);
    } else {
        s >> url;
    }
    s >> typedUrl;
    s >> title;
    s >> numberOfTimesVisited;
    s >> firstVisited;
    s >> lastVisited;
}

void KonqHistoryEntry::save(QDataStream &s, Flags flags) const
{
    if (flags & MarshalUrlAsStrings) {
        s << url.url();
    } else {
        s << url;
    }
    s << typedUrl;
    s << title;
    s << numberOfTimesVisited;
    s << firstVisited;
    s << lastVisited;
}

////

KonqHistoryList::iterator KonqHistoryList::findEntry(const QUrl &url)
{
    // we search backwards, probably faster to find an entry
    KonqHistoryList::iterator it = end();
    while (it != begin()) {
        --it;
        if ((*it).url == url) {
            return it;
        }
    }
    return end();
}

KonqHistoryList::const_iterator KonqHistoryList::constFindEntry(const QUrl &url) const
{
    // we search backwards, probably faster to find an entry
    KonqHistoryList::const_iterator it = constEnd();
    while (it != constBegin()) {
        --it;
        if ((*it).url == url) {
            return it;
        }
    }
    return constEnd();
}

void KonqHistoryList::removeEntry(const QUrl &url)
{
    iterator it = findEntry(url);
    if (it != end()) {
        erase(it);
    }
}

