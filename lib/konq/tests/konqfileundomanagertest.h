/* This file is part of KDE
    Copyright (c) 2006 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KONQFILEUNDOMANAGERTEST_H
#define KONQFILEUNDOMANAGERTEST_H

#include <QObject>
#include <QEventLoop>
class TestUiInterface;

class KonqFileUndoManagerTest : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    void testCopyFiles();
    void testMoveFiles();
    void testCopyDirectory();
    void testMoveDirectory();
    void testRenameFile();
    void testRenameDir();
    void testTrashFiles();
    void testModifyFileBeforeUndo(); // #20532

    // TODO find tests that would lead to kio job errors

    // TODO test renaming during a CopyJob.
    // Doesn't seem possible though, requires user interaction...

    // TODO: add test for undoing after a partial move (http://bugs.kde.org/show_bug.cgi?id=91579)
    // Difficult too.

private:
    void doUndo();
    TestUiInterface* m_uiInterface;
};

#endif
