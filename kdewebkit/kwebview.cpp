/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
 * Copyright (C) 2008 Michael Howell <mhowell123@gmail.com>
 * Copyright (C) 2009 Dawit Alemayehu <adawit @ kde.org>
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

#include <kurl.h>
#include <kdebug.h>
#include <kio/global.h>
#include <kparts/part.h> // Where's the Qt includes?
#include <kparts/browserextension.h>
#include <kdeversion.h>

#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QMouseEvent>
#include <QtWebKit/QWebFrame>
#include <QtNetwork/QNetworkRequest>

class KWebView::KWebViewPrivate
{
public:
    KWebViewPrivate()
    : keyboardModifiers(Qt::NoModifier) , pressedButtons(Qt::NoButton) {}

    Qt::KeyboardModifiers keyboardModifiers;
    Qt::MouseButtons pressedButtons;
};


KWebView::KWebView(QWidget *parent)
         :QWebView(parent), d(new KWebView::KWebViewPrivate())
{
    setPage(new KWebPage(this));
}

KWebView::~KWebView()
{
    delete d;
}

bool KWebView::isExternalContentAllowed() const
{
    KWebPage *webPage = qobject_cast<KWebPage*>(page());
    if (webPage)
        return webPage->isExternalContentAllowed();
    return false;
}

void KWebView::setAllowExternalContent(bool allow)
{
    KWebPage *webPage = qobject_cast<KWebPage*>(page());
    if (webPage)
      webPage->setAllowExternalContent(allow);
}

void KWebView::wheelEvent(QWheelEvent *event)
{
    if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
        const int numDegrees = event->delta() / 8;
        const int numSteps = numDegrees / 15;
#if QT_VERSION < 0x040500
        setTextSizeMultiplier(textSizeMultiplier() + numSteps * 0.1);
#else
        setZoomFactor(zoomFactor() + numSteps * 0.1);
#endif
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
    const QWebHitTestResult result = page()->mainFrame()->hitTestContent(event->pos());
    const QUrl url = result.linkUrl();
    if (!url.isEmpty()) {
        if ((d->pressedButtons & Qt::MidButton) ||
            ((d->pressedButtons & Qt::LeftButton) && (d->keyboardModifiers & Qt::ControlModifier))) {
          kDebug() << "middle clicked or ctrl-clicked url" << url;
          emit openUrlInNewTab(url);
          return;
        }

       if ((d->pressedButtons & Qt::LeftButton) && (d->keyboardModifiers & Qt::ShiftModifier)) {
          kDebug() << "shift-clicked url" << url;
          emit saveUrl(url);
          return;
        }
    }

    QWebView::mouseReleaseEvent(event);

    // just leave if the site has not modified by the user (for example pasted text with mouse middle click)
    if (!isModified() && d->pressedButtons & Qt::MidButton) {
        const QString clipboardText(QApplication::clipboard()->text(QClipboard::Selection));
        KUrl url(clipboardText);
        if (!url.isEmpty() && url.isValid() && clipboardText.contains('.')) { // contains '.' -> domain
            if (url.scheme().isEmpty()) {
                url = "http://" + clipboardText;
            }
            emit openUrl(url);
        }
    }
}
