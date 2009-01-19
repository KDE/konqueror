/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
 * Copyright 2008 Benjamin C. Meyer <ben@meyerhome.net>
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
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

#ifndef SEARCHBAR_H
#define SEARCHBAR_H

#include <QWidget>

#include "ui_searchbar.h"

class QTimeLine;
class KWebView;

/**
 * This is the widget that shows up when the search is initiated.
 */
class SearchBar : public QWidget
{
    Q_OBJECT

public:
    SearchBar(QWidget *parent = 0);

    QString searchText() const;
    bool caseSensitive() const;
    void setFoundMatch(bool match);

public Q_SLOTS:
    void clear();
    void show();
    void hide();

protected:
    void resizeEvent(QResizeEvent *event);

Q_SIGNALS:
    void searchChanged(const QString& text);
    void closeClicked();
    void findNextClicked();
    void findPreviousClicked();

private:
    class SearchBarPrivate;
    SearchBarPrivate* d;

private Q_SLOTS:
    void frameChanged(int frame);
    void notifySearchChanged();
    void slotHide();
    void searchAsYouTypeChanged(int state);

};

#endif // SEARCHBAR_H
