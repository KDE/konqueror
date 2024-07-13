/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "konqutils.h"

#include <kparts_version.h>
#include <KParts/PartLoader>

#include <QJsonObject>

using namespace Konq;

KParts::PartCapabilities Konq::partCapabilities(const KPluginMetaData& md)
{
#if KPARTS_VERSION >= QT_VERSION_CHECK(6,4,0)
    return KParts::PartLoader::partCapabilities(md);
#else
    QStringList serviceTypes = md.rawData().value(QLatin1String("KPlugin")).toObject().value(QLatin1String("ServiceTypes")).toVariant().toStringList();
    KParts::partCapabilities capabilities = KParts::ReadOnly;
    if (serviceTypes.contains(QLatin1String("KParts/ReadWritePart"))) {
        capabilities |= KParts::ReadWritePart;
    }
    if (ServiceTypes.contains(QStringLiteral("Browser/View"))) {
        capabilities |= KParts::PartCapability::BrowserView;
    }
    return capabilities;
#endif
}

Konq::ViewType::ViewType(const QString& mime) : std::variant<QString, KParts::PartCapability>(mime)
{
}

Konq::ViewType::ViewType(KParts::PartCapability cap) : std::variant<QString, KParts::PartCapability>(cap)
{
}

Konq::ViewType::ViewType() : std::variant<QString, KParts::PartCapability>()
{
}

bool Konq::ViewType::isMimetype() const
{
    return std::holds_alternative<QString>(*this);
}

bool Konq::ViewType::isCapability() const
{
    return std::holds_alternative<KParts::PartCapability>(*this);
}

std::optional<QString> Konq::ViewType::mimetype() const
{
    const QString *value = std::get_if<QString>(this);
    return value ? std::optional{*value} : std::nullopt;
}

std::optional<KParts::PartCapability> Konq::ViewType::capability() const
{
    const KParts::PartCapability *capability = std::get_if<KParts::PartCapability>(this);
    return capability ? std::optional{*capability} : std::nullopt;
}

void Konq::ViewType::setMimetype(const QString& mimetype)
{
    emplace<QString>(mimetype);
}

void Konq::ViewType::setCapability(KParts::PartCapability capability)
{
    emplace<KParts::PartCapability>(capability);
}

QString Konq::ViewType::toString() const
{
    if (isMimetype()) {
        return mimetype().value();
    } else {
        KParts::PartCapability cap = capability().value();
        switch (cap) {
            case KParts::PartCapability::ReadOnly:
                return QStringLiteral("KParts/ReadOnlyPart");
            case KParts::PartCapability::ReadWrite:
                return QStringLiteral("KParts/ReadWritePart");
            case KParts::PartCapability::BrowserView:
                return QStringLiteral("Browser/View");
            default: //Should never happen
                return {};
        }
    }
}

bool Konq::ViewType::isEmpty() const
{
    return isMimetype() && mimetype().value().isEmpty();
}

ViewType Konq::ViewType::fromString(const QString& str)
{
    KParts::PartCapability cap = KParts::PartCapability::ReadOnly;
    if (str == QLatin1String("KParts/ReadWritePart")) {
        cap = KParts::PartCapability::ReadWrite;
    } else if (str == QLatin1String("Browser/View")) {
        cap = KParts::PartCapability::BrowserView;
    } else if (str != QLatin1String("KParts/ReadOnlyPart")) {
        return {str}; //Treat anything except the three values above as a mimetype
    }
    return {cap};
}

QDebug operator<<(QDebug dbg, const Konq::ViewType &type)
{
    QDebugStateSaver state(dbg);
    dbg.nospace() << QStringLiteral("Konq::ViewType<");
    dbg << type.toString();
    dbg << '>';
    return dbg;
}
