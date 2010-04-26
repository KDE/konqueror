// -*- c-basic-offset: 4; indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
/* This file is part of the KDE project
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) version 3.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#ifndef __favicons_h
#define __favicons_h

#include <kbookmark.h>

#include "bookmarkiterator.h"

class FavIconsItrHolder : public BookmarkIteratorHolder {
public:
   FavIconsItrHolder(QObject* parent, KBookmarkModel* model);
};

class KBookmarkModel;
class FavIconUpdater;

class FavIconsItr : public BookmarkIterator
{
   Q_OBJECT

public:
   FavIconsItr(BookmarkIteratorHolder* holder, const QList<KBookmark>& bks);
   ~FavIconsItr();

   virtual void cancel();

public Q_SLOTS:
   void slotDone(bool succeeded, const QString& errorString);

protected:
   virtual void doAction();
   virtual bool isApplicable(const KBookmark &bk) const;

private:
   void setStatus(const QString & status);
   FavIconUpdater *m_updater;
   QString m_oldStatus;
};

#endif

