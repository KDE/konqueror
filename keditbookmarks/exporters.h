/* This file is part of the KDE project
   Copyright (C) 2003 Alexander Kellett <lypanov@kde.org>

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

#ifndef __exporters_h
#define __exporters_h

#include <kbookmark.h>
//Added by qt3to4:
#include <QtCore/QTextStream>

class HTMLExporter : private KBookmarkGroupTraverser {
public:
   HTMLExporter();
   virtual ~HTMLExporter(){}
   QString toString(const KBookmarkGroup &, bool showAddress = false);
   void write(const KBookmarkGroup &, const QString &, bool showAddress = false);
private:
   virtual void visit(const KBookmark &);
   virtual void visitEnter(const KBookmarkGroup &);
   virtual void visitLeave(const KBookmarkGroup &);
private:
   QString m_string;
   QTextStream m_out;
   bool m_showAddress;
};

#endif
