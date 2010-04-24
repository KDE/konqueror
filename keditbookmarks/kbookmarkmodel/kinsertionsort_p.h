// -*- c-basic-offset: 4; indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __kinsertionsort_h
#define __kinsertionsort_h

/**
 * A template-based insertion sort algorithm, but not really 100%
 * generic. It is mostly written for lists, not for arrays.
 *
 * A good reason to use insertion sort over faster algorithms like
 * heap sort or quick sort, is that it minimizes the number of
 * movements of the items. This is important in applications which support
 * undo, because the number of commands is kept to a minimum.
 */

// Item must define isNull(), previousSibling(), nextSibling()
// SortHelper must define  moveAfter( const Item &, const Item & )
// Criteria must define  static Key key(const Item &)
template <class Item, class Criteria, class Key, class SortHelper>
inline void kInsertionSort( Item& firstChild, SortHelper& sortHelper )
{
    if (firstChild.isNull()) return;
    Item j = firstChild.nextSibling();
    while ( !j.isNull() )
    {
        Key key = Criteria::key(j);
        // Insert A[j] into the sorted sequence A[1..j-1]
        Item i = j.previousSibling();
        bool moved = false;
        while ( !i.isNull() && Criteria::key(i) > key )
        {
            i = i.previousSibling();
            moved = true;
        }
        if ( moved )
            sortHelper.moveAfter( j, i ); // move j right after i. If i is null, move to first position.
        j = j.nextSibling();
    }
}

#endif
