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
#include <QLineEdit>
#include <QTimer>
#include <QEvent>
#include <QKeyEvent>
#include <QCheckBox>
#include <KIcon>
#include <KColorScheme>

WebKitSearchBar::WebKitSearchBar( QWidget *parent )
    : QWidget( parent )
{
    QHBoxLayout* layout = new QHBoxLayout(this);

    QToolButton* close = new QToolButton(this);
    close->setObjectName("close-button");
    close->setToolTip( i18n("Close the search bar") );
    close->setAutoRaise(true);
    close->setIcon(KIcon("dialog-close"));
    connect( close , SIGNAL(clicked()) , this , SIGNAL(closeClicked()) );

    QLabel* findLabel = new QLabel(i18n("Find:"),this);
    _searchEdit = new QLineEdit(this);
    _searchEdit->installEventFilter(this);
    _searchEdit->setObjectName("search-edit");
    _searchEdit->setToolTip( i18n("Enter the text to search for here") );

    QFontMetrics metrics(_searchEdit->font());
    int maxWidth = metrics.maxWidth();
    _searchEdit->setMinimumWidth(maxWidth*6);
    _searchEdit->setMaximumWidth(maxWidth*10);

    _searchTimer = new QTimer(this);
    _searchTimer->setInterval(250);
    _searchTimer->setSingleShot(true);
    connect( _searchTimer , SIGNAL(timeout()) , this , SLOT(notifySearchChanged()) );
    connect( _searchEdit , SIGNAL(textChanged(const QString&)) , _searchTimer , SLOT(start()));

    QToolButton* findNext = new QToolButton(this);
    findNext->setObjectName("find-next-button");
    findNext->setText(i18n("Next"));
    findNext->setAutoRaise(true);
    findNext->setIcon( KIcon("go-down-search") );
    findNext->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    findNext->setToolTip( i18n("Find the next match for the current search phrase") );
    connect( findNext , SIGNAL(clicked()) , this , SIGNAL(findNextClicked()) );

    QToolButton* findPrev = new QToolButton(this);
    findPrev->setObjectName("find-previous-button");
    findPrev->setText(i18n("Previous"));
    findPrev->setAutoRaise(true);
    findPrev->setIcon( KIcon("go-up-search") );
    findPrev->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    findPrev->setToolTip( i18n("Find the previous match for the current search phrase") );
    connect( findPrev , SIGNAL(clicked()) , this , SIGNAL(findPreviousClicked()) );

    _caseSensitive = new QCheckBox( i18n( "Match Case" ), this );

    layout->addWidget(close);
    layout->addWidget(findLabel);
    layout->addWidget(_searchEdit);
    layout->addWidget(findNext);
    layout->addWidget(findPrev);
    layout->addWidget( _caseSensitive );

    layout->addStretch();
    layout->setMargin(4);

    setLayout(layout);
}

WebKitSearchBar::~WebKitSearchBar()
{
}


void WebKitSearchBar::notifySearchChanged()
{
    emit searchChanged( searchText() );
}

QString WebKitSearchBar::searchText() const
{
    return _searchEdit->text();
}


bool WebKitSearchBar::eventFilter(QObject* watched , QEvent* event)
{
    if ( watched == _searchEdit )
    {
        if ( event->type() == QEvent::KeyPress )
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

            if ( keyEvent->key() == Qt::Key_Escape )
            {
                emit closeClicked();
                return true;
            }
        }
    }

    return QWidget::eventFilter(watched,event);
}

void WebKitSearchBar::setVisible(bool visible)
{
    QWidget::setVisible(visible);

    if ( visible )
    {
        //TODO - Check if this is the correct reason value to use here
        _searchEdit->setFocus( Qt::ActiveWindowFocusReason );
        _searchEdit->selectAll();
    }
}

bool WebKitSearchBar::caseSensitive() const
{
    return _caseSensitive->isChecked();
}

void WebKitSearchBar::setFoundMatch( bool match )
{
    if ( !match && !_searchEdit->text().isEmpty() )
    {
        KStatefulBrush backgroundBrush(KColorScheme::View,KColorScheme::NegativeBackground);

        QString styleSheet = QString("QLineEdit{ background-color:%1 }")
                             .arg(backgroundBrush.brush(_searchEdit).color().name());

        _searchEdit->setStyleSheet( styleSheet );
    }
    else
    {
        _searchEdit->setStyleSheet( QString() );
    }
}


#include "webkitsearchbar.moc"
