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

#include <KLineEdit>
#include <KColorScheme>
#include <QIcon>
#include <KLocalizedString>

#include <QResizeEvent>


SearchBar::SearchBar(QWidget *parent)
    :QWidget(parent)
{

    // Get the widget that currently has the focus so we can properly
    // restore it when the filter bar is closed.
    QWidget* widgetWindow = (parent ? parent->window() : nullptr);
    m_focusWidget = (widgetWindow ? widgetWindow->focusWidget() : nullptr);

    // Initialize the user interface...
    m_ui.setupUi(this);
    m_ui.optionsButton->addAction(m_ui.actionMatchCase);
    m_ui.optionsButton->addAction(m_ui.actionHighlightMatch);
    m_ui.optionsButton->addAction(m_ui.actionSearchAutomatically);

    setFocusProxy(m_ui.searchComboBox);
    
    connect(m_ui.nextButton, SIGNAL(clicked()),
            this, SLOT(findNext()));
    connect(m_ui.previousButton, SIGNAL(clicked()),
            this, SLOT(findPrevious()));
    connect(m_ui.searchComboBox, SIGNAL(returnPressed()),
            this, SLOT(findNext()));
    connect(m_ui.searchComboBox, SIGNAL(editTextChanged(QString)),
            this, SLOT(textChanged(QString)));

    // Start off hidden by default...
    setVisible(false);
}

SearchBar::~SearchBar()
{
    // NOTE: For some reason, if we do not clear the focus from the line edit
    // widget before we delete this object, it seems to cause a crash!!
    m_ui.searchComboBox->clearFocus();
}

void SearchBar::clear()
{
    m_ui.searchComboBox->clear();
}

void SearchBar::setVisible (bool visible)
{
    if (visible) {
        m_ui.searchComboBox->setFocus(Qt::ActiveWindowFocusReason);
        m_ui.searchComboBox->lineEdit()->selectAll();
    } else {
        m_ui.searchComboBox->setPalette(QPalette());
        emit searchTextChanged(QString());
    }

    QWidget::setVisible(visible);
}

QString SearchBar::searchText() const
{
    return m_ui.searchComboBox->currentText();
}

bool SearchBar::caseSensitive() const
{
    return m_ui.actionMatchCase->isChecked();
}

bool SearchBar::highlightMatches() const
{
    return m_ui.actionHighlightMatch->isChecked();
}

void SearchBar::setSearchText(const QString& text)
{
    show();
    m_ui.searchComboBox->setEditText(text);
}

void SearchBar::setFoundMatch(bool match)
{
    //kDebug() << match;
    if (m_ui.searchComboBox->currentText().isEmpty()) {
        m_ui.searchComboBox->setPalette(QPalette());
        return;
    }

    KColorScheme::BackgroundRole role = (match ? KColorScheme::PositiveBackground : KColorScheme::NegativeBackground);
    QPalette newPal(m_ui.searchComboBox->palette());
    KColorScheme::adjustBackground(newPal, role);
    m_ui.searchComboBox->setPalette(newPal);
}

void SearchBar::findNext()
{
    if (!isVisible())
        return;

    const QString text (m_ui.searchComboBox->currentText());
    if (m_ui.searchComboBox->findText(text) == -1) {
        m_ui.searchComboBox->addItem(text);
    }

    emit searchTextChanged(text);
}

void SearchBar::findPrevious()
{
    if (!isVisible())
        return;

    const QString text (m_ui.searchComboBox->currentText());
    if (m_ui.searchComboBox->findText(text) == -1) {
        m_ui.searchComboBox->addItem(text);
    }

    emit searchTextChanged(m_ui.searchComboBox->currentText(), true);
}

void SearchBar::textChanged(const QString &text)
{
    if (text.isEmpty()) {
        m_ui.searchComboBox->setPalette(QPalette());
        m_ui.nextButton->setEnabled(false);
        m_ui.previousButton->setEnabled(false);
    } else {
        m_ui.nextButton->setEnabled(true);
        m_ui.previousButton->setEnabled(true);
    }

    if (m_ui.actionSearchAutomatically->isChecked()) {
        emit searchTextChanged(m_ui.searchComboBox->currentText());
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
            close();
            if (m_focusWidget) {
                m_focusWidget->setFocus();
                m_focusWidget = nullptr;
            }
            return true;
        }
    }
    return QWidget::event(e);
}
