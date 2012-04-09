/*
   This file is part of the KDE project
   Copyright (C) 2008 David Faure <faure@kde.org>
   Copyright (C) 2009 Christoph Feck <christoph@maxiom.de>

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

#include "konqanimatedlogo_p.h"

#include <KDE/KIconLoader>

#include <QtCore/QEvent>
#include <QToolBar>

KonqAnimatedLogo::KonqAnimatedLogo(QWidget *parent)
    : KAnimatedButton(parent)
{
    setAutoRaise(true);
    setFocusPolicy(Qt::NoFocus);
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    QToolBar * bar = qobject_cast<QToolBar *>(parent);
    if (bar) {
        connectToToolBar(bar);
    }
}

void KonqAnimatedLogo::changeEvent(QEvent *event)
{
    KAnimatedButton::changeEvent(event);
    if (event->type() == QEvent::ParentAboutToChange) {
        if (parentWidget()) {
            disconnect(parentWidget(), SIGNAL(iconSizeChanged(QSize)), this, SLOT(setAnimatedLogoSize()));
        }
    } else if (event->type() == QEvent::ParentChange) {
        QToolBar *bar = qobject_cast<QToolBar *>(parentWidget());
        if (bar) {
            connectToToolBar(bar);
        }
    }
}

void KonqAnimatedLogo::connectToToolBar(QToolBar *bar)
{
    setAnimatedLogoSize(bar->iconSize());
    connect(bar, SIGNAL(iconSizeChanged(QSize)), SLOT(setAnimatedLogoSize(QSize)));
}

void KonqAnimatedLogo::setAnimatedLogoSize(const QSize &size)
{
    setIconSize(size);
    updateIcons();
}

#include "konqanimatedlogo_p.moc"
