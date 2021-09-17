/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2011 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
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
