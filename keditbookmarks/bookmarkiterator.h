/* This file is part of the KDE project
   Copyright (C) 2010 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 or at your option version 3 as published by
   the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#ifndef __bookmarkiterator_h
#define __bookmarkiterator_h

#include <QtCore/QObject>
#include <QtCore/QList>
#include <kbookmark.h>

class KBookmarkModel;
class BookmarkIteratorHolder;

/**
 * A bookmark iterator goes through every bookmark and performs an asynchronous
 * action (e.g. downloading the favicon or testing whether the url exists).
 */
class BookmarkIterator : public QObject
{
    Q_OBJECT

public:
    BookmarkIterator(BookmarkIteratorHolder* holder, const QList<KBookmark>& bks);
    virtual ~BookmarkIterator();
    BookmarkIteratorHolder* holder() const { return m_holder; }
    KBookmarkModel* model();
    void delayedEmitNextOne();
    virtual void cancel() = 0;

public Q_SLOTS:
    void nextOne();

protected:
    virtual void doAction() = 0;
    virtual bool isApplicable(const KBookmark &bk) const = 0;
    KBookmark currentBookmark();

private:
    KBookmark m_bk;
    QList<KBookmark> m_bookmarkList;
    BookmarkIteratorHolder* m_holder;
};

/**
 * The "bookmark iterator holder" handles all concurrent iterators for a given
 * functionality: e.g. all favicon iterators.
 *
 * BookmarkIteratorHolder is the base class for the favicon and testlink holders.
 */
class BookmarkIteratorHolder : public QObject
{
    Q_OBJECT
public:
    void cancelAllItrs();
    void removeIterator(BookmarkIterator*);
    void insertIterator(BookmarkIterator*);
    void addAffectedBookmark(const QString & address);
    KBookmarkModel* model() { return m_model; }

Q_SIGNALS:
    void setCancelEnabled(bool canCancel);

protected:
    BookmarkIteratorHolder(KBookmarkModel* model);
    virtual ~BookmarkIteratorHolder() {}
    void doIteratorListChanged();
    int count() const { return m_iterators.count(); }
    KBookmarkModel* m_model;

private:
    QString m_affectedBookmark;
    QList<BookmarkIterator *> m_iterators;
};

#endif
