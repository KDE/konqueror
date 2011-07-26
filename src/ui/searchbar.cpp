/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
 * Copyright (C) 2008 Benjamin C. Meyer <ben@meyerhome.net>
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
 *
 */

#include "searchbar.h"
#include "ui_searchbar.h"

#include <KDE/KColorScheme>
#include <KDE/KDebug>
#include <KDE/KIcon>
#include <KDE/KLocalizedString>

#include <QtGui/QResizeEvent>
#include <QtGui/QShortcut>


namespace KDEPrivate {


static KColorScheme::BackgroundRole bgColorRoleForState(bool state)
{
    if (state)
        return KColorScheme::PositiveBackground;

    return KColorScheme::NegativeBackground;
}

class SearchBar::SearchBarPrivate
{
public:
  SearchBarPrivate() : lastBgColorRole(KColorScheme::NormalBackground) {}

    void init (SearchBar* searchBar)
    {
        ui.setupUi(searchBar);
        ui.optionsButton->addAction(ui.actionMatchCase);
        ui.optionsButton->addAction(ui.actionHighlightMatch);
        ui.optionsButton->addAction(ui.actionSearchAutomatically);
        ui.closeButton->setIcon(KIcon("dialog-close"));
        ui.previousButton->setIcon(KIcon("go-up-search"));
        ui.previousButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        ui.nextButton->setIcon(KIcon("go-down-search"));
        ui.nextButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        ui.searchInfo->setText(i18nc("label for input line to find text", "&Find:"));

        connect(ui.actionSearchAutomatically, SIGNAL(triggered(bool)),
                searchBar, SLOT(searchAsYouTypeChanged(bool)));
        connect(ui.nextButton, SIGNAL(clicked()),
                searchBar, SLOT(findNext()));
        connect(ui.previousButton, SIGNAL(clicked()),
                searchBar, SLOT(findPrevious()));
        connect(ui.searchLineEdit, SIGNAL(returnPressed()),
                searchBar, SLOT(findNext()));
        connect(ui.searchLineEdit, SIGNAL(textChanged(QString)),
                searchBar, SLOT(textChanged(QString)));

        // Update the state of the searchAsYouType option
        searchBar->searchAsYouTypeChanged (ui.actionSearchAutomatically->isChecked());
    }

    Ui::SearchBar ui;
    KColorScheme::BackgroundRole lastBgColorRole;
};

SearchBar::SearchBar(QWidget *parent)
          :QWidget(parent), d (new SearchBarPrivate)
{

    // Initialize the user interface...
    d->init(this);

    // Start off hidden by default...
    setVisible(false);
}

SearchBar::~SearchBar()
{
    // NOTE: For some reason, if we do not clear the focus from the line edit
    // widget before we delete this object, it seems to cause a crash!!
    d->ui.searchLineEdit->clearFocus();
    delete d;
}

void SearchBar::clear()
{
    d->ui.searchLineEdit->clear();
}

void SearchBar::show()
{
    if (!isVisible())
        QWidget::show();

    if (!d->ui.searchLineEdit->hasFocus()) {
        d->ui.searchLineEdit->selectAll();
        d->ui.searchLineEdit->setFocus();
    }
}

void SearchBar::hide()
{
    if (!isVisible())
        return;

    d->ui.searchLineEdit->setStyleSheet(QString());
    d->lastBgColorRole = KColorScheme::NormalBackground;
    emit searchTextChanged(QString());
    QWidget::hide();
}

QString SearchBar::searchText() const
{
    return d->ui.searchLineEdit->text();
}

bool SearchBar::caseSensitive() const
{
    return d->ui.actionMatchCase->isChecked();
}

bool SearchBar::highlightMatches() const
{
    return d->ui.actionHighlightMatch->isChecked();
}

void SearchBar::setSearchText(const QString& text)
{
    show();
    d->ui.searchLineEdit->setText(text);
}

void SearchBar::setFoundMatch(bool match)
{
    //kDebug() << match;
    const bool isEmptySearchText = d->ui.searchLineEdit->text().isEmpty();
    KColorScheme::BackgroundRole bgColorRole = bgColorRoleForState(match);

    if (bgColorRole == d->lastBgColorRole)
        return;
    
    if (isEmptySearchText && d->lastBgColorRole == KColorScheme::NormalBackground)
        return;

    QString styleSheet;

    if (isEmptySearchText) {
        bgColorRole = KColorScheme::NormalBackground;
    } else {
        KStatefulBrush bgBrush(KColorScheme::View, bgColorRole);
        styleSheet = QString("QLineEdit{ background-color:%1 }")
                      .arg(bgBrush.brush(d->ui.searchLineEdit).color().name());
    }

    //kDebug() << "stylesheet:" << styleSheet;
    d->ui.searchLineEdit->setStyleSheet(styleSheet);
    d->lastBgColorRole = bgColorRole;
}

void SearchBar::searchAsYouTypeChanged(bool checked)
{
    if (checked) {
        connect(d->ui.searchLineEdit, SIGNAL(textEdited(QString)),
                this, SIGNAL(searchTextChanged(QString)));
    } else {
        disconnect(d->ui.searchLineEdit, SIGNAL(textEdited(QString)),
                   this, SIGNAL(searchTextChanged(QString)));
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

void SearchBar::textChanged(const QString &text)
{
    if (!text.isEmpty())
        return;

    d->ui.searchLineEdit->setStyleSheet(QString());
}

}

#include "searchbar.moc"
