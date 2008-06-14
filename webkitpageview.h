
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

#ifndef WEBKITPAGEVIEW_H
#define WEBKITPAGEVIEW_H

#include <QtWebKit/QWebPage>
class WebKitPart;
class WebView;
class SearchBar;

class WebKitPageView : public QWidget
{
    Q_OBJECT
public:
    WebKitPageView(WebKitPart *part, QWidget *parent);
    ~WebKitPageView();
    inline WebView *view() { return m_webView; }

    SearchBar *searchBar() const { return m_searchBar; }

public slots:
    void slotFind();
    void slotCloseSearchBarClicked();
    void slotFindNextClicked();
    void slotFindPreviousClicked();
    void slotSearchChanged(const QString &);
protected:
    void resultSearch(QWebPage::FindFlags flags);


private:
    WebView *m_webView;
    SearchBar *m_searchBar;
};


#endif /* WEBKITPAGEVIEW_H */

