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

#ifndef WEBKITSEARCHBAR_H
#define WEBKITSEARCHBAR_H

#include <QWidget>

class KLineEdit;
class QTimer;
class QCheckBox;

class WebKitSearchBar : public QWidget
{
    Q_OBJECT
public:
    WebKitSearchBar(QWidget *parent);
    ~WebKitSearchBar();
    QString searchText() const;
    bool caseSensitive() const;

    virtual void setVisible(bool visible);

    void setFoundMatch(bool match);

protected slots:
    void notifySearchChanged();

protected:
    bool eventFilter(QObject* watched, QEvent* event);
signals:
    void searchChanged(const QString& text);
    void closeClicked();
    void findNextClicked();
    void findPreviousClicked();

private:
    KLineEdit* m_searchEdit;
    QCheckBox* m_caseSensitive;
    QTimer* m_searchTimer;

};

#endif /* WEBKITSEARCHBAR_H */
