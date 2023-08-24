/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/
#ifndef KONQUTILS_H
#define KONQUTILS_H

#include <KPluginMetaData>

namespace Konq
{
    /**
     * @brief The service types implemented by a plugin
     * @note This function is only a workaround for the deprecation of `KPluginMetaData::serviceTypes()`. However it still uses the
     * `ServiceTypes` entry in the plugin metadata. If `KPluginMetaData::serviceTypes()` will be removed in KF6, it is possible that
     * this entry will be removed, too.
     * @todo Find a replacement for the `ServiceTypes` metadata entry
     * @param md the metadata for the plugin
     * @return A list of the service types implemented by @p md
     */
    QStringList serviceTypes(const KPluginMetaData &md);

    /**
     * @brief Creates an URL with scheme `error` describing the error occurred while attempting to load an URL
     *
     * @param error the `KIO` error code (or `KIO::ERR_WORKER_DEFINED` if not from `KIO`)
     * @param errorText the text of the error message
     * @param initialUrl the URL that we were trying to open
     * @return the `error` URL
     */
    QUrl makeErrorUrl(int error, const QString &errorText, const QUrl &initialUrl);

}
#endif //KONQUTILS_H
