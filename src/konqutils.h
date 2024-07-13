/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/
#ifndef KONQUTILS_H
#define KONQUTILS_H

#include <KPluginMetaData>
#include <kparts_version.h>

#include <QDebug>

#include <variant>
#include <optional>

#if KPARTS_VERSION >= QT_VERSION_CHECK(6,4,0)
#include <KParts/PartLoader>
#else
namespace KParts {
    enum class PartCapability {
        ReadOnly = 1,
        ReadWrite = 2,
        BrowserView = 4,
    }
    Q_DECLARE_FLAGS(PartCapabilities, PartCapability)
    Q_DECLARE_OPERATORS_FOR_FLAGS(PartCapabilities)
}
#endif

namespace Konq {

    //TODO Remove this function and use KParts::PartLoader::partCapabilities() when depending on
    //KParts >= 6.4.0
    /**
     * @brief Utility function to retrieve the capabilities of a part
     *
     * When using a version of KParts from 6.4.0, this is just a wrapper for
     * KParts::PartLoader::partCapabilities(). When using a previous version of
     * KParts, it reads the `Services` entry from @p md
     * @note This function will be removed when the minimum KParts version will be 6.4.0 or greater:
     * at that point, it will be possible to use KParts::PartLoader::partCapabilities()
     * directly
     * @param md the metadata associated with the part
     * @return the capabilities of the part
     */
    KParts::PartCapabilities partCapabilities(const KPluginMetaData &md);

    /**
     * @brief Class representing the type of a view
     *
     * A view type can be a either mimetype or a part capability, but not both.
     *
     * Use hasMimetype() and hasCapability() to determine which of the two options is
     * contained in an instance and mimetype() and capability() to retrieve them (note
     * that this functions return `std::optional` values.
     */
    class ViewType : public std::variant<QString, KParts::PartCapability> {
    public:

        /**
         * @brief Default constructor
         *
         * It constructs a ViewType representing an empty mimetype
         */
        ViewType();

        /**
         * @brief Constructor which creates a ViewType representing a mimetype
         * @param mime the mimetype. It should be either an empty string or a valid mimetype
         * @note Don't use this constructor to create an object based to its string representation
         * as returned by toString(). This constructor always interpret @p mime as a mimetype,
         * which means that passing, for example, `BrowserView`, will create an object
         * representing the (invalid) mimetype `BrowserView` and not (as expected) an object
         * representing the part capability `KParts::Capability::BrowserView`
         */
        ViewType(const QString &mime);

        /**
         * @brief Constructor which creates a ViewType representing a part capability
         * @param cap the part capability
         */
        ViewType(KParts::PartCapability cap);

        /**
         * @brief The mimetype represented by the ViewType
         * @return The mimetype represented by this object or `std::nullopt` if
         * this object represents a part capability
         */
        std::optional<QString> mimetype() const;

        /**
         * @brief The part capability represented by the ViewType
         * @return The part capability represented by this object or `std::nullopt` if
         * this object represents a mimetype
         */
        std::optional<KParts::PartCapability> capability() const;

        /**
         * @brief Whether this object represents a mimetype
         * @return `true` if this object represents a mimetype, that is if mimetype()
         * would return a string, and `false` if it represents a capability, that is if
         * mimetype() would return `std::nullopt`
         */
        bool isMimetype() const;

        /**
         * @brief Whether this object represents a part capability
         * @return `true` if this object represents a part capability, that is if capability()
         * would return a part capability, and `false` if it represents a mimetype, that is if
         * capability() would return `std::nullopt`
         */
        bool isCapability() const;

        /**
         * @brief Whether the object is empty
         *
         * @return `true` if the object represents a mimetype and the mimetype is an empty
         * string and `false` otherwise
         */
        bool isEmpty() const;

        /**
         * @brief Makes the object represent the given mimetype
         *
         * After a call to this function, isMimetype() will return `true` and
         * isCapability() will return `false`
         * @param mimetype the mimetype the object will represent
         */
        void setMimetype(const QString &mimetype);

        /**
         * @brief Makes the object represent the given part capability
         *
         * After a call to this function, isCapability() will return `true` and
         * isMimetype() will return `false`
         * @param capability the part capability the object will represent
         */
        void setCapability(KParts::PartCapability capability);

        /**
         * @brief A string representation of the object
         *
         * @return a string representation of the object. This is:
         * - the mimetype itself if the object represents a mimetype
         * - one of `KParts/ReadOnlyPart`, `KParts/ReadWritePart` or `Browser/View` if
         * the object represents a part capability
         */
        QString toString() const;

        /**
         * @brief Creates an object from its string representation as returned by toString()
         * @param str the string representation of the object
         * @return a ViewType representing a part capability if @p str is `KParts/ReadOnlyPart`,
         * `KParts/ReadWritePart` or `Browser/View` and a ViewType representing the mimetype
         * @p str otherwise
         */
        static ViewType fromString(const QString &str);
    };
}

QDebug operator <<(QDebug dbg, const Konq::ViewType &type);


#endif //KONQUTILS_H
