/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
 * Copyright (C) 2008 Michael Howell <mhowell123@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "kwebview.h"
#include "kwebpage.h"
#include <KDE/KUrl>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QMouseEvent>

class KWebView::KWebViewPrivate
{
public:
    KWebViewPrivate()
    : customContextMenu(false)
    , keyboardModifiers(Qt::NoModifier)
    , pressedButtons(Qt::NoButton)
    {}
    bool customContextMenu;
    Qt::KeyboardModifiers keyboardModifiers;
    Qt::MouseButtons pressedButtons;
};


KWebView::KWebView(QWidget *parent)
    : QWebView(parent), d(new KWebView::KWebViewPrivate())
{
    setPage(new KWebPage(parent));
}

KWebView::~KWebView()
{
    delete d;
}

void KWebView::contextMenuEvent(QContextMenuEvent *event)
{
    if (!d->customContextMenu) {
        QWebView::contextMenuEvent(event);
    } else {
        emit showContextMenu(event);
    }
}

void KWebView::setCustomContextMenu(bool show)
{
    d->customContextMenu = show;
}

KWebPage *KWebView::page()
{
    KWebPage *webPage = qobject_cast<KWebPage*>(QWebView::page());
    if (!webPage) {
        return 0;
    }
    return webPage;
}

void KWebView::wheelEvent(QWheelEvent *event)
{
    if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
        int numDegrees = event->delta() / 8;
        int numSteps = numDegrees / 15;
        setTextSizeMultiplier(textSizeMultiplier() + numSteps * 0.1);
        event->accept();
        return;
    }
    QWebView::wheelEvent(event);
}


void KWebView::mousePressEvent(QMouseEvent *event)
{
    d->pressedButtons = event->buttons();
    d->keyboardModifiers = event->modifiers();
    QWebView::mousePressEvent(event);
}

void KWebView::mouseReleaseEvent(QMouseEvent *event)
{
    QWebView::mouseReleaseEvent(event);
    if (!event->isAccepted() && (d->pressedButtons & Qt::MidButton)) {
        KUrl url(QApplication::clipboard()->text(QClipboard::Selection));
        if (!url.isEmpty() && url.isValid() && !url.scheme().isEmpty()) {
            emit openUrl(url);
        }
    }
}

