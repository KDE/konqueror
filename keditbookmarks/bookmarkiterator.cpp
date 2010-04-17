// -*- indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 or at your option version 3 as published by
   the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "bookmarkiterator.h"

#include "toplevel.h"

#include <kdebug.h>
#include <QtCore/QTimer>
#include <assert.h>

BookmarkIterator::BookmarkIterator(BookmarkIteratorHolder* holder, const QList<KBookmark>& bks)
    : m_bklist(bks), m_holder(holder)
{
    delayedEmitNextOne();
}

BookmarkIterator::~BookmarkIterator()
{
}

void BookmarkIterator::delayedEmitNextOne()
{
    QTimer::singleShot(1, this, SLOT(nextOne()));
}

KBookmark BookmarkIterator::curBk()
{
    return m_bk;
}

void BookmarkIterator::nextOne()
{
    // kDebug() << "BookmarkIterator::nextOne";

    if (m_bklist.isEmpty()) {
        holder()->removeItr(this); // deletes "this"
        return;
    }

    QList<KBookmark>::iterator head = m_bklist.begin();
    KBookmark bk = (*head);

    bool viable = bk.hasParent() && isApplicable(bk);

    if (viable) {
        m_bk = bk;
        doAction();
    }

    m_bklist.erase(head);

    if (!viable)
        delayedEmitNextOne();
}

/* --------------------------- */

BookmarkIteratorHolder::BookmarkIteratorHolder()
{
}

void BookmarkIteratorHolder::insertItr(BookmarkIterator *itr)
{
    m_itrs.prepend(itr);
    doItrListChanged();
}

void BookmarkIteratorHolder::removeItr(BookmarkIterator *itr)
{
    m_itrs.removeAll(itr);
    itr->deleteLater();
    doItrListChanged();
}

void BookmarkIteratorHolder::cancelAllItrs()
{
    qDeleteAll(m_itrs);
    m_itrs.clear();
    doItrListChanged();
}

#include "bookmarkiterator.moc"
