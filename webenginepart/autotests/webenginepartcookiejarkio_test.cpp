/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2018 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "webenginepartcookiejarkio_test.h"
#include <cookies/webenginepartcookiejar_kio.h>

#include <QTest>

#include <QNetworkCookie>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>
#include <QDBusInterface>
#include <QDBusReply>

#include <algorithm>
    
//Cookie expiration dates returned by KCookieServer always have msecs set to 0
static QDateTime currentDateTime(){return QDateTime::fromSecsSinceEpoch(QDateTime::currentMSecsSinceEpoch()/1000);}

namespace QTest {
    template <>
    char *toString(const QNetworkCookie &cookie){
        QByteArray ba = "QNetworkCookie{";
        ba += "\nname: " + cookie.name();
        ba += "\ndomain: " + (cookie.domain().isEmpty() ? "<EMPTY>" : cookie.domain().toUtf8());
        ba += "\npath: " + (cookie.path().isEmpty() ? "<EMPTY>" : cookie.path().toUtf8());
        ba += "\nvalue: " + cookie.value();
        ba += "\nexpiration: " + (cookie.expirationDate().isValid() ? QByteArray::number(cookie.expirationDate().toMSecsSinceEpoch()) : "<INVALID>");
        ba += "\nsecure: " + QByteArray::number(cookie.isSecure());
        ba += "\nhttp: only" + QByteArray::number(cookie.isHttpOnly());
        return qstrdup(ba.data());
    }
}

QTEST_MAIN(TestWebEnginePartCookieJarKIO);

void TestWebEnginePartCookieJarKIO::initTestCase()
{
    m_cookieName = "webenginepartcookiejartest";
}

void TestWebEnginePartCookieJarKIO::init()
{
    m_server = new QDBusInterface("org.kde.kcookiejar5", "/modules/kcookiejar", "org.kde.KCookieServer");
    m_profile = new QWebEngineProfile(this);
    m_store = m_profile->cookieStore();
    m_jar = new WebEnginePartCookieJarKIO(m_profile, this);
}

void TestWebEnginePartCookieJarKIO::cleanup()
{
    if (m_server->isValid()) {
        deleteCookies(findTestCookies());
    }
    delete m_server;
    m_server = nullptr;
    delete m_jar;
    m_jar = nullptr;
    m_store = nullptr;
    delete m_profile;
    m_profile = nullptr;
}

QNetworkCookie TestWebEnginePartCookieJarKIO::CookieData::cookie() const
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

void TestWebEnginePartCookieJarKIO::testCookieAddedToStoreAreAddedToKCookieServer_data()
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
        "persistent cookie with domain and / as path",
        "persistent cookie with path and no domain",
        "persistent cookie without secure",
    };
    
    const QList<CookieData> input{
        {m_cookieName + "-persistent", "test-value", ".yyy.xxx.com", "/abc/def/", "zzz.yyy.xxx.com", QDateTime::currentDateTime().addYears(1), true},
        {m_cookieName + "-session", "test-value", ".yyy.xxx.com", "/abc/def/", "zzz.yyy.xxx.com", QDateTime(), true},
        {m_cookieName + "-no-path", "test-value", ".yyy.xxx.com", "", "zzz.yyy.xxx.com", QDateTime::currentDateTime().addYears(1), true},
        {m_cookieName + "-slash-as-path", "test-value", ".yyy.xxx.com", "", "zzz.yyy.xxx.com", QDateTime::currentDateTime().addYears(1), true},
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

void TestWebEnginePartCookieJarKIO::testCookieAddedToStoreAreAddedToKCookieServer()
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
    
    QVERIFY(!resFields.isEmpty());
    QCOMPARE(fields.count(), resFields.count());
    
    QCOMPARE(resFields.at(0), domain);
    QCOMPARE(resFields.at(1), path);
    QCOMPARE(resFields.at(2), name);
    if (!domain.isEmpty()){
        QEXPECT_FAIL("", "The value returned by KCookieServer strips the leftmost part of the fqdn. Why?", Continue);
    }
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

QList<TestWebEnginePartCookieJarKIO::CookieData> TestWebEnginePartCookieJarKIO::findTestCookies()
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

void TestWebEnginePartCookieJarKIO::deleteCookies(const QList<TestWebEnginePartCookieJarKIO::CookieData> &cookies)
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

void TestWebEnginePartCookieJarKIO::testCookieRemovedFromStoreAreRemovedFromKCookieServer_data()
{
    QTest::addColumn<QNetworkCookie>("cookie");
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("domain");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("host");
    
    const QStringList labels{
        "remove persistent cookie with domain and path",
        "remove session cookie with domain and path",
        "remove persistent cookie with domain and no path",
        "remove persistent cookie with domain and / as path",
        "remove persistent cookie with path and no domain",
        "remove persistent cookie without secure"
    };
    
    const QList<CookieData> input{
        {m_cookieName + "-persistent-remove", "test-remove-value", ".yyy.xxx.com", "/abc/def/", "zzz.yyy.xxx.com", QDateTime::currentDateTime().addYears(1), true},
        {m_cookieName + "-session-remove", "test-remove-value", ".yyy.xxx.com", "/abc/def/", "zzz.yyy.xxx.com", QDateTime(), true},
        {m_cookieName + "-no-path-remove", "test-remove-value", ".yyy.xxx.com", "", "zzz.yyy.xxx.com", QDateTime::currentDateTime().addYears(1), true},
        {m_cookieName + "-slash-as-path-remove", "test-remove-value", ".yyy.xxx.com", "/", "zzz.yyy.xxx.com", QDateTime::currentDateTime().addYears(1), true},
        {m_cookieName + "-no-domain-remove", "test-remove-value", "", "/abc/def/", "zzz.yyy.xxx.com", QDateTime::currentDateTime().addYears(1), true},
        {m_cookieName + "-no-secure-remove", "test-remove-value", ".yyy.xxx.com", "/abc/def/", "zzz.yyy.xxx.com", QDateTime::currentDateTime().addYears(1), false},
    };
    
    QList<CookieData> expected(input);
    
    for (int i = 0; i < input.count(); ++i) {
        const CookieData &ex = expected.at(i);
        const CookieData &in = input.at(i);
        QNetworkCookie c = in.cookie();
        if (in.domain.isEmpty()) {
            c.normalize(QUrl("https://" + in.host));
        }
        QTest::newRow(labels.at(i).toLatin1()) << c << ex.name << ex.domain << ex.path << ex.host;
    }}

void TestWebEnginePartCookieJarKIO::testCookieRemovedFromStoreAreRemovedFromKCookieServer()
{
    QFETCH(const QNetworkCookie, cookie);
    QFETCH(const QString, name);
    QFETCH(const QString, domain);
    QFETCH(const QString, path);
    QFETCH(const QString, host);
    
    //Add cookie to KCookieServer
    QDBusError e = addCookieToKCookieServer(cookie, host);
    QVERIFY2(!e.isValid(), qPrintable(m_server->lastError().message()));
    
    auto findCookies=[this, &domain, &host, &path, &name](){
        QDBusReply<QStringList> reply = m_server->call(QDBus::Block, "findCookies", QVariant::fromValue(QList<int>{2}), domain, host, path, name);
        return reply;
    };

    //Ensure cookie has been added to KCookieServer
    QDBusReply<QStringList> reply = findCookies();
    QVERIFY2(reply.isValid(), qPrintable(reply.error().message()));
    QVERIFY2(reply.value().contains(name), "Cookie wasn't added to server");

    //Emit QWebEngineCookieStore::cookieRemoved signal and check that cookie has indeed been removed
    emit m_store->cookieRemoved(cookie);

    //Check that cookie is no longer in KCookieServer
    reply = findCookies();
    QVERIFY2(reply.isValid(), qPrintable(reply.error().message()));
    QVERIFY2(!reply.value().contains(name), "Cookie wasn't removed from server");
}

QDBusError TestWebEnginePartCookieJarKIO::addCookieToKCookieServer(const QNetworkCookie& _cookie, const QString& host)
{
    QNetworkCookie cookie(_cookie);
    QUrl url;
    url.setHost(host);
    url.setScheme(cookie.isSecure() ? "https" : "http");
    if (!cookie.domain().startsWith('.')) {
        cookie.setDomain(QString());
    }
    const QByteArray setCookie = "Set-Cookie: " + cookie.toRawForm();
    m_server->call(QDBus::Block, "addCookies", url.toString(), setCookie, static_cast<qlonglong>(0));
    return m_server->lastError();
}

void TestWebEnginePartCookieJarKIO::testPersistentCookiesAreAddedToStoreOnCreation()
{
    delete m_jar;
    QDateTime exp = QDateTime::currentDateTime().addYears(1);
    QString baseCookieName = m_cookieName + "-startup";
    QList<CookieData> data {
        {baseCookieName + "-persistent", "test-value", ".yyy.xxx.com", "/abc/def/", "zzz.yyy.xxx.com", currentDateTime().addYears(1), true},
        {baseCookieName + "-no-path", "test-value", ".yyy.xxx.com", "", "zzz.yyy.xxx.com", currentDateTime().addYears(1), true},
        {baseCookieName + "-no-domain", "test-value", "", "/abc/def/", "zzz.yyy.xxx.com", currentDateTime().addYears(1), true},
        {baseCookieName + "-no-secure", "test-value", ".yyy.xxx.com", "/abc/def/", "zzz.yyy.xxx.com", currentDateTime().addYears(1), false}
    };
    QList<QNetworkCookie> expected;
    for(const CookieData &d: data){
        QNetworkCookie c = d.cookie();
        QDBusError e = addCookieToKCookieServer(c, d.host);
        QVERIFY2(!e.isValid(), qPrintable(e.message()));
        expected << c;
    }
    m_jar = new WebEnginePartCookieJarKIO(m_profile, this);
    QList<QNetworkCookie> cookiesInsertedIntoJar;
    for(const QNetworkCookie &c: qAsConst(m_jar->m_testCookies)){
        if(QString(c.name()).startsWith(baseCookieName)) {
            cookiesInsertedIntoJar << c;
        }
    }
    
    //Ensure that cookies in the two lists are in the same order before comparing them
    //(the order in cookiesInsertedIntoJar depends on the order KCookieServer::findCookies
    //returns them)
    auto sortLambda = [](const QNetworkCookie &c1, const QNetworkCookie &c2){
        return c1.name() < c2.name();
    };
    std::sort(cookiesInsertedIntoJar.begin(), cookiesInsertedIntoJar.end(), sortLambda);
    std::sort(expected.begin(), expected.end(), sortLambda);
    
    QCOMPARE(cookiesInsertedIntoJar, expected);
}

void TestWebEnginePartCookieJarKIO::testSessionCookiesAreNotAddedToStoreOnCreation()
{
    delete m_jar;
    CookieData data{m_cookieName + "-startup-session", "test-value", ".yyy.xxx.com", "/abc/def", "zzz.yyy.xxx.com", QDateTime(), true};
    QDBusError e = addCookieToKCookieServer(data.cookie(), data.host);
    QVERIFY2(!e.isValid(), qPrintable(e.message()));
    m_jar = new WebEnginePartCookieJarKIO(m_profile, this);
    QList<QNetworkCookie> cookiesInsertedIntoJar;
    for(const QNetworkCookie &c: qAsConst(m_jar->m_testCookies)) {
        if (c.name() == data.name) {
            cookiesInsertedIntoJar << c;
        }
    }
    QVERIFY2(cookiesInsertedIntoJar.isEmpty(), "Session cookies inserted into cookie store");
}

