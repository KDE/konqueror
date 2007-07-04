/*
   This file is part of the KDE project
   Copyright (C) 2007 David Faure <faure@kde.org>

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

#ifndef KONQ_FRAMEVISITOR_H
#define KONQ_FRAMEVISITOR_H

#include <QList>

class KonqFrameBase;
class KonqView;
class KonqFrame;
class KonqFrameContainer;
class KonqFrameTabs;
class KonqMainWindow;

class KonqFrameVisitor
{
public:
    KonqFrameVisitor() {}
    virtual ~KonqFrameVisitor() {}
    virtual bool visit(KonqFrame*) { return true; }
    virtual bool visit(KonqFrameContainer*) { return true; }
    virtual bool visit(KonqFrameTabs*) { return true; }
    virtual bool visit(KonqMainWindow*) { return true; }

    virtual bool endVisit(KonqFrameContainer*) { return true; }
    virtual bool endVisit(KonqFrameTabs*) { return true; }
    virtual bool endVisit(KonqMainWindow*) { return true; }
};

class KonqViewCollector : public KonqFrameVisitor
{
public:
    static QList<KonqView *> collect(KonqFrameBase* topLevel);
    virtual bool visit(KonqFrame* frame);
private:
    QList<KonqView *> m_views;
};

#endif /* KONQ_FRAMEVISITOR_H */

