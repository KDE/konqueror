/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef LIBKONQ_UTILS_H
#define LIBKONQ_UTILS_H

#include "libkonq_export.h"

#include <QUrl>

class KBookmarkManager;
class QWidget;

namespace Konq {
   LIBKONQ_EXPORT KBookmarkManager* userBookmarksManager();

    /**
     * @brief Creates an URL with scheme `error` describing the error occurred while attempting to load an URL
     *
     * @param error the `KIO` error code (or `KIO::ERR_WORKER_DEFINED` if not from `KIO`)
     * @param errorText the text of the error message
     * @param initialUrl the URL that we were trying to open
     * @return the `error` URL
    */
    LIBKONQ_EXPORT QUrl makeErrorUrl(int error, const QString &errorText, const QUrl &initialUrl);

    /**
     * @brief Displays a `Save as` dialog asking for the location where to download an URL
     * @param suggestedFileName the suggested file name for the downloaded file
     * @param parent the dialog's parent
     * @param the starting directory for the dialog. If omitted, the standard download location will be used
     * @return a string with the download path chosen by the user or an empty string if the user cancels the dialog
     */
    LIBKONQ_EXPORT QString askDownloadLocation(const QString &suggestedFileName, QWidget *parent = nullptr, const QString &startingDir = {});
}
#endif //LIBKONQ_UTILS_H

