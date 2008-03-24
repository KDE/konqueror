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

#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <KDialog>
#include "ui_searchwidget.h"

class QWebPage;

class SearchWidget : public QWidget, public Ui::SearchWidget
{
public:
  SearchWidget( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};

class SearchWidgetDialog : public KDialog
{
    Q_OBJECT
public:
    SearchWidgetDialog( QWidget *parent, QWebPage *page);

    QString searchText() const;
    bool caseSensitive() const;

protected slots:
    void slotSearch();

private:
    SearchWidget *m_search;
    QWebPage *m_page;
};

#endif
