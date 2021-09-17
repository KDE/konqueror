/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2008 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2009 Christoph Feck <christoph@maxiom.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqanimatedlogo_p.h"

#include <KIconLoader>

#include <QEvent>
#include <QToolBar>

KonqAnimatedLogo::KonqAnimatedLogo(QWidget *parent)
    : KAnimatedButton(parent)
{
    setAutoRaise(true);
    setFocusPolicy(Qt::NoFocus);
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    QToolBar *bar = qobject_cast<QToolBar *>(parent);
    if (bar) {
        connectToToolBar(bar);
    }
    setAnimatedLogoSize(iconSize());
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
    const int sizeToLoad = qMin(size.width(), size.height());
    setAnimationPath(KIconLoader::global()->iconPath(QStringLiteral("process-working-kde"), -sizeToLoad));
}
