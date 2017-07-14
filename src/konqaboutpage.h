/*
 *  This file is part of the KDE project
 *  Copyright (C) 2017 David Faure <faure@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef KONQ_ABOUTPAGE_H
#define KONQ_ABOUTPAGE_H

#include <konqprivate_export.h>
#include <QString>

class QUrl;

class KONQUERORPRIVATE_EXPORT KonqAboutPage
{
public:
    KonqAboutPage();
    ~KonqAboutPage();

    static QString aboutProtocol();

    QString pageContents(const QString &path) const;

private:
    QString launch() const;
    QString intro() const;
    QString specs() const;
    QString tips() const;
    QString plugins() const;
};

#endif
