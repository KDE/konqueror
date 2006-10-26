/* This file is part of the KDE project
   Copyright (C) 2006 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konqviewmgrtest.h"

#include <QApplication>
#include <QLabel>
#include <QTimer>
#include <QSplitter>

SCWMainWindow::SCWMainWindow( QWidget* parent )
    : QMainWindow( parent )
{
    QLabel* widget1 = new QLabel( "widget1" );
    setCentralWidget( widget1 );
    QTimer::singleShot( 10, this, SLOT( slotSwitchCentralWidget() ) );
}

void SCWMainWindow::slotSwitchCentralWidget()
{
    QLabel* widget2 = new QLabel( "widget2" );
    delete centralWidget(); // ## workaround for the crash
    setCentralWidget( widget2 );
}

void testSplitterChildren()
{
    // OK, it works; the bug was a wrong insertWidget(0,label2) in konqviewmgr.

    QSplitter* container = new QSplitter( 0 );

    QLabel* label0 = new QLabel( "Label0, sidebar, should be on left", container );
    label0->show();

    QLabel* label1 = new QLabel( "Label1, iconview, should be on the right", container );
    label1->show();
    label1->setParent( 0 ); // should be some toplevel, rather, see convertDocContainer
    label1->hide();

    QLabel* label2 = new QLabel( "Label2, tabwidget, should be on the right", container );
    label2->show();

    container->show();
}

int main( int argc, char** argv ) {
    QApplication app( argc, argv );

    //SCWMainWindow* mw = new SCWMainWindow;
    //mw->show();

    testSplitterChildren();

    return app.exec();
}

#include "konqviewmgrtest.moc"
