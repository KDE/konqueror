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

#include "webhistoryinterface.h"

#include <KParts/HistoryProvider>


WebHistoryInterface::WebHistoryInterface(QObject* parent)
{
}

void WebHistoryInterface::addHistoryEntry(const QString& url)
{
    KParts::HistoryProvider::self()->insert(url);
}

bool WebHistoryInterface::historyContains(const QString& url) const
{
    return KParts::HistoryProvider::self()->contains(url);
}
