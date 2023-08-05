// /* This file is part of the KDE project
//     SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>
// 
//     SPDX-License-Identifier: LGPL-2.0-or-later
// */

#include "cookiejar.h"

#include <KConfigGroup>

#include <QNetworkCookie>

using namespace KonqInterfaces;

CookieJar::CookieJar(QObject* parent) : QObject(parent)
{
}

CookieJar::~CookieJar()
{
}

CookieJar::CookieAdvice CookieJar::intToAdvice(int num, CookieAdvice defaultVal)
{
    if (num < 0 || num > static_cast<int>(CookieAdvice::Ask)) { //Ask is the last value
        return defaultVal;
    }
    return static_cast<CookieAdvice>(num);
}

CookieJar::CookieAdvice CookieJar::readAdviceConfigEntry(const KConfigGroup& grp, const char* key, CookieAdvice defaultVal)
{
    int val = grp.readEntry(key, -1);
    return intToAdvice(val, defaultVal);
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

