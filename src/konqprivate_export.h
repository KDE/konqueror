/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KONQPRIVATE_EXPORT_H
#define KONQPRIVATE_EXPORT_H

#include "konquerorprivate_export.h"

/* Classes from the Konqueror application, which are exported only for unit tests */
#ifdef BUILD_TESTING
# ifndef KONQ_TESTS_EXPORT
#  define KONQ_TESTS_EXPORT KONQUERORPRIVATE_EXPORT
# endif
#else /* not compiling tests */
# define KONQ_TESTS_EXPORT
#endif

#endif
