/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "libkonq_utils.h"

#include <KBookmarkManager>
#include <QStandardPaths>

using namespace Konq;

KBookmarkManager * Konq::userBookmarksManager()
{
#if QT_VERSION_MAJOR < 6
    return KBookmarkManager::userBookmarksManager();
#else
    static const QString bookmarksFile = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/konqueror/bookmarks.xml");
    static KBookmarkManager s_manager(bookmarksFile);
    return &s_manager;
#endif
}

