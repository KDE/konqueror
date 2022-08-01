/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2022 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "pluginmetadatautils.h"

#include <QStringLiteral>

KPluginMetaData findPartById(const QString& id)
{
    return KPluginMetaData::findPluginById(QStringLiteral("kf5/parts"), id);
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
