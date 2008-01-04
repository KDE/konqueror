/* This file is part of KDE
    Copyright (c) 2007 David Faure <faure@kde.org>

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

#ifndef KONQPOPUPMENUTEST_H
#define KONQPOPUPMENUTEST_H

#include <QObject>
#include <KFileItem>
#include <KActionCollection>
class KNewMenu;

class KonqPopupMenuTest : public QObject
{
    Q_OBJECT
 public:
    KonqPopupMenuTest();

private slots:
    void initTestCase();
    void testFile();
    void testFilePreviewSubMenu();
    void testSubDirectory();
    void testViewDirectory();

    void testHtmlLink();
    void testHtmlPage();

private:
    KFileItem m_fileItem;
    KFileItem m_linkItem;
    KFileItem m_subDirItem;
    KFileItem m_rootItem;
    QAction* m_back;
    QAction* m_forward;
    QAction* m_up;
    QAction* m_reload;
    QAction* m_cut;
    QAction* m_copy;
    QAction* m_paste;
    QAction* m_pasteTo;
    QAction* m_properties;
    QAction* m_rename;
    QAction* m_trash;
    QAction* m_newWindow;
    QAction* m_newTab;
    QAction* m_preview1;
    QAction* m_preview2;
    QActionGroup* m_tabHandlingActions;
    QActionGroup* m_previewActions;
    QActionGroup* m_htmlEditActions;
    QActionGroup* m_fileEditActions;
    QActionGroup* m_linkActions;
    QActionGroup* m_partActions;
    KNewMenu* m_newMenu;
    KActionCollection m_actionCollection;
};

#endif
