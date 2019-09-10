/* This file is part of the KDE libraries
    Copyright (C) 2003 Stephan Binner <binner@kde.org>
    Copyright (C) 2003 Zack Rusin <zack@kde.org>
    Copyright (C) 2009 Urs Wolfer <uwolfer @ kde.org>

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

#include "ktabbar.h"

#include <QTimer>
#include <QApplication>
#include <QCursor>
#include <QMouseEvent>

class Q_DECL_HIDDEN KTabBar::Private
{
public:
    Private()
        :
          mDragSwitchTab(-1),
          mActivateDragSwitchTabTimer(nullptr),
          mMiddleMouseTabMoveInProgress(false)
    {
    }

    QPoint mDragStart;
    int mDragSwitchTab;
    QTimer *mActivateDragSwitchTabTimer;

    bool mMiddleMouseTabMoveInProgress : 1;

};

KTabBar::KTabBar(QWidget *parent)
    : QTabBar(parent),
      d(new Private)
{
    setAcceptDrops(true);
    setMouseTracking(true);

    d->mActivateDragSwitchTabTimer = new QTimer(this);
    d->mActivateDragSwitchTabTimer->setSingleShot(true);
    connect(d->mActivateDragSwitchTabTimer, SIGNAL(timeout()), SLOT(activateDragSwitchTab()));
}

KTabBar::~KTabBar()
{
    delete d;
}

void KTabBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    int tab = selectTab(event->pos());

    if (tab == -1) {
        emit newTabRequest();
    } else {
        emit tabDoubleClicked(tab);
    }

    QTabBar::mouseDoubleClickEvent(event);
}

void KTabBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        d->mDragStart = event->pos();
    } else if (event->button() == Qt::RightButton) {
        int tab = selectTab(event->pos());
        if (tab != -1) {
            emit contextMenu(tab, mapToGlobal(event->pos()));
        } else {
            emit emptyAreaContextMenu(mapToGlobal(event->pos()));
        }
        return;
    }

    QTabBar::mousePressEvent(event);
}

void KTabBar::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton && !isMovable()) {
        int tab = selectTab(event->pos());
        if (d->mDragSwitchTab && tab != d->mDragSwitchTab) {
            d->mActivateDragSwitchTabTimer->stop();
            d->mDragSwitchTab = 0;
        }

        int delay = QApplication::startDragDistance();
        QPoint newPos = event->pos();
        if (newPos.x() > d->mDragStart.x() + delay || newPos.x() < d->mDragStart.x() - delay ||
                newPos.y() > d->mDragStart.y() + delay || newPos.y() < d->mDragStart.y() - delay) {
            if (tab != -1) {
                emit initiateDrag(tab);
                return;
            }
        }
    }

    QTabBar::mouseMoveEvent(event);
}

void KTabBar::activateDragSwitchTab()
{
    int tab = selectTab(mapFromGlobal(QCursor::pos()));
    if (tab != -1 && d->mDragSwitchTab == tab) {
        setCurrentIndex(d->mDragSwitchTab);
    }

    d->mDragSwitchTab = 0;
}

void KTabBar::dragEnterEvent(QDragEnterEvent *event)
{
    int tab = selectTab(event->pos());
    if (tab != -1) {
        bool accept = false;
        // The receivers of the testCanDecode() signal has to adjust
        // 'accept' accordingly.
        emit testCanDecode(event, accept);
        if (accept && tab != currentIndex()) {
            d->mDragSwitchTab = tab;
            d->mActivateDragSwitchTabTimer->start(QApplication::doubleClickInterval() * 2);
        }

        event->setAccepted(accept);
        return;
    }

    QTabBar::dragEnterEvent(event);
}

void KTabBar::dragMoveEvent(QDragMoveEvent *event)
{
    int tab = selectTab(event->pos());
    if (tab != -1) {
        bool accept = false;
        // The receivers of the testCanDecode() signal has to adjust
        // 'accept' accordingly.
        emit testCanDecode(event, accept);
        if (accept && tab != currentIndex()) {
            d->mDragSwitchTab = tab;
            d->mActivateDragSwitchTabTimer->start(QApplication::doubleClickInterval() * 2);
        }

        event->setAccepted(accept);
        return;
    }

    QTabBar::dragMoveEvent(event);
}

void KTabBar::dropEvent(QDropEvent *event)
{
    int tab = selectTab(event->pos());
    if (tab != -1) {
        d->mActivateDragSwitchTabTimer->stop();
        d->mDragSwitchTab = 0;
        emit receivedDropEvent(tab, event);
        return;
    }

    QTabBar::dropEvent(event);
}

#ifndef QT_NO_WHEELEVENT
void KTabBar::wheelEvent(QWheelEvent *event)
{
    if (!(event->orientation() == Qt::Horizontal)) {
        if (receivers(SIGNAL(wheelDelta(int)))) {
            emit(wheelDelta(event->delta()));
            return;
        }
        int lastIndex = count() - 1;
        //Set an invalid index as base case
        int targetIndex = -1;
        bool forward = event->delta() < 0;
        if (forward && lastIndex == currentIndex()) {
            targetIndex = 0;
        } else if (!forward && 0 == currentIndex()) {
            targetIndex = lastIndex;
        }
        //Will not move when targetIndex is invalid
        setCurrentIndex(targetIndex);
        //If it has not moved yet (targetIndex == -1), or if it moved but current tab is disabled
        if (targetIndex != currentIndex() || !isTabEnabled(targetIndex)) {
            QTabBar::wheelEvent(event);
        }
        event->accept();
    } else {
        event->ignore();
    }
}
#endif

void KTabBar::tabLayoutChange()
{
    d->mActivateDragSwitchTabTimer->stop();
    d->mDragSwitchTab = 0;
}

int KTabBar::selectTab(const QPoint &pos) const
{
    const int tabCount = count();
    for (int i = 0; i < tabCount; ++i)
        if (tabRect(i).contains(pos)) {
            return i;
        }

    return -1;
}

