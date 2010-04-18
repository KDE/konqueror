/* This file is part of the KDE project
   Copyright (C) 2000, 2010 David Faure <faure@kde.org>
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
#include "bookmarkmodel.h"
#include <kbookmarkmanager.h>

#include <kdebug.h>
#include <QtCore/QTimer>

BookmarkIterator::BookmarkIterator(BookmarkIteratorHolder* holder, const QList<KBookmark>& bks)
    : m_bookmarkList(bks), m_holder(holder)
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

KBookmark BookmarkIterator::currentBookmark()
{
    return m_bk;
}

void BookmarkIterator::nextOne()
{
    // kDebug() << "BookmarkIterator::nextOne";

    // Look for an interesting bookmark
    while (!m_bookmarkList.isEmpty()) {
        KBookmark bk = m_bookmarkList.takeFirst();
        if (bk.hasParent() && isApplicable(bk)) {
            m_bk = bk;
            doAction();
            // Async action started, we'll have to come back later
            return;
        }
    }
    if (m_bookmarkList.isEmpty()) {
        holder()->removeIterator(this); // deletes "this"
        return;
    }
}

KBookmarkModel* BookmarkIterator::model()
{
    return m_holder->model();
}

/* --------------------------- */

BookmarkIteratorHolder::BookmarkIteratorHolder(KBookmarkModel* model)
    : m_model(model)
{
    Q_ASSERT(m_model);
}

void BookmarkIteratorHolder::insertIterator(BookmarkIterator *itr)
{
    m_iterators.prepend(itr);
    doIteratorListChanged();
}

void BookmarkIteratorHolder::removeIterator(BookmarkIterator *itr)
{
    m_iterators.removeAll(itr);
    itr->deleteLater();
    doIteratorListChanged();
}

void BookmarkIteratorHolder::cancelAllItrs()
{
    Q_FOREACH(BookmarkIterator* iterator, m_iterators) {
        iterator->cancel();
    }
    qDeleteAll(m_iterators);
    m_iterators.clear();
    doIteratorListChanged();
}

void BookmarkIteratorHolder::addAffectedBookmark(const QString & address)
{
    kDebug() << address;
    if(m_affectedBookmark.isNull())
        m_affectedBookmark = address;
    else
        m_affectedBookmark = KBookmark::commonParent(m_affectedBookmark, address);
    kDebug() << "m_affectedBookmark is now" << m_affectedBookmark;
}

void BookmarkIteratorHolder::doIteratorListChanged()
{
    kDebug() << count() << "iterators";
    emit setCancelEnabled(count() > 0);
    if(count() == 0) {
        kDebug() << "Notifing managers" << m_affectedBookmark;
        KBookmarkManager* mgr = m_model->bookmarkManager();
        model()->notifyManagers(mgr->findByAddress(m_affectedBookmark).toGroup());
        m_affectedBookmark.clear();
    }
}

#include "bookmarkiterator.moc"
