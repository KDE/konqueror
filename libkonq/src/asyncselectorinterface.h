// /* This file is part of the KDE project
//     SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>
// 
//     SPDX-License-Identifier: LGPL-2.0-or-later
// */

#ifndef ASYNCSELECTORINTERFACE_H
#define ASYNCSELECTORINTERFACE_H

#include <KParts/SelectorInterface>

#include <libkonq_export.h>

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

    /**
     * @brief As KParts::SelectorInterface::supportedQueryMethods
     * @return the query methods supported by the part. The default implementation returns KParts::SelectorInterface::None
     */
    virtual KParts::SelectorInterface::QueryMethods supportedAsyncQueryMethods() const;

    /**
     * @brief A function taking a single KParts::SelectorInterface::Element as argument and without return value
     */
    typedef const std::function<void (const KParts::SelectorInterface::Element &)> SingleElementSelectorCallback;

    /**
     * @brief A function taking a `QList<KParts::SelectorInterface::Element>` as argument and without return value
     */
    typedef const std::function<void (const QList<KParts::SelectorInterface::Element> &)> MultipleElementSelectorCallback;

    /**
     * @brief As KParts::SelectorInterface::querySelector except that the element is not returned by passed to a callback function
     *
     * If no element satisfying the given query is found, including the case where the requested query method is not supported,
     * the callback must be called with an invalid element.
     * @see KParts::SelectorInterface::querySelector()
     * @note This function is _asynchronous_
     * @param query the query containing the string
     * @param method the method to use for the query
     * @param callback the function to call with the found element
     */
    virtual void querySelectorAsync(const QString &query, KParts::SelectorInterface::QueryMethod method, SingleElementSelectorCallback& callback) = 0;

    /**
     * @brief As KParts::SelectorInterface::querySelectorAll except that the list of elements is not returned by passed to a callback function
     *
     * If no element satisfying the given query is found, including the case where the requested query method is not supported,
     * the callback must be called with an empty list.
     * @see KParts::SelectorInterface::querySelector()
     * @note This function is _asynchronous_
     * @param query the query containing the string
     * @param method the method to use for the query
     * @param callback the function to call with the found elements
     */
    virtual void querySelectorAllAsync(const QString &query, KParts::SelectorInterface::QueryMethod method, MultipleElementSelectorCallback& callback) = 0;
};

Q_DECLARE_INTERFACE(AsyncSelectorInterface, "org.kde.libkonq.AsyncSelectorInterface")

#endif // ASYNCSELECTORINTERFACE_H
