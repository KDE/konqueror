/* This file is part of the KDE project
   Copyright (C) 2006 David Faure <faure@kde.org>

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
   Boston, MA 02110-1301, USA.
*/

#ifndef KONQVIEWMGRTEST_H
#define KONQVIEWMGRTEST_H

#include <QtGui/QMainWindow>

#include <QObject>
#include <kcomponentdata.h>

class ViewMgrTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void testCreateFirstView();
    void testRemoveFirstView();
    void testSplitView();
    void testSplitMainContainer();

    void testAddTab();
    void testDuplicateTab();
    void testDuplicateSplittedTab();
    void testDeletePartInTab();

    void testLoadProfile();

private:
    KComponentData m_konqComponentData;
};

#endif
