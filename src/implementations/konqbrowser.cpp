// /* This file is part of the KDE project
//     SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>
// 
//     SPDX-License-Identifier: LGPL-2.0-or-later
// */

#include "implementations/konqbrowser.h"
#include "interfaces/cookiejar.h"

using namespace KonqInterfaces;

KonqBrowser::KonqBrowser(QObject* parent) : Browser(parent), m_cookieJar(nullptr)
{
}

KonqBrowser::~KonqBrowser()
{
}

void KonqBrowser::setCookieJar(CookieJar* jar)
{
    m_cookieJar = jar;
}

CookieJar* KonqBrowser::cookieJar() const
{
    return m_cookieJar;
}
