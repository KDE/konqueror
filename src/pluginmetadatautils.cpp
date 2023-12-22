/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2022 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "pluginmetadatautils.h"

#include <KParts/PartLoader>

#include <QStringLiteral>

static QString partDir() {
    static QString s_partDir{QStringLiteral("kf" QT_STRINGIFY(QT_VERSION_MAJOR) "/parts")};
    return s_partDir;
}

KPluginMetaData findPartById(const QString& id)
{
    return KPluginMetaData::findPluginById(partDir(), id);
}

KPluginMetaData preferredPart(const QString &mimeType) {
    QVector<KPluginMetaData> plugins = KParts::PartLoader::partsForMimeType(mimeType);
    if (!plugins.isEmpty()) {
        return plugins.first();
    } else {
        return KPluginMetaData();
    }
}

QVector<KPluginMetaData> findParts(std::function<bool (const KPluginMetaData &)> filter)
{
    return KPluginMetaData::findPlugins(partDir(), filter);
}

QVector<KPluginMetaData> findParts(std::function<bool (const KPluginMetaData &)> filter, bool includeDefaultDir)
{
    QVector<KPluginMetaData> plugins = KPluginMetaData::findPlugins(partDir(), filter);
    if (includeDefaultDir) {
        plugins.append(KPluginMetaData::findPlugins(QString(), filter));
    }
    return plugins;
}

QDebug operator<<(QDebug debug, const KPluginMetaData& md)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "KPluginMetaData(name:" << md.name() << ", plugin-id:" << md.pluginId() << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const QVector<KPluginMetaData>& vec)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "QVector<KPluginMetaData> {\n";
    for (const KPluginMetaData &md : vec) {
        debug << '\t' << md << ",\n";
    }
    debug << '}';
    return debug;
}
