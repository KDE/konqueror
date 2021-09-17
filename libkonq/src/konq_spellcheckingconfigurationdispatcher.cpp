/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2021 Stefano Crocco <posta@stefanocrocco.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "konq_spellcheckingconfigurationdispatcher.h"

KonqSpellCheckingConfigurationDispatcher * KonqSpellCheckingConfigurationDispatcher::self()
{
    static KonqSpellCheckingConfigurationDispatcher s_self;
    return &s_self;
}


