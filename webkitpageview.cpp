/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
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

#include "webkitpageview.h"

#include <QTextEdit>
#include <QVBoxLayout>

#include "webkitview.h"
#include "webkitsearchbar.h"

WebKitPageView::WebKitPageView(WebKitPart *part, QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *lay = new QVBoxLayout(this);
    m_webView = new WebView(part, parent);
    setLayout(lay);
    lay->addWidget(m_webView);
    m_searchBar = new WebKitSearchBar(parent);
    m_searchBar->setVisible(false);
    lay->setMargin(0);
    lay->setSpacing(0);
    lay->addWidget(m_searchBar);

    connect(m_searchBar, SIGNAL(closeClicked()), this, SLOT(slotCloseSearchBarClicked()));
    connect(m_searchBar, SIGNAL(findNextClicked()), this, SLOT(slotFindNextClicked()));
    connect(m_searchBar, SIGNAL(findPreviousClicked()),  this, SLOT(slotFindPreviousClicked()));
    connect(m_searchBar, SIGNAL(searchChanged(const QString&)), this, SLOT(slotSearchChanged(const QString &)));
}


WebKitPageView::~WebKitPageView()
{

}

void WebKitPageView::slotCloseSearchBarClicked()
{
    m_searchBar->setVisible(false);
}

void WebKitPageView::slotFind()
{
    m_searchBar->setVisible(true);
}

void WebKitPageView::slotFindPreviousClicked()
{
    QWebPage::FindFlags flags;
    flags |= QWebPage::FindBackward;
    resultSearch(flags);
}

void WebKitPageView::slotFindNextClicked()
{
    QWebPage::FindFlags flags;
    resultSearch(flags);
}

void WebKitPageView::slotSearchChanged(const QString & text)
{
    QWebPage::FindFlags flags;
    resultSearch(flags);
}

void WebKitPageView::resultSearch(QWebPage::FindFlags flags)
{
    if (m_searchBar->caseSensitive())
        flags |= QWebPage::FindCaseSensitively;
    bool status = m_webView->page()->findText(m_searchBar->searchText(), flags);
    m_searchBar->setFoundMatch(status);
}

#include "webkitpageview.moc"
