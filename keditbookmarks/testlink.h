/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
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

#ifndef __testlink_h
#define __testlink_h

#include <QtCore/QObject>

#include <kio/job.h>
#include <kbookmark.h>

#include "bookmarkiterator.h"

class TestLinkItrHolder : public BookmarkIteratorHolder {
public:
   static TestLinkItrHolder* self() {
      if (!s_self) { s_self = new TestLinkItrHolder(); }; return s_self;
   }
   void addAffectedBookmark( const QString & address );
protected:
   virtual void doItrListChanged();
private:
   TestLinkItrHolder();
   static TestLinkItrHolder *s_self;
   QString m_affectedBookmark;
};

class TestLinkItr : public BookmarkIterator
{
   Q_OBJECT

public:
   TestLinkItr(QList<KBookmark> bks);
   ~TestLinkItr();
   virtual TestLinkItrHolder* holder() const { return TestLinkItrHolder::self(); }

public Q_SLOTS:
   void slotJobResult(KJob *job);

private:
   void setStatus(const QString & text);
   virtual void doAction();
   virtual bool isApplicable(const KBookmark &bk) const;

   KIO::TransferJob *m_job;
   QString m_oldStatus;
};

#endif
