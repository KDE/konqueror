/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2011 Dawit Alemayehu <adawit@kde.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef WEBHISTORYINTERFACE_H
#define WEBHISTORYINTERFACE_H

#include <QObject>


class WebHistoryInterface
{
public:
    WebHistoryInterface(QObject* parent = nullptr);
    void addHistoryEntry (const QString & url);
    bool historyContains (const QString & url) const;
};

#endif //WEBHISTORYINTERFACE_H
