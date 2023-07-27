/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2011 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "webhistoryinterface.h"

#include <historyprovider.h>


WebHistoryInterface::WebHistoryInterface(QObject* parent)
{
}

void WebHistoryInterface::addHistoryEntry(const QString& url)
{
    HistoryProvider::self()->insert(url);
}

bool WebHistoryInterface::historyContains(const QString& url) const
{
    return HistoryProvider::self()->contains(url);
}
