/* This file is part of the KDE project
   Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_historyentry.h"

bool KonqHistoryEntry::marshalURLAsStrings;

// QDataStream operators (read and write a KonqHistoryEntry
// from/into a QDataStream)
QDataStream& operator<< (QDataStream& s, const KonqHistoryEntry& e) {
    if (KonqHistoryEntry::marshalURLAsStrings)
	s << e.url.url();
    else
	s << e.url;

    s << e.typedUrl;
    s << e.title;
    s << e.numberOfTimesVisited;
    s << e.firstVisited;
    s << e.lastVisited;

    return s;
}

QDataStream& operator>> (QDataStream& s, KonqHistoryEntry& e) {
    if (KonqHistoryEntry::marshalURLAsStrings)
    {
	QString url;
	s >> url;
	e.url = url;
    }
    else
    {
	s>>e.url;
    }

    s >> e.typedUrl;
    s >> e.title;
    s >> e.numberOfTimesVisited;
    s >> e.firstVisited;
    s >> e.lastVisited;

    return s;
}
