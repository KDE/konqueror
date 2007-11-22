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

#ifndef CENTRALWIDGET_H
#define CENTRALWIDGET_H

#include <QtGui/QMainWindow>

// SCW == Switch (or Set) Central Widget
class SCWMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    SCWMainWindow( QWidget* parent = 0 );

private slots:
    void slotSwitchCentralWidget();
};

#endif
