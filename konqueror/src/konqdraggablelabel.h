/* This file is part of the KDE project
   Copyright 2002 John Firebaugh <jfirebaugh@kde.org>

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

#include <kurl.h>
#include <QtGui/QLabel>
class KonqMainWindow;

class KonqDraggableLabel : public QLabel
{
    Q_OBJECT
public:
    KonqDraggableLabel( KonqMainWindow * mw, const QString & text );

protected:
    void mousePressEvent( QMouseEvent * ev );
    void mouseMoveEvent( QMouseEvent * ev );
    void mouseReleaseEvent( QMouseEvent * );
    void dragEnterEvent( QDragEnterEvent *ev );
    void dropEvent( QDropEvent* ev );

private Q_SLOTS:
    void delayedOpenURL();

private:
    QPoint startDragPos;
    bool validDrag;
    KonqMainWindow * m_mw;
    KUrl::List _savedLst;
};
