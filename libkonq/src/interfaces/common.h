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
     * @note the cast is performed using `qobject_cast()` if `T` is a subclass of `QObject` and
     * using `dynamic_cast()` otherwise.
     */
    template <typename T> T* as(QObject *obj);
}

template<typename T> T * KonqInterfaces::as(QObject* obj)
{
    //We have two different implementations depending on whether T is a QObject or not
    //to allow the use of findChild when possible
    if (std::is_base_of<QObject*,T*>()) {
        T* res = qobject_cast<T*>(obj);
        if (res) {
            return res;
        }
        return obj->findChild<T*>();
    } else {
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
}

#endif //KONQINTERFACES_COMMON
