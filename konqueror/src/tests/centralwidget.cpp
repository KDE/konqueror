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
*/

#include "centralwidget.h"

#include <QtGui/QApplication>
#include <QtGui/QLabel>
#include <QtCore/QTimer>
#include <QtGui/QSplitter>

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

int main( int argc, char** argv ) {
    QApplication app( argc, argv );

    SCWMainWindow* mw = new SCWMainWindow;
    mw->show();

    return app.exec();
}

#include "centralwidget.moc"
