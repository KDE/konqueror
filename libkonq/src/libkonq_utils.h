/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef LIBKONQ_UTILS_H
#define LIBKONQ_UTILS_H

#include "libkonq_export.h"

class KBookmarkManager;

namespace LIBKONQ_EXPORT Konq {
    KBookmarkManager* userBookmarksManager();
}
#endif //LIBKONQ_UTILS_H

