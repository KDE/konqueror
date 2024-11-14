//  This file is part of the KDE project
//  SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
// 
//  SPDX-License-Identifier: LGPL-2.0-or-later

#ifndef ALLOWEDURLACTIONSTEST_H
#define ALLOWEDURLACTIONSTEST_H

#include <QObject>

class AllowedUrlActionsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testAllow();
    void testForce();
    void testForcedAction();
    void testIsForcedRealActions();
    void testIsForcedDoNothing();
    void testIsForcedUnknownAction();
    void testIsAllowed();
};

#endif // ALLOWEDURLACTIONSTEST_H
