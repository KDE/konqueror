/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "konqutils.h"

#include <kparts_version.h>
#include <KParts/PartLoader>

#include <QJsonObject>

QStringList Konq::serviceTypes(const KPluginMetaData& md)
{
    //TODO KF6: ensure that this entry will still exist
#if KPARTS_VERSION >= QT_VERSION_CHECK(6,4,0)
    KParts::PartCapabilities capabilities = KParts::PartLoader::partCapabilities(md);
    QStringList serviceTypes;
    if (capabilities & KParts::PartCapability::ReadOnly) {
        serviceTypes << QLatin1String("KParts/ReadOnlyPart");
    }
    if (capabilities & KParts::PartCapability::ReadWrite) {
        serviceTypes <<  QStringLiteral("KParts/ReadWritePart");
    }
    if (capabilities & KParts::PartCapability::BrowserView) {
        serviceTypes << QStringLiteral("Browser/View");
    }
    return serviceTypes;
#else
    return md.rawData().value(QLatin1String("KPlugin")).toObject().value(QLatin1String("ServiceTypes")).toVariant().toStringList();
#endif
}
