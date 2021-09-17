/* This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999, 2016 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQMAINWINDOWFACTORY_H
#define KONQMAINWINDOWFACTORY_H

#include "konqopenurlrequest.h"
#include "konqprivate_export.h"

class KonqMainWindow;
class QUrl;

namespace KonqMainWindowFactory
{

/**
 * Create a new empty window.
 * This layer on top of the KonqMainWindow constructor allows to reuse preloaded windows,
 * and offers restoring windows after a crash.
 * Note: the caller must call show()
 */
KonqMainWindow *createEmptyWindow();

/**
 * Create a new window for @p url using @p args and @p req.
 * This layer on top of the KonqMainWindow constructor allows to reuse preloaded windows,
 * and offers restoring windows after a crash.
 * Note: the caller must call show()
 */
KONQ_TESTS_EXPORT KonqMainWindow *createNewWindow(const QUrl &url = QUrl(),
        const KonqOpenURLRequest &req = KonqOpenURLRequest());
};

#endif // KONQMAINWINDOWFACTORY_H
