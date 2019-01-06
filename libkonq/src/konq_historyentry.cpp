/* This file is part of the KDE project
   Copyright 2009 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2 of the License or
   ( at your option ) version 3 or, at the discretion of KDE e.V.
   ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
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

