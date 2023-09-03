/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KONQINTERFACES_COMMON
#define KONQINTERFACES_COMMON

#include <QObject>

namespace KonqInterfaces {
    /**
     * @brief Casts either the given object or one of its children as a pointer to T if possible
     *
     * @param obj the object to cast
     * @return @p obj or one of its children cast to a `T*` or `nullptr` if neither @p obj nor
     * its children can be cast to a `T*`
     * @internal
     * This function has two implementations, depending on whether or not `T` derives from `QObject`.
     * This allows to make use `QObject::findChild` when possible.
     *
     * To distinguish the two cases, as() uses `std::conditional`, delegating the actual implementation
     * either to CallAsForExtension::call() or CallAsForInterface::call()
     * @endinternal
     */
    template <typename T> T* as(QObject *obj);

    /**
     * @brief Struct used to implement as() when the template is a `QObject`
     */
    struct CallAsForExtension {
        /**
        * @brief Casts either the given object or one of its children as a pointer to T if possible
        *
        * This function assumes that `T` is a `QObject`-derived class and uses `QObject::findChild`
        * to determine the return value, except if @p obj can itself be cast to `T*`
        * @return @p obj or one of its children cast to a `T*` or `nullptr` if neither @p obj nor
        * its children can be cast to a `T*`
        */
        template <typename T> T* call(QObject* obj);
    };
    /**
     * @brief Struct used to implement as() when the template is not a `QObject`
     */
    struct CallAsForInterface {
        /**
        * @brief Casts either the given object or one of its children as a pointer to T if possible
        *
        * Unlike CallAsForExtension::call(), this function doesn't make any assumption about the type
        * of `T`, so it always works. However, it uses `dynamic_cast` and custom code rather than
        * the more specialized `QObject::findChild`.
        * @return @p obj or one of its children cast to a `T*` or `nullptr` if neither @p obj nor
        * its children can be cast to a `T*`
        */
        template <typename T> T* call(QObject* obj);
    };
}

template<typename T> T* KonqInterfaces::as(QObject *obj)
{
    typename std::conditional<(std::is_base_of<QObject, T>()), CallAsForExtension, CallAsForInterface>::type func;
    return func.template call<T>(obj);
}

template<typename T> T* KonqInterfaces::CallAsForExtension::call(QObject *obj)
{
    T* res = qobject_cast<T*>(obj);
    return res ? res : obj->findChild<T*>();
}

template<typename T> T* KonqInterfaces::CallAsForInterface::call(QObject *obj)
{
    T* res = dynamic_cast<T*>(obj);
    if (res) {
        return res;
    }
    QObjectList children = obj->findChildren<QObject*>();
    for (QObject *c : children) {
        res = dynamic_cast<T*>(c);
        if (res) {
            return res;
        }
    }
    return nullptr;
}

#endif //KONQINTERFACES_COMMON
