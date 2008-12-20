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
#include "searchbar.h"

#include <KDE/KUrl>
#include <KDE/KDebug>

#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QMouseEvent>
#include <QtWebKit/QWebFrame>

class KWebView::KWebViewPrivate
{
public:
    KWebViewPrivate()
    : customContextMenu(false)
    , keyboardModifiers(Qt::NoModifier)
    , pressedButtons(Qt::NoButton)
    , searchBar(0)
    {}
    bool customContextMenu;
    Qt::KeyboardModifiers keyboardModifiers;
    Qt::MouseButtons pressedButtons;
    SearchBar *searchBar;
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
    if (!url.isEmpty() && (d->pressedButtons & Qt::MidButton)) {
        kDebug() << "middle clicked url" << url;
        emit openUrlInNewTab(url);
        return;
    }

    QWebView::mouseReleaseEvent(event);
    if (!event->isAccepted() && (d->pressedButtons & Qt::MidButton)) {
        KUrl url(QApplication::clipboard()->text(QClipboard::Selection));
        if (!url.isEmpty() && url.isValid() && !url.scheme().isEmpty()) {
            emit openUrl(url);
        }
    }
}

QWidget *KWebView::searchBar()
{
    if (!d->searchBar) {
        d->searchBar = new SearchBar;
        kDebug() << "Created new SearchBar" << d->searchBar;
        d->searchBar->setVisible(false);

        connect(d->searchBar, SIGNAL(findNextClicked()), this, SLOT(slotFindNextClicked()));
        connect(d->searchBar, SIGNAL(findPreviousClicked()),  this, SLOT(slotFindPreviousClicked()));
        connect(d->searchBar, SIGNAL(searchChanged(const QString&)), this, SLOT(slotSearchChanged(const QString &)));
        connect(this, SIGNAL(destroyed()), d->searchBar, SLOT(deleteLater()));
    }
    return d->searchBar;
}

void KWebView::slotFindPreviousClicked()
{
    resultSearch(KWebPage::FindBackward);
}

void KWebView::slotFindNextClicked()
{
    KWebPage::FindFlags flags;
    resultSearch(flags);
}

void KWebView::slotSearchChanged(const QString & text)
{
    Q_UNUSED(text);
    KWebPage::FindFlags flags;
    resultSearch(flags);
}

void KWebView::resultSearch(KWebPage::FindFlags flags)
{
    if (d->searchBar->caseSensitive())
        flags |= KWebPage::FindCaseSensitively;
    bool status = page()->findText(d->searchBar->searchText(), flags);
    d->searchBar->setFoundMatch(status);
}

