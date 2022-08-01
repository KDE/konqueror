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

QDebug operator<<(QDebug debug, const KPluginMetaData &md);

QDebug operator<<(QDebug debug, const QVector<KPluginMetaData> &vec);

#endif //PLUGINMETADATAUTILS_H
