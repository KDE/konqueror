/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
 * code based on konsole search bar
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

#include "webkitsearchbar.h"
#include <QHBoxLayout>
#include <QToolButton>
#include <QFontMetrics>
#include <QLabel>
#include <KLocale>
#include <QTimer>
#include <QEvent>
#include <QKeyEvent>
#include <QCheckBox>

#include <KIcon>
#include <KLineEdit>
#include <KColorScheme>

WebKitSearchBar::WebKitSearchBar(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout(this);

    QToolButton* close = new QToolButton(this);
    close->setObjectName("close-button");
    close->setToolTip(i18n("Close the search bar"));
    close->setAutoRaise(true);
    close->setIcon(KIcon("dialog-close"));
    connect(close, SIGNAL(clicked()), this, SIGNAL(closeClicked()));

    QLabel* findLabel = new QLabel(i18n("Find:"), this);
    m_searchEdit = new KLineEdit(this);
    m_searchEdit->installEventFilter(this);
    m_searchEdit->setClearButtonShown(true);
    m_searchEdit->setObjectName("search-edit");
    m_searchEdit->setToolTip(i18n("Enter the text to search for here"));

    QFontMetrics metrics(m_searchEdit->font());
    int maxWidth = metrics.maxWidth();
    m_searchEdit->setMinimumWidth(maxWidth*6);
    m_searchEdit->setMaximumWidth(maxWidth*10);

    m_searchTimer = new QTimer(this);
    m_searchTimer->setInterval(250);
    m_searchTimer->setSingleShot(true);
    connect(m_searchTimer, SIGNAL(timeout()), this, SLOT(notifySearchChanged()));
    connect(m_searchEdit, SIGNAL(textChanged(const QString&)), m_searchTimer, SLOT(start()));

    QToolButton* findNext = new QToolButton(this);
    findNext->setObjectName("find-next-button");
    findNext->setText(i18n("Next"));
    findNext->setAutoRaise(true);
    findNext->setIcon(KIcon("go-down-search"));
    findNext->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    findNext->setToolTip(i18n("Find the next match for the current search phrase"));
    connect(findNext, SIGNAL(clicked()), this, SIGNAL(findNextClicked()));

    QToolButton* findPrev = new QToolButton(this);
    findPrev->setObjectName("find-previous-button");
    findPrev->setText(i18n("Previous"));
    findPrev->setAutoRaise(true);
    findPrev->setIcon(KIcon("go-up-search"));
    findPrev->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    findPrev->setToolTip(i18n("Find the previous match for the current search phrase"));
    connect(findPrev, SIGNAL(clicked()), this, SIGNAL(findPreviousClicked()));

    m_caseSensitive = new QCheckBox(i18n("Match Case"), this);

    layout->addWidget(close);
    layout->addWidget(findLabel);
    layout->addWidget(m_searchEdit);
    layout->addWidget(findNext);
    layout->addWidget(findPrev);
    layout->addWidget(m_caseSensitive);

    layout->addStretch();
    layout->setMargin(4);

    setLayout(layout);
}

WebKitSearchBar::~WebKitSearchBar()
{
}


void WebKitSearchBar::notifySearchChanged()
{
    emit searchChanged(searchText());
}

QString WebKitSearchBar::searchText() const
{
    return m_searchEdit->text();
}


bool WebKitSearchBar::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_searchEdit) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

            if (keyEvent->key() == Qt::Key_Escape) {
                emit closeClicked();
                return true;
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

void WebKitSearchBar::setVisible(bool visible)
{
    QWidget::setVisible(visible);

    if (visible) {
        //TODO - Check if this is the correct reason value to use here
        m_searchEdit->setFocus(Qt::ActiveWindowFocusReason);
        m_searchEdit->selectAll();
    }
}

bool WebKitSearchBar::caseSensitive() const
{
    return m_caseSensitive->isChecked();
}

void WebKitSearchBar::setFoundMatch(bool match)
{
    if (!match && !m_searchEdit->text().isEmpty()) {
        KStatefulBrush backgroundBrush(KColorScheme::View, KColorScheme::NegativeBackground);

        QString styleSheet = QString("QLineEdit{ background-color:%1 }")
                             .arg(backgroundBrush.brush(m_searchEdit).color().name());

        m_searchEdit->setStyleSheet(styleSheet);
    } else {
        m_searchEdit->setStyleSheet(QString());
    }
}


#include "webkitsearchbar.moc"
