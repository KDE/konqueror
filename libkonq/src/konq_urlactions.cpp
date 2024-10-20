/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "konq_urlactions.h"

#include <QDebug>

using namespace Konq;

AllowedUrlActions::AllowedUrlActions() : QList<UrlAction>{UrlAction::Save, UrlAction::Embed, UrlAction::Open, UrlAction::Execute}
{
}

AllowedUrlActions::AllowedUrlActions(std::initializer_list<UrlAction> actions) : QList()
{
    reserve(actions.size());
    for (UrlAction a : actions) {
        if (a != UrlAction::DoNothing && a != UrlAction::UnknownAction && !contains(a)) {
            append(a);
        }
    }
}

bool AllowedUrlActions::isAllowed(UrlAction action) const {
    return action == UrlAction::DoNothing ? true : contains(action);
}

bool AllowedUrlActions::isForced(UrlAction action) const {
    if (action == UrlAction::UnknownAction || count() > 1) {
        return false;
    } else if (isEmpty()) {
        return action == UrlAction::DoNothing;
    }
    return first() == action;
}

bool AllowedUrlActions::isForced() const
{
    if (isEmpty()) {
        return true;
    } else {
        return count() == 1 && first() != UrlAction::UnknownAction;
    }
}

UrlAction AllowedUrlActions::forcedAction() const {
    if (isEmpty()) {
        return UrlAction::DoNothing;
    } else {
        return count() == 1 ? first() : UrlAction::UnknownAction;
    }
}

bool AllowedUrlActions::force(UrlAction action) {
    if (action != UrlAction::UnknownAction) {
        clear();
        if (action != UrlAction::UnknownAction) {
            append(action);
        }
        return true;
    } else {
        return false;
    }
}

void AllowedUrlActions::allow(UrlAction action, bool allowed){
    if (allowed && !contains(action)) {
        append(action);
    } else if (!allowed) {
        removeOne(action);
    }
    if (isEmpty()) {
        append(UrlAction::DoNothing);
    }
}

QDebug Konq::operator<<(QDebug dbg, UrlAction action)
{
    QDebugStateSaver saver(dbg);
    dbg.resetFormat();
    switch (action) {
        case UrlAction::UnknownAction:
            dbg << "UnknownAction";
            break;
        case UrlAction::DoNothing:
            dbg << "DoNothing";
            break;
        case UrlAction::Save:
            dbg << "Save";
            break;
        case UrlAction::Embed:
            dbg << "Embed";
            break;
        case UrlAction::Open:
            dbg << "Open";
            break;
        case UrlAction::Execute:
            dbg << "Execute";
            break;
    }
    return dbg;
}

QDebug Konq::operator<<(QDebug dbg, const AllowedUrlActions &actions)
{
    QDebugStateSaver saver(dbg);
    dbg.resetFormat();
    return dbg << static_cast<QList<UrlAction>>(actions);
}
