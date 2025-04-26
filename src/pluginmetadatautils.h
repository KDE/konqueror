/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2022 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef PLUGINMETADATAUTILS_H
#define PLUGINMETADATAUTILS_H

#include <KPluginMetaData>

#include <QDebug>

KPluginMetaData findPartById(const QString &id);

/**
 * @brief Finds the meta data for the preferred part to display the given mime type
 * @param mimeType the mime type
 * @return the plugin meta data for the preferred part to display @p mimeType or an
 * invalid @c KPluginMetaData if no part is available for the mime type
 */
KPluginMetaData preferredPart(const QString &mimeType);

/**
 * @brief Find all parts satisfying the given criteria
 *
 * Parts are searched in `kf6/parts` and, if @p includeDefaultDir is `true`, also
 * in the directories returned by `QCoreApplication::libraryPaths()`.
 *
 * @param filter a function to determine if a part is acceptable or not. It must return
 * `true` if the part should be included among the returned ones and `false` otherwise.
 * @param includeDefaultDir whether to also search for parts in the directories returned
 * by `QCoreApplication::libraryPaths()`
 * @return a list of the plugin metadata for available parts matching @p filter
 */
QVector<KPluginMetaData> findParts(std::function<bool(const KPluginMetaData &)> filter, bool includeDefaultDir);

/**
 * @brief Overload of findParts(std::function<bool(const KPluginMetaData &)>, bool)
 *
 * It only looks for parts in `kf6/parts`.
 *
 * @param filter a function to determine if a part is acceptable or not. It must return
 * `true` if the part should be included among the returned ones and `false` otherwise. If
 * not given, all parts are returned.
 *
 * @return a list of the plugin metadata for available parts matching @p filter
 */
QVector<KPluginMetaData> findParts(std::function<bool(const KPluginMetaData &)> filter={});

/**
 * @brief Returns a list of ids for the givne plugin
 *
 * @param mds the metadata associated with the plugins
 * @return a list with the `KPluginMetaData::id()` of the moedatata in @p mds
 */
QStringList pluginIds(const QList<KPluginMetaData> &mds);

/**
 * @brief Debug operator for a `KPluginMetaData`
 *
 * It prints the plugin id of the metadata.
 *
 * @param debug the debug object
 * @param md the object to print
 */
QDebug operator<<(QDebug debug, const KPluginMetaData &md);

/**
 * @brief Debug operator for a vector of `KPluginMetaData`
 *
 * It prints each metadata.
 *
 * @param debug the debug object
 * @param vec the object to print
 */
QDebug operator<<(QDebug debug, const QVector<KPluginMetaData> &vec);

#endif //PLUGINMETADATAUTILS_H
