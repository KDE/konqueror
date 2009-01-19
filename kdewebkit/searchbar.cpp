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
 *
 */

#include "searchbar.h"

#include <QResizeEvent>
#include <QShortcut>
#include <QTimeLine>

#include <KColorScheme>

class SearchBar::SearchBarPrivate {
public:
      SearchBarPrivate(SearchBar* searchBar, QWidget* widget, QTimeLine* timeLine)
      : widget(widget), timeLine(timeLine), m_searchBar(searchBar) {}
    void initializeSearchWidget() {
        widget = new QWidget(m_searchBar);
        widget->setContentsMargins(0, 0, 0, 0);
        ui.setupUi(widget);
        ui.closeButton->setIcon(KIcon("dialog-close"));
        ui.previousButton->setIcon(KIcon("go-up-search"));
        ui.previousButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        ui.nextButton->setIcon(KIcon("go-down-search"));
        ui.nextButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        ui.searchInfo->setText(QString());
        if (ui.searchAsYouType->checkState() == Qt::Checked) {
        connect(ui.searchLineEdit, SIGNAL(textChanged(const QString&)),
                m_searchBar, SIGNAL(searchChanged(const QString&)));
        }
        connect(ui.searchAsYouType, SIGNAL(stateChanged(int)), m_searchBar, SLOT(searchAsYouTypeChanged(int)));
        connect(ui.nextButton, SIGNAL(clicked()),
                m_searchBar, SIGNAL(findNextClicked()));
        connect(ui.previousButton, SIGNAL(clicked()),
                m_searchBar, SIGNAL(findPreviousClicked()));
        connect(ui.searchLineEdit, SIGNAL(returnPressed()),
                m_searchBar, SIGNAL(findNextClicked()));
        connect(ui.closeButton, SIGNAL(clicked()),
                m_searchBar, SLOT(hide()));
        m_searchBar->setMinimumWidth(widget->minimumWidth());
        m_searchBar->setMaximumWidth(widget->maximumWidth());
        m_searchBar->setMinimumHeight(widget->minimumHeight());
    }
    Ui::SearchBar ui;
    QWidget *widget;
    QTimeLine *timeLine;
private:
    SearchBar* m_searchBar;
};

SearchBar::SearchBar(QWidget *parent)
    : QWidget(parent)
{
    d = new SearchBarPrivate(this, 0, new QTimeLine(150, this));
    d->initializeSearchWidget();

    // we start off hidden
    setMaximumHeight(0);
    d->widget->setGeometry(0, -1 * d->widget->height(),
                          d->widget->width(), d->widget->height());
    QWidget::hide();

    connect(d->timeLine, SIGNAL(frameChanged(int)),
            this, SLOT(frameChanged(int)));
    connect(this, SIGNAL(closeClicked()), this, SLOT(hide()));

    new QShortcut(QKeySequence(Qt::Key_Escape), this, SLOT(hide()));
}

SearchBar::~SearchBar()
{
    delete d;
}

void SearchBar::clear()
{
    d->ui.searchLineEdit->setText(QString());
}

void SearchBar::show()
{
    if (isVisible())
        return;

    QWidget::show();
    d->ui.searchLineEdit->setFocus();
    d->ui.searchLineEdit->selectAll();

    d->timeLine->setFrameRange(-1 * d->widget->height(), 0);
    d->timeLine->setDirection(QTimeLine::Forward);
    disconnect(d->timeLine, SIGNAL(finished()),
               this, SLOT(slotHide()));
    d->timeLine->start();
}

void SearchBar::resizeEvent(QResizeEvent *event)
{
    if (event->size().width() != d->widget->width())
        d->widget->resize(event->size().width(), d->widget->height());
    QWidget::resizeEvent(event);
}

void SearchBar::hide()
{
    d->timeLine->setDirection(QTimeLine::Backward);
    d->timeLine->start();
    connect(d->timeLine, SIGNAL(finished()), this, SLOT(slotHide()));
}

void SearchBar::slotHide()
{
    QWidget::hide();
}

void SearchBar::frameChanged(int frame)
{
    if (!d->widget)
        return;
    d->widget->move(0, frame);
    const int height = qMax(0, d->widget->y() + d->widget->height());
    setMinimumHeight(height);
    setMaximumHeight(height);
}

void SearchBar::notifySearchChanged()
{
    emit searchChanged(searchText());
}

QString SearchBar::searchText() const
{
    return d->ui.searchLineEdit->text();
}

bool SearchBar::caseSensitive() const
{
    return d->ui.matchCaseCheckBox->isChecked();
}

void SearchBar::setFoundMatch(bool match)
{
    if (!match && !searchText().isEmpty()) {
        KStatefulBrush backgroundBrush(KColorScheme::View, KColorScheme::NegativeBackground);

        QString styleSheet = QString("QLineEdit{ background-color:%1 }")
                             .arg(backgroundBrush.brush(d->ui.searchLineEdit).color().name());

        d->ui.searchLineEdit->setStyleSheet(styleSheet);
    } else {
        d->ui.searchLineEdit->setStyleSheet(QString());
    }
}
void SearchBar::searchAsYouTypeChanged(int state)
{
    if (state == 0) {
        disconnect(d->ui.searchLineEdit, SIGNAL(textChanged(const QString&)),
                   this, SIGNAL(searchChanged(const QString&)));
    } else {
        connect(d->ui.searchLineEdit, SIGNAL(textChanged(const QString&)),
               this, SIGNAL(searchChanged(const QString&)));
    }
}

#include "searchbar.moc"
