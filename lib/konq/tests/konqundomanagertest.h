/* This file is part of KDE
    Copyright (c) 2006 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KONQUNDOMANAGERTEST_H
#define KONQUNDOMANAGERTEST_H

#include <QObject>
#include <QEventLoop>

class KonqUndomanagerTest : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    void testCopyFiles();
    void testMoveFiles();
    //void testCopyFilesOverwrite();
    void testCopyDirectory();
    // TODO testTrashFiles

private:
    void doUndo();
    QEventLoop m_eventLoop;
};

#endif
