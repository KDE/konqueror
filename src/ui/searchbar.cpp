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

#include <KDE/KLineEdit>
#include <KDE/KColorScheme>
#include <KDE/KDebug>
#include <KDE/KIcon>
#include <KDE/KLocalizedString>

#include <QtGui/QResizeEvent>
#include <QtGui/QShortcut>


namespace KDEPrivate {


class SearchBar::SearchBarPrivate
{
public:
    void init (SearchBar* searchBar)
    {
        ui.setupUi(searchBar);
        ui.optionsButton->addAction(ui.actionMatchCase);
        ui.optionsButton->addAction(ui.actionHighlightMatch);
        ui.optionsButton->addAction(ui.actionSearchAutomatically);
        ui.closeButton->setIcon(KIcon("dialog-close"));
        ui.previousButton->setIcon(KIcon("go-up-search"));
        ui.nextButton->setIcon(KIcon("go-down-search"));
        ui.previousButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        ui.nextButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        ui.searchInfo->setText(i18nc("label for input line to find text", "&Find:"));

        connect(ui.nextButton, SIGNAL(clicked()),
                searchBar, SLOT(findNext()));
        connect(ui.previousButton, SIGNAL(clicked()),
                searchBar, SLOT(findPrevious()));
        connect(ui.searchComboBox, SIGNAL(returnPressed()),
                searchBar, SLOT(findNext()));
        connect(ui.searchComboBox, SIGNAL(editTextChanged(QString)),
                searchBar, SLOT(textChanged(QString)));
    }

    Ui::SearchBar ui;
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
    d->ui.searchComboBox->clearFocus();
    delete d;
}

void SearchBar::clear()
{
    d->ui.searchComboBox->clear();
}

void SearchBar::setVisible (bool visible)
{
    if (visible) {
        d->ui.searchComboBox->setFocus( Qt::ActiveWindowFocusReason );
        d->ui.searchComboBox->lineEdit()->selectAll();
    } else {
        d->ui.searchComboBox->setPalette(QPalette());
        emit searchTextChanged(QString());
    }

    QWidget::setVisible(visible);
}

QString SearchBar::searchText() const
{
    return d->ui.searchComboBox->currentText();
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
    d->ui.searchComboBox->setEditText(text);
}

void SearchBar::setFoundMatch(bool match)
{
    //kDebug() << match;
    if (d->ui.searchComboBox->currentText().isEmpty()) {
        d->ui.searchComboBox->setPalette(QPalette());
        return;
    }

    KColorScheme::BackgroundRole role = (match ? KColorScheme::PositiveBackground : KColorScheme::NegativeBackground);
    QPalette newPal( d->ui.searchComboBox->palette() );
    KColorScheme::adjustBackground(newPal, role );
    d->ui.searchComboBox->setPalette(newPal);
}

void SearchBar::findNext()
{
    if (!isVisible())
        return;

    const QString text (d->ui.searchComboBox->currentText());
    if (d->ui.searchComboBox->findText(text) == -1) {
        d->ui.searchComboBox->addItem(text);
    }

    emit searchTextChanged(text);
}

void SearchBar::findPrevious()
{
    if (!isVisible())
        return;

    const QString text (d->ui.searchComboBox->currentText());
    if (d->ui.searchComboBox->findText(text) == -1) {
        d->ui.searchComboBox->addItem(text);
    }

    emit searchTextChanged(d->ui.searchComboBox->currentText(), true);
}

void SearchBar::textChanged(const QString &text)
{
    if (text.isEmpty()) {
        d->ui.searchComboBox->setPalette(QPalette());
        d->ui.nextButton->setEnabled(false);
        d->ui.previousButton->setEnabled(false);
    } else {
        d->ui.nextButton->setEnabled(true);
        d->ui.previousButton->setEnabled(true);
    }

    if (d->ui.actionSearchAutomatically->isChecked()) {
        emit searchTextChanged(d->ui.searchComboBox->currentText());
    }
}

bool SearchBar::event(QEvent* e)
{
    // Close the bar when Escape is pressed. Note we cannot
    // assign Escape as a shortcut key because it would cause
    // a conflict with the Stop button.
    if (e->type() == QEvent::ShortcutOverride) {
        QKeyEvent* kev = static_cast<QKeyEvent*>(e);
        if (kev->key() == Qt::Key_Escape) {
            e->accept();
            clearFocus();
            setVisible(false);
            return true;
        }
    }
    return QWidget::event(e);
}

}

#include "searchbar.moc"
