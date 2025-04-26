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

/**
 * @brief Namespace for functions related to creating windows
 */
namespace KonqMainWindowFactory
{

/**
 * @brief Returns a new empty window
 *
 * If there's a preloaded window, that window is returned, otherwise a new window
 * is created.
 *
 * The new window is always on the current activity and its URL is `konq:blank`.
 *
 * If the user chose to always have a preloaded window, this function starts a
 * timer which creates a preloaded window after 500ms.
 *
 * @note: the window is not made visible, so the caller of this function must be
 * sure to call `show()` on it.
 *
 * @return an empty Konqueror window
 */
KonqMainWindow *createEmptyWindow();

/**
 * @brief Creates a preloaded window
 *
 * A preloaded window is simply a window which is not visible and points to `konq:blank`.
 *
 * @return The new preloaded window
 */
KonqMainWindow *createPreloadWindow();

/**
 * @brief Creates a new window showing the given URL
 *
 * If there's a preloaded window, that window is used, otherwise a new window
 * is created. In both cases, @p url is opened in the window.
 *
 * @note The returned window is not visible, so the caller must call `show()` on
 * it
 *
 * @param url the URL to open in the new window
 * @param req information about how to open the URL
 * @return a new or preloaded window which shows @p url
 */
KONQ_TESTS_EXPORT KonqMainWindow *createNewWindow(const QUrl &url = QUrl(),
        const KonqOpenURLRequest &req = KonqOpenURLRequest());

/**
 * @brief Finds a preloaded window, if it exists
 *
 * @return an existing preloaded window or `nullptr` if no preloaded window
 * exists
 */
KonqMainWindow *findPreloadedWindow();

};

#endif // KONQMAINWINDOWFACTORY_H
