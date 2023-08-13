/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KONQINTERFACES_BROWSER_H
#define KONQINTERFACES_BROWSER_H

#include "libkonq_export.h"

#include <QObject>

namespace KonqInterfaces {

/**
 * @brief Abstract class representing the Konqueror browser
 */
class LIBKONQ_EXPORT Browser : public QObject
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     *
     * @param parent the parent object
     */
    Browser(QObject* parent = nullptr);
    virtual ~Browser(); ///< Destructor

    /**
     * @brief Casts the given object or one of its children to a Browser
     *
     * This is similar to
     * @code
     * obj->findChild<Browser*>();
     * @endcode
     * except that if @p obj derives from Browser, it will be returned, regardless of whether any
     * of its children also derive from it.
     * @param obj the object to cast to a Browser
     * @return @p obj or one of its children as a Browser* or `nullptr` if neither @p obj nor its children derive from Browser
     */
    static Browser* browser(QObject* obj);

signals:
    void configurationChanged(); ///< Signal emitted after the configuration has changed
};

}

#endif // KONQINTERFACES_BROWSER_H
