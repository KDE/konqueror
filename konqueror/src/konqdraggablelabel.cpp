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

#include "konqdraggablelabel.h"
#include "konqmainwindow.h"
#include "konqview.h"
#include <QMouseEvent>
#include <QApplication>


KonqDraggableLabel::KonqDraggableLabel( KonqMainWindow* mw, const QString& text )
  : QLabel( text )
  , m_mw(mw)
{
  setBackgroundRole( QPalette::Button );
  setAlignment( (QApplication::isRightToLeft() ? Qt::AlignRight : Qt::AlignLeft) |
                 Qt::AlignVCenter );
  setAcceptDrops(true);
  adjustSize();
  validDrag = false;
}

void KonqDraggableLabel::mousePressEvent( QMouseEvent * ev )
{
  validDrag = true;
  startDragPos = ev->pos();
}

void KonqDraggableLabel::mouseMoveEvent( QMouseEvent * ev )
{
  if ((startDragPos - ev->pos()).manhattanLength() > QApplication::startDragDistance())
  {
    validDrag = false;
    if ( m_mw->currentView() )
    {
      KUrl::List lst;
      lst.append( m_mw->currentView()->url() );
      QDrag* drag = new QDrag( m_mw );
      QMimeData* md = new QMimeData();
      lst.populateMimeData( md );
      drag->setMimeData( md );
      QString iconName = KMimeType::iconNameForUrl( lst.first() );

      drag->setPixmap(KIconLoader::global()->loadMimeTypeIcon(iconName, KIconLoader::Small));

      drag->start();
    }
  }
}

void KonqDraggableLabel::mouseReleaseEvent( QMouseEvent * )
{
  validDrag = false;
}

void KonqDraggableLabel::dragEnterEvent( QDragEnterEvent *ev )
{
  if ( KUrl::List::canDecode( ev->mimeData() ) )
    ev->accept();
}

void KonqDraggableLabel::dropEvent( QDropEvent* ev )
{
    _savedLst.clear();
    _savedLst = KUrl::List::fromMimeData( ev->mimeData() );
    if ( !_savedLst.isEmpty() ) {
        QMetaObject::invokeMethod(this, "delayedOpenURL", Qt::QueuedConnection);
    }
}

void KonqDraggableLabel::delayedOpenURL()
{
    m_mw->openUrl( 0L, _savedLst.first() );
}

