/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
 * Copyright (C) 2008 Benjamin C. Meyer <ben@meyerhome.net>
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
 *
 */

#include "ui_searchbar.h"
#include "searchbar.h"

#include <QResizeEvent>
#include <QShortcut>

#include <KDE/KColorScheme>
#include <KDE/KDebug>
#include <KDE/KIcon>
#include <KDE/KLocalizedString>

namespace KDEPrivate {

class SearchBar::SearchBarPrivate
{
public:
    SearchBarPrivate() {}

    void init (SearchBar* searchBar)
    {
        ui.setupUi(searchBar);
        ui.optionsButton->addAction(ui.actionMatchCase);
        ui.optionsButton->addAction(ui.actionSearchAutomatically);
        ui.closeButton->setIcon(KIcon("dialog-close"));
        ui.previousButton->setIcon(KIcon("go-up-search"));
        ui.previousButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        ui.nextButton->setIcon(KIcon("go-down-search"));
        ui.nextButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        ui.searchInfo->setText(i18n("&Find:"));

        connect(ui.actionSearchAutomatically, SIGNAL(triggered(bool)),
                searchBar, SLOT(searchAsYouTypeChanged(bool)));
        connect(ui.nextButton, SIGNAL(clicked()),
                searchBar, SLOT(findNext()));
        connect(ui.previousButton, SIGNAL(clicked()),
                searchBar, SLOT(findPrevious()));
        connect(ui.searchLineEdit, SIGNAL(returnPressed()),
                searchBar, SLOT(findNext()));

        //searchBar->setMinimumWidth(widget->minimumWidth());
        //searchBar->setMaximumWidth(widget->maximumWidth());
        //searchBar->setMinimumHeight(widget->minimumHeight());

        // Update the state of the searchAsYouType option
        searchBar->searchAsYouTypeChanged (ui.actionSearchAutomatically->isChecked());
    }

    Ui::SearchBar ui;
};

SearchBar::SearchBar(QWidget *parent)
          :QWidget(parent)
{
    d = new SearchBarPrivate;
    d->init(this);

    // Start off hidden by default...
    setVisible(false);
}

SearchBar::~SearchBar()
{
    delete d;
}

void SearchBar::clear()
{
    d->ui.searchLineEdit->clear();
}

void SearchBar::show()
{
    if (isVisible())
        return;

    QWidget::show();
    d->ui.searchLineEdit->setFocus();
    d->ui.searchLineEdit->selectAll();
}

QString SearchBar::searchText() const
{
    return d->ui.searchLineEdit->text();
}

bool SearchBar::caseSensitive() const
{
    return d->ui.actionMatchCase->isChecked();
}

void SearchBar::setSearchText(const QString& text)
{
    d->ui.searchLineEdit->setText(text);
}

void SearchBar::setFoundMatch(bool match)
{
    QString styleSheet;

    if (!d->ui.searchLineEdit->text().isEmpty()) {
        KColorScheme::BackgroundRole bgColorScheme;

        if (match)
          bgColorScheme = KColorScheme::PositiveBackground;
        else
          bgColorScheme = KColorScheme::NegativeBackground;

        KStatefulBrush bgBrush(KColorScheme::View, bgColorScheme);

        styleSheet = QString("QLineEdit{ background-color:%1 }")
                     .arg(bgBrush.brush(d->ui.searchLineEdit).color().name());
    }

    d->ui.searchLineEdit->setStyleSheet(styleSheet);
}

void SearchBar::searchAsYouTypeChanged(bool checked)
{
    if (checked) {
        connect(d->ui.searchLineEdit, SIGNAL(textEdited(const QString&)),
                this, SIGNAL(searchTextChanged(const QString&)));
    } else {
        disconnect(d->ui.searchLineEdit, SIGNAL(textEdited(const QString&)),
                   this, SIGNAL(searchTextChanged(const QString&)));
    }
}

void SearchBar::findNext()
{
    if (!isVisible())
        return;

    emit searchTextChanged(d->ui.searchLineEdit->text());
}

void SearchBar::findPrevious()
{
    if (!isVisible())
        return;

    emit searchTextChanged(d->ui.searchLineEdit->text(), true);
}

}

#include "searchbar.moc"
