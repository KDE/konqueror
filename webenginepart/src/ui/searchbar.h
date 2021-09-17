/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2008 Laurent Montel <montel@kde.org>
    SPDX-FileCopyrightText: 2008 Benjamin C. Meyer <ben@meyerhome.net>
    SPDX-FileCopyrightText: 2008 Urs Wolfer <uwolfer @ kde.org>
    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit @ kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef SEARCHBAR_P_H
#define SEARCHBAR_P_H

#include <QWidget>
#include <QMenu>

#include "ui_searchbar.h"

class QEvent;

/**
 * This is the widget that shows up when the search is initiated.
 */
class SearchBar : public QWidget
{
    Q_OBJECT

public:
    SearchBar(QWidget *parent = nullptr);
    ~SearchBar() override;

    QString searchText() const;
    bool caseSensitive() const;
    bool highlightMatches() const;
    void setFoundMatch(bool match);
    void setSearchText(const QString&);

    bool event(QEvent* e) override;

public Q_SLOTS:
    void setVisible(bool visible) override;
    void clear();
    void findNext();
    void findPrevious();
    void textChanged(const QString&);

Q_SIGNALS:
    void searchTextChanged(const QString& text, bool backward = false);

private:
    Ui::SearchBar m_ui;
    QPointer<QWidget> m_focusWidget;
    QMenu *m_optionsMenu;
};

#endif // SEARCHBAR_P_H
