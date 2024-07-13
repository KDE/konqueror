/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 1999 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 1999 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 1999 Torben Weis <weis@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef __konq_factory_h__
#define __konq_factory_h__

#include "konqprivate_export.h"
#include "pluginmetadatautils.h"
#include "konqutils.h"

#include <KService>
#include <KPluginMetaData>

class KPluginFactory;
namespace KParts
{
class ReadOnlyPart;
}

class KonqViewFactory // TODO rename to KonqPartLoader?
{
public:
    /**
     * Create null factory
     */
    KonqViewFactory() : m_factory(nullptr), m_args() {}

    KonqViewFactory(const KPluginMetaData &data, KPluginFactory *factory);

    // The default copy ctor and operator= can be used, this is a value class.

    void setArgs(const QVariantList &args);

    KParts::ReadOnlyPart *create(QWidget *parentWidget, QObject *parent);

    bool isNull() const
    {
        return m_factory ? false : true;
    }

private:
    KPluginMetaData m_metaData;
    KPluginFactory *m_factory;
    QVariantList m_args;
};

/**
 * Factory for creating (loading) parts when creating a view.
 */
class KONQ_TESTS_EXPORT KonqFactory
{
public:
    /**
     * Return the factory that can be used to actually create the part inside a view.
     *
     * The implementation locates the part module (library), using the trader
     * and opens it (using klibfactory), which gives us a factory that can be used to
     * actually create the part (later on, when the KonqView exists).
     *
     * Not a static method so that we can define an abstract base class
     * with another implementation, for unit tests, if wanted.
     */
    KonqViewFactory createView(const Konq::ViewType &type,
                               const QString &serviceName = QString(),
                               KPluginMetaData *serviceImpl = nullptr,
                               QVector<KPluginMetaData> *partServiceOffers = nullptr,
                               KService::List *appServiceOffers = nullptr,
                               bool forceAutoEmbed = false);

    static void getOffers(const Konq::ViewType &type, QVector<KPluginMetaData> *partServiceOffers = nullptr, KService::List* appServiceOffers = nullptr);
};

#endif
