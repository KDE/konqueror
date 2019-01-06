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

#include <KMessageWidget>

#include <QUrl>


class PasswordBar : public KMessageWidget
{
    Q_OBJECT
public:
    explicit PasswordBar(QWidget *parent = nullptr);
    ~PasswordBar() override;

    QUrl url() const;
    QString requestKey() const;

    void setUrl(const QUrl&);
    void setRequestKey(const QString&);

Q_SIGNALS:
    void saveFormDataRejected(const QString &key);
    void saveFormDataAccepted(const QString &key);
    void done();

private Q_SLOTS:
    void onNotNowButtonClicked();
    void onNeverButtonClicked();
    void onRememberButtonClicked();

private:
    void clear();

    QUrl m_url;
    QString m_requestKey;
};

#endif // PASSWORDBAR_H
