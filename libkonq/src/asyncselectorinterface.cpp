// /* This file is part of the KDE project
//     SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>
// 
//     SPDX-License-Identifier: LGPL-2.0-or-later
// */

#include "asyncselectorinterface.h"

AsyncSelectorInterface::~AsyncSelectorInterface()
{
}

KParts::SelectorInterface::QueryMethods AsyncSelectorInterface::supportedAsyncQueryMethods() const
{
    return KParts::SelectorInterface::None;
}
