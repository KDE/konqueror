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

QVector<KPluginMetaData> findParts(std::function<bool(const KPluginMetaData &)> filter, bool includeDefaultDir);

QVector<KPluginMetaData> findParts(std::function<bool(const KPluginMetaData &)> filter={});

QStringList pluginIds(const QList<KPluginMetaData> &mds);

QDebug operator<<(QDebug debug, const KPluginMetaData &md);

QDebug operator<<(QDebug debug, const QVector<KPluginMetaData> &vec);

#endif //PLUGINMETADATAUTILS_H
