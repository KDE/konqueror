/*
 * This file is part of the KDE project
 * Copyright (C) 2018 Stefano Crocco <stefano.crocco@alice.it>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "webenginepartcookiejar_test.h"
#include <webenginepartcookiejar.h>

#include <QTest>
#include <QNetworkCookie>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>
#include <QDBusInterface>
#include <QDBusReply>

QTEST_MAIN(TestWebEnginePartCookieJar);

void TestWebEnginePartCookieJar::initTestCase()
{
    m_cookieName = "webenginepartcookiejartest";
}

void TestWebEnginePartCookieJar::init()
{
    m_server = new QDBusInterface("org.kde.kcookiejar5", "/modules/kcookiejar", "org.kde.KCookieServer");
    m_profile = new QWebEngineProfile(this);
    m_store = m_profile->cookieStore();
    m_jar = new WebEnginePartCookieJar(m_profile, this);
}

void TestWebEnginePartCookieJar::cleanup()
{
    if (m_server->isValid()) {
        deleteCookies(findTestCookies());
    }
    delete m_server;
    m_server = Q_NULLPTR;
    delete m_jar;
    m_jar = Q_NULLPTR;
    m_store = Q_NULLPTR;
    delete m_profile;
    m_profile = Q_NULLPTR;
}

QNetworkCookie TestWebEnginePartCookieJar::CookieData::cookie() const
{
    QNetworkCookie cookie;
    cookie.setName(name.toUtf8());
    cookie.setValue(value.toUtf8());
    cookie.setPath(path);
    cookie.setDomain(domain);
    cookie.setSecure(secure);
    cookie.setExpirationDate(expiration);
    return cookie;
}

void TestWebEnginePartCookieJar::testCookieAddedToStoreAreAddedToKCookieServer_data()
{
    QTest::addColumn<QNetworkCookie>("cookie");
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("value");
    QTest::addColumn<QString>("domain");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("host");
    QTest::addColumn<QDateTime>("expiration");
    QTest::addColumn<bool>("secure");
    
    const QStringList labels{
        "persistent cookie with domain and path",
        "session cookie with domain and path",
        "persistent cookie with domain and no path",
        "persistent cookie with path and no domain",
        "persistent cookie without secure",
    };
    
    const QList<CookieData> input{
        {m_cookieName + "-persistent", "test-value", ".yyy.xxx.com", "/abc/def/", "zzz.yyy.xxx.com", QDateTime::currentDateTime().addYears(1), true},
        {m_cookieName + "-session", "test-value", ".yyy.xxx.com", "/abc/def/", "zzz.yyy.xxx.com", QDateTime(), true},
        {m_cookieName + "-no-path", "test-value", ".yyy.xxx.com", "", "zzz.yyy.xxx.com", QDateTime::currentDateTime().addYears(1), true},
        {m_cookieName + "-no-domain", "test-value", "", "/abc/def/", "zzz.yyy.xxx.com", QDateTime::currentDateTime().addYears(1), true},
        {m_cookieName + "-no-secure", "test-value", ".yyy.xxx.com", "/abc/def/", "zzz.yyy.xxx.com", QDateTime::currentDateTime().addYears(1), false}
    };
    
    QList<CookieData> expected(input);
    
    for (int i = 0; i < input.count(); ++i) {
        const CookieData &ex = expected.at(i);
        const CookieData &in = input.at(i);
        QNetworkCookie c = in.cookie();
        if (in.domain.isEmpty()) {
            c.normalize(QUrl("https://" + in.host));
        }
        QTest::newRow(labels.at(i).toLatin1()) << c << ex.name << ex.value << ex.domain << ex.path << ex.host << ex.expiration << ex.secure;
    }
}

void TestWebEnginePartCookieJar::testCookieAddedToStoreAreAddedToKCookieServer()
{
    QFETCH(const QNetworkCookie, cookie);
    QFETCH(const QString, name);
    QFETCH(const QString, value);
    QFETCH(const QString, domain);
    QFETCH(const QString, path);
    QFETCH(const QString, host);
    QFETCH(const QDateTime, expiration);
    QFETCH(const bool, secure);
    
    QVERIFY2(m_server->isValid(), qPrintable(m_server->lastError().message()));
    
//     domain=0, path=1, name=2, host=3, value=4, expirationDate=5, protocolVersion=6, secure=7;
    const QList<int> fields{0,1,2,3,4,5,6,7};
    
    emit m_store->cookieAdded(cookie);
    const QDBusReply<QStringList> res = m_server->call(QDBus::Block, "findCookies", QVariant::fromValue(fields), domain, host, cookie.path(), QString(cookie.name()));
    QVERIFY2(!m_server->lastError().isValid(), m_server->lastError().message().toLatin1());
    QStringList resFields = res.value();
    
    QEXPECT_FAIL("persistent cookie with path and no domain", "Handling of cookies without domain is currently broken", Abort);
    QVERIFY(!resFields.isEmpty());
    QCOMPARE(fields.count(), resFields.count());
    
    QCOMPARE(resFields.at(0), domain);
    QCOMPARE(resFields.at(1), path);
    QCOMPARE(resFields.at(2), name);
    QEXPECT_FAIL("", "The value returned by KCookieServer strips the leftmost part of the fqdn. Why?", Continue);
    QCOMPARE(resFields.at(3), host);
    QCOMPARE(resFields.at(4), value);
    const int secsSinceEpoch = resFields.at(5).toInt();
    //KCookieServer gives a session cookie an expiration time equal to epoch, while QNetworkCookie uses an invalid QDateTime
    if (!expiration.isValid()) {
        QCOMPARE(secsSinceEpoch, 0);
    } else {
        QCOMPARE(secsSinceEpoch, expiration.toSecsSinceEpoch());
    }
    const bool sec = resFields.at(7).toInt();
    QCOMPARE(sec, secure);
}

QList<TestWebEnginePartCookieJar::CookieData> TestWebEnginePartCookieJar::findTestCookies()
{
    QList<CookieData> cookies;
    if (!m_server->isValid()) {
        return cookies;
    }
    QDBusReply<QStringList> rep = m_server->call(QDBus::Block, "findDomains");
    if (!rep.isValid()) {
        qDebug() << rep.error().message();
        return cookies;
    }
    QStringList domains = rep.value();
    //domain, path, name, host
    const QList<int> fields{0,1,2,3};
    foreach (const QString &d, domains){
    rep = m_server->call(QDBus::Block, "findCookies", QVariant::fromValue(fields), d, "", "", "");
        if (!rep.isValid()) {
            qDebug() << rep.error().message();
            return cookies;
        }
        QStringList res = rep.value();
        for (int i = 0; i < res.count(); i+= fields.count()) {
            if (res.at(i+2).startsWith(m_cookieName)) {
                CookieData d;
                d.name = res.at(i+2);
                d.domain = res.at(i);
                d.path = res.at(i+1);
                d.host = res.at(i+3);
                cookies.append(d);
            }
        }
    }
    return cookies;
}

void TestWebEnginePartCookieJar::deleteCookies(const QList<TestWebEnginePartCookieJar::CookieData> &cookies)
{
    if (!m_server->isValid()) {
        return;
    }
    for (const CookieData &c: cookies) {
        QDBusMessage deleteRep = m_server->call(QDBus::Block, "deleteCookie", c.domain, c.host, c.path, c.name);
        if (m_server->lastError().isValid()) {
            qDebug() << m_server->lastError().message();
        }
    }
}
