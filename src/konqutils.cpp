/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "konqutils.h"

#include <QJsonObject>

QStringList Konq::serviceTypes(const KPluginMetaData& md)
{
    //TODO KF6: ensure that this entry will still exist
    return md.rawData().value(QLatin1String("KPlugin")).toObject().value(QLatin1String("ServiceTypes")).toVariant().toStringList();
}

