// /* This file is part of the KDE project
//     SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>
// 
//     SPDX-License-Identifier: LGPL-2.0-or-later
// */

#ifndef ASYNCSELECTORINTERFACE_H
#define ASYNCSELECTORINTERFACE_H

#include <QtGlobal>
#include <QSharedDataPointer>
#include <QString>
#include <QtPlugin>

#include <libkonq_export.h>

#include <functional>

/**
 * @brief Alternative to KParts::SelectorInterface which provides an asynchronous API
 *
 * This interface closely mimics the API of KParts::SelectorInterface
 */
class LIBKONQ_EXPORT AsyncSelectorInterface
{
public:

    /**
     * @brief Destructor
     */
    virtual ~AsyncSelectorInterface();

    class Element;
    class ElementPrivate;

    enum QueryMethod {
        None = 0x00, /*!< Querying is not possible. */
        EntireContent = 0x01, /*!< Query or can query the entire content. */
        SelectedContent = 0x02, /*!< Query or can query only the user selected content, if any. */
    };
    /**
     * Stores a combination of #QueryMethod values.
     */
    Q_DECLARE_FLAGS(QueryMethods, QueryMethod)

    /**
     * Returns the supported query methods.
     *
     * By default this function returns None.
     *
     * @see QueryMethod
     */
    virtual QueryMethods supportedAsyncQueryMethods() const;

    /**
     * @brief A function taking a single KParts::SelectorInterface::Element as argument and without return value
     */
    typedef const std::function<void (const Element &)> SingleElementSelectorCallback;

    /**
     * @brief A function taking a `QList<Element>` as argument and without return value
     */
    typedef const std::function<void (const QList<Element> &)> MultipleElementSelectorCallback;

    /**
     * @brief Passes to the given callback the first (in document order) element in this fragment matching
     * the given CSS selector and querying method
     *
     * If no element satisfying the given query is found, including the case where the requested query method is not supported,
     * the callback must be called with an invalid element.
     * @see KParts::SelectorInterface::querySelector()
     * @note This function is _asynchronous_
     * @param query the query containing the string
     * @param method the method to use for the query
     * @param callback the function to call with the found element
     */
    virtual void querySelectorAsync(const QString &query, QueryMethod method, SingleElementSelectorCallback& callback) = 0;

    /**
     * @brief Passes to the given callback all the elements in this fragment matching the given CSS selector and querying method
     *
     * If no element satisfying the given query is found, including the case where the requested query method is not supported,
     * the callback must be called with an empty list.
     * @see KParts::SelectorInterface::querySelector()
     * @note This function is _asynchronous_
     * @param query the query containing the string
     * @param method the method to use for the query
     * @param callback the function to call with the found elements
     */
    virtual void querySelectorAllAsync(const QString &query, QueryMethod method, MultipleElementSelectorCallback& callback) = 0;

    //Code for this class copied from kparts/selectorinterface.h (KF 5.110) written by David Faure <faure@kde.org>
    class LIBKONQ_EXPORT Element
    {
    public:
        /**
        * Constructor
        */
        Element();

        /**
        * Copy constructor
        */
        Element(const Element &other);

        /**
        * Destructor
        */
        ~Element();

        /**
        * Returns true if the element is null ; otherwise returns false.
        */
        bool isNull() const;

        /**
        * Sets the tag name of this element.
        */
        void setTagName(const QString &tag);

        /**
        * Returns the tag name of this element.
        */
        QString tagName() const;

        /**
        * Adds an attribute with the given name and value.
        * If an attribute with the same name exists, its value is replaced by value.
        */
        void setAttribute(const QString &name, const QString &value);

        /**
        * Returns the list of attributes in this element.
        */
        QStringList attributeNames() const;

        /**
        * Returns the attribute with the given name. If the attribute does not exist, defaultValue is returned.
        */
        QString attribute(const QString &name, const QString &defaultValue = QString()) const;

        /**
        * Returns true if the attribute with the given @p name exists.
        */
        bool hasAttribute(const QString &name) const;

        // No namespace support yet, could be added with attributeNS, setAttributeNS

        /**
        * Swaps the contents of @p other with the contents of this.
        */
        void swap(Element &other)
        {
            d.swap(other.d);
        }

        /**
        * Assignment operator
        */
        Element &operator=(const Element &other)
        {
            if (this != &other) {
                Element copy(other);
                swap(copy);
            }
            return *this;
        }

    private:
        QSharedDataPointer<ElementPrivate> d;
    };
};

inline void qSwap(AsyncSelectorInterface::Element &lhs, AsyncSelectorInterface::Element &rhs)
{
    lhs.swap(rhs);
}

Q_DECLARE_TYPEINFO(AsyncSelectorInterface::Element, Q_MOVABLE_TYPE);

Q_DECLARE_INTERFACE(AsyncSelectorInterface, "org.kde.libkonq.AsyncSelectorInterface")

#endif // ASYNCSELECTORINTERFACE_H
