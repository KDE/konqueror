//  This file is part of the KDE project
//  SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
// 
//  SPDX-License-Identifier: LGPL-2.0-or-later

#include "allowedurlactionstest.h"
#include "konq_urlactions.h"

#include <QTest>

using namespace Konq;

QTEST_APPLESS_MAIN(AllowedUrlActionsTest);

void AllowedUrlActionsTest::testAllow()
{
    AllowedUrlActions actions{UrlAction::Save, UrlAction::Execute};
    QVERIFY(!actions.isAllowed(Konq::UrlAction::Open));
    actions.allow(Konq::UrlAction::Open, true);
    QVERIFY(actions.isAllowed(Konq::UrlAction::Open));

    QVERIFY(actions.isAllowed(Konq::UrlAction::Execute));
    actions.allow(Konq::UrlAction::Execute, false);
    QVERIFY(!actions.isAllowed(Konq::UrlAction::Execute));

    QVERIFY(!actions.isAllowed(Konq::UrlAction::UnknownAction));
    actions.allow(Konq::UrlAction::UnknownAction, false);
    QVERIFY(!actions.isAllowed(Konq::UrlAction::UnknownAction));
}

void AllowedUrlActionsTest::testForce()
{
    std::initializer_list<UrlAction> list{UrlAction::Save, UrlAction::Open, UrlAction::Embed, UrlAction::Execute, UrlAction::DoNothing};
    for (UrlAction a : list) {
        AllowedUrlActions actions{list};
        QVERIFY(!actions.isForced(a));
        actions.force(a);
        QVERIFY(actions.isForced(a));
    }

    AllowedUrlActions actions{list};
    QVERIFY(!actions.isForced(UrlAction::UnknownAction));
    actions.force(UrlAction::UnknownAction);
    QVERIFY(!actions.isForced(UrlAction::UnknownAction));
}

void AllowedUrlActionsTest::testForcedAction()
{
    QList<UrlAction> list{UrlAction::Save, UrlAction::Open, UrlAction::Embed, UrlAction::Execute, UrlAction::DoNothing};
    for (UrlAction a : list) {
        AllowedUrlActions actions{a};
        QCOMPARE(actions.forcedAction(), a);
    }
    QCOMPARE(AllowedUrlActions{UrlAction::UnknownAction}.forcedAction(), UrlAction::DoNothing);
}

void AllowedUrlActionsTest::testIsForcedRealActions()
{
    QList<UrlAction> list{UrlAction::Save, UrlAction::Open, UrlAction::Embed, UrlAction::Execute};
    for (UrlAction a : list) {
        AllowedUrlActions actions{a};
        QVERIFY(actions.isForced());
        QVERIFY(!actions.isForced(Konq::UrlAction::DoNothing));
        QVERIFY(!actions.isForced(Konq::UrlAction::UnknownAction));
        for (UrlAction a1 : list) {
            if (a1 == a) {
                QVERIFY(actions.isForced(a1));
            } else {
                QVERIFY(!actions.isForced(a1));
            }
        }
    }
}

void AllowedUrlActionsTest::testIsForcedDoNothing()
{
    QList<UrlAction> list{UrlAction::Save, UrlAction::Open, UrlAction::Embed, UrlAction::Execute};
    AllowedUrlActions actions{UrlAction::DoNothing};
    QVERIFY(actions.isForced());
    QVERIFY(actions.isForced(Konq::UrlAction::DoNothing));
    QVERIFY(!actions.isForced(Konq::UrlAction::UnknownAction));
    for (UrlAction a1 : list) {
        QVERIFY(!actions.isForced(a1));
    }
}

void AllowedUrlActionsTest::testIsForcedUnknownAction()
{
    QList<UrlAction> list{UrlAction::Save, UrlAction::Open, UrlAction::Embed, UrlAction::Execute};
    AllowedUrlActions actions{UrlAction::UnknownAction};
    QVERIFY(actions.isForced());
    QVERIFY(actions.isForced(Konq::UrlAction::DoNothing));
    QVERIFY(!actions.isForced(Konq::UrlAction::UnknownAction));
    for (UrlAction a1 : list) {
        QVERIFY(!actions.isForced(a1));
    }
}

void AllowedUrlActionsTest::testIsAllowed()
{
    std::initializer_list<UrlAction> list{UrlAction::Save, UrlAction::Open, UrlAction::Embed};
    QList<UrlAction> expectedAllowed{list};
    AllowedUrlActions actions{list};
    for (UrlAction a : expectedAllowed) {
        QVERIFY(actions.isAllowed(a));
    }
    QVERIFY(!actions.isAllowed(UrlAction::UnknownAction));
    QVERIFY(!actions.isAllowed(UrlAction::Execute));
    QVERIFY(actions.isAllowed(UrlAction::DoNothing));

}
