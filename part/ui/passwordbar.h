/*
 * This file is part of the KDE project.
 *
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
 */

#ifndef PASSWORDBAR_H
#define PASSWORDBAR_H

#include <QtGui/QWidget>

class QUrl;

namespace KDEPrivate {

class PasswordBar : public QWidget
{
    Q_OBJECT

public:
    PasswordBar(QWidget *parent = 0);
    ~PasswordBar();

Q_SIGNALS:
    void saveFormDataRejected(const QString &key);
    void saveFormDataAccepted(const QString &key);

public Q_SLOTS:
    void onSaveFormData(const QString &key, const QUrl &url);

private Q_SLOTS:
    void onNotNowButtonClicked();
    void onNeverButtonClicked();
    void onRememberButtonClicked();

private:
    class PasswordBarPrivate;
    PasswordBarPrivate * const d;
};

}
#endif // PASSWORDBAR_H
