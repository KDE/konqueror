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

/**
 * @brief Convenience class to create a `KParts::ReadOnlyPart` instance using a `KPluginFactory`
 *
 * This class stores the information needed to create the part, including the factory itself,
 * and provides a method to create it later.
 *
 * A KonqViewFactory can be _null_: a KonqViewFactory is null if it doesn't have a factory
 */
class KonqViewFactory // TODO rename to KonqPartLoader?
{
public:
    /**
     * @brief Constructor which creates a null factory
     */
    KonqViewFactory() : m_factory(nullptr), m_args() {}

    /**
     * @brief Constructor
     * @param data the metadata of the part to load with this object
     * @param factory the plugin factory to use to load the part
     */
    KonqViewFactory(const KPluginMetaData &data, KPluginFactory *factory);

    // The default copy ctor and operator= can be used, this is a value class.

    /**
     * @brief Sets the arguments the plugin factory should use when creating the part
     *
     * @param args the arguments to pass as last argument to `KPluginFactory::create()`
     */
    void setArgs(const QVariantList &args);

    /**
     * @brief Creates a part corresponding to the metadata set in the constructor
     *
     * This is a wrapper around `KPluginFactory::create(QWidget*, QObject*, const QVariantList&)
     * which uses the plugin factory, metadata and arguments stored in this object.
     *
     * @param parentWidget the parent of the part widget
     * @param parent the part's parent
     * @return the new part or `nullptr` if a new part couldn't be created. If the factory
     * is null, this will always return `nullptr`
     */
    KParts::ReadOnlyPart *create(QWidget *parentWidget, QObject *parent);

    /**
     * @brief Whether the factory is null or not
     * @return `true` if the factory is null and `false` otherwise
     */
    bool isNull() const
    {
        return m_factory ? false : true;
    }

private:
    KPluginMetaData m_metaData; //!< The metadata of the part to create
    KPluginFactory *m_factory; //!< The plugin factory to use to create the part
    QVariantList m_args; //!< The variant list argument to pass to `KPluginFactory::create()`
};

/**
 * @brief Helper class for creating parts for a view
 */
class KONQ_TESTS_EXPORT KonqFactory
{
public:

    /**
     * @brief Creates a KonqViewFactory which can be used to create a part according
     * to the given information
     *
     * @param type the mimetype or part capability the part should have
     * @param serviceName the name of the part to create. If it's an empty string, a suitable part
     * according to @p type will be determined automatically
     * @param serviceImpl [out] if not `nullptr`, it will point to the metadata of the part the factory will create.
     * @param partServiceOffers [out] if not `nullptr`, it will point to a list of all available parts for @p type
     * @param appServiceOffers [out] if not `nullptr`, it will point to a list of all applications which can handle @p type
     * @param forceAutoEmbed whether or not to always prefer embedding to opening in an external application
     * @return a KonqViewFactory which creates the most suitable part for displaying a view with type @p type.
     * If, according to the user settings, the mimetype @p type should be shown in an external application and @p serviceName
     * is empty, a null KonqViewFactory will be returned
     *
     * @internal
     * @note This is not a static method so that we can define an abstract base class
     * with another implementation, for unit tests, if wanted.
     * @endinternal
     */
    KonqViewFactory createView(const Konq::ViewType &type,
                               const QString &serviceName = QString(),
                               KPluginMetaData *serviceImpl = nullptr,
                               QVector<KPluginMetaData> *partServiceOffers = nullptr,
                               KService::List *appServiceOffers = nullptr,
                               bool forceAutoEmbed = false);

    /**
     * @brief Determines the available parts and applications for a given view type
     *
     * @param type the type of view
     * @param [out] partServiceOffers if not `nullptr`, it will point to a list of parts of type @p type
     * @param [out] appServiceOffers if not `nullptr`, it will point to a list of applications for type @p type.
     * If @p type is a part capability, this will always be an empty list
     */
    static void getOffers(const Konq::ViewType &type, QVector<KPluginMetaData> *partServiceOffers = nullptr, KService::List* appServiceOffers = nullptr);
};

#endif
