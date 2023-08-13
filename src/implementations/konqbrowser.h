// /* This file is part of the KDE project
//     SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>
// 
//     SPDX-License-Identifier: LGPL-2.0-or-later
// */

#ifndef KONQBROWSER_H
#define KONQBROWSER_H

#include "interfaces/browser.h"

#include <QPointer>

/**
 * @brief Implementation of KonqInterfaces::Browser
 */
class KonqBrowser : public KonqInterfaces::Browser
{
    Q_OBJECT

public:
    /**
     * @brief Default constructor
     *
     * @param parent the parent object
     */
    KonqBrowser(QObject* parent = nullptr);

    ~KonqBrowser();  ///< Destructor
};

#endif // KONQBROWSER_H
