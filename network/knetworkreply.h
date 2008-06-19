/*
 * This file is part of the KDE project.
 *
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

#ifndef KNETWORKREPLY_H
#define KNETWORKREPLY_H

#include <QNetworkReply>

class KNetworkReply : public QNetworkReply
{
    Q_OBJECT
public:
    KNetworkReply(QObject *parent);

    virtual void abort();

protected:
    virtual qint64 readData(char *data, qint64 maxSize);
};

#endif // KNETWORKREPLY_H
