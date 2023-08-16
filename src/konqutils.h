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
}
#endif //KONQUTILS_H
