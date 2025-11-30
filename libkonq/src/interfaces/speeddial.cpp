//  This file is part of the KDE project
//  SPDX-FileCopyrightText: 2025 Stefano Crocco <stefano.crocco@alice.it>
// 
//  SPDX-License-Identifier: LGPL-2.0-or-later

#include "speeddial.h"

#include "common.h"

using namespace KonqInterfaces;

SpeedDial::SpeedDial(QObject* parent) : QObject(parent)
{
}

SpeedDial::~SpeedDial() = default;

SpeedDial* KonqInterfaces::SpeedDial::speedDial(QObject* parent)
{
    return as<SpeedDial>(parent);
}
