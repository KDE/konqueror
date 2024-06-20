// /* This file is part of the KDE project
//     SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>
// 
//     SPDX-License-Identifier: LGPL-2.0-or-later
// */

#include "cookiejar.h"

#include <KConfigGroup>

using namespace KonqInterfaces;

CookieJar::CookieJar(QObject* parent) : QObject(parent)
{
}

CookieJar::~CookieJar()
{
}

void CookieJar::writeAdviceConfigEntry(KConfigGroup& grp, const char* key, CookieAdvice advice)
{
    grp.writeEntry(key, static_cast<int>(advice));
}

void CookieJar::removeCookies(const QVector<QNetworkCookie>& cookies)
{
    for (const QNetworkCookie &c : cookies) {
        removeCookie(c);
    }
}

