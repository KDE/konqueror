// /* This file is part of the KDE project
//     SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>
// 
//     SPDX-License-Identifier: LGPL-2.0-or-later
// */

#ifndef KONQBROWSER_H
#define KONQBROWSER_H

#include "interfaces/browser.h"

#include <QPointer>

namespace KonqInterfaces
{
    class CookieJar;
}

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

    KonqInterfaces::CookieJar* cookieJar() const override; ///< Implementation of KonqInterfaces::Browser::cookieJar()

    void setCookieJar(KonqInterfaces::CookieJar* jar) override; ///< Implementation of KonqInterfaces::Browser::setCookieJar()

private:

    QPointer<KonqInterfaces::CookieJar> m_cookieJar; ///< Holds the cookie jar
};

#endif // KONQBROWSER_H
