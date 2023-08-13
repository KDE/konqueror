/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "browser.h"
#include "interfaces/common.h"

using namespace KonqInterfaces;

Browser::Browser(QObject* parent) : QObject(parent)
{
}

KonqInterfaces::Browser::~Browser() noexcept
{
}

Browser* Browser::browser(QObject* obj)
{
    return as<Browser>(obj);
}
