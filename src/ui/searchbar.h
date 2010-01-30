/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
 * Copyright 2008 Benjamin C. Meyer <ben@meyerhome.net>
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
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
 */

#ifndef SEARCHBAR_P_H
#define SEARCHBAR_P_H

#include <QtGui/QWidget>

namespace KDEPrivate {

/**
 * This is the widget that shows up when the search is initiated.
 */
class SearchBar : public QWidget
{
    Q_OBJECT

public:
    SearchBar(QWidget *parent = 0);
    ~SearchBar();

    QString searchText() const;
    bool caseSensitive() const;
    void setFoundMatch(bool match);
    void setSearchText(const QString&);

public Q_SLOTS:
    void show();
    void hide();
    void clear();
    void findNext();
    void findPrevious();

Q_SIGNALS:
    void searchTextChanged(const QString& text, bool backward = false);

private Q_SLOTS:
    void searchAsYouTypeChanged(bool);

private:
    class SearchBarPrivate;
    SearchBarPrivate * const d;

};

}

#endif // SEARCHBAR_P_H
