/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2018 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "webenginepartcookiejar.h"
#include "settings/webenginesettings.h"
#include <webenginepart_debug.h>
#include "cookiealertdlg.h"
#include "interfaces/browser.h"
#include "konqsettings.h"

#include <QWebEngineProfile>
#include <QStringList>
#include <QDBusReply>
#include <QDebug>
#include <QWidget>
#include <QDateTime>
#include <QTimeZone>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDataStream>
#include <QDir>
#include <QNetworkCookie>

#include <kio_version.h>
#include <KSharedConfig>
#include <KConfigGroup>

#include <QTimer>

WebEnginePartCookieJar::CookieIdentifier::CookieIdentifier(const QNetworkCookie& cookie):
    name(cookie.name()), domain(cookie.domain()), path(cookie.path())
{
}

WebEnginePartCookieJar::CookieIdentifier::CookieIdentifier(const QString& n, const QString& d, const QString& p):
    name(n), domain(d), path(p)
{
}

WebEnginePartCookieJar::WebEnginePartCookieJar(QWebEngineProfile *prof, QObject *parent):
    CookieJar(parent), m_cookieStore(prof->cookieStore())
{
    auto filter = [this](const QWebEngineCookieStore::FilterRequest &req){return filterCookie(req);};
    m_cookieStore->setCookieFilter(filter);

    connect(m_cookieStore, &QWebEngineCookieStore::cookieAdded, this, &WebEnginePartCookieJar::handleCookieAdditionToStore);
    connect(m_cookieStore, &QWebEngineCookieStore::cookieRemoved, this, &WebEnginePartCookieJar::removeCookieFromSet);
    connect(qApp, &QApplication::lastWindowClosed, this, &WebEnginePartCookieJar::saveCookieAdvice);
    KonqInterfaces::Browser *br = KonqInterfaces::Browser::browser(qApp);
    if (br) {
        connect(br, &KonqInterfaces::Browser::configurationChanged, this, &WebEnginePartCookieJar::applyConfiguration);
    }

    //WARNING: call this *before* applyConfiguration(), otherwise, if the default policy is Ask, the user will be asked again for all cookies
    //already in the jar
    readCookieAdvice();
    loadCookies();
    m_cookieStore->loadAllCookies();
    applyConfiguration();
}

WebEnginePartCookieJar::~WebEnginePartCookieJar()
{
    QFile f(cookieDataPath());
    if (!f.open(QFile::WriteOnly)) {
        return;
    }
    m_cookies.removeIf([](const QNetworkCookie &c){return !c.expirationDate().isValid();});
    QDataStream ds(&f);
    ds << m_cookies;
    f.close();
}

QSet<QNetworkCookie> WebEnginePartCookieJar::cookies() const
{
    return m_cookies;
}

void WebEnginePartCookieJar::writeConfig()
{
    Konq::Settings::self()->setCookieGlobalAdvice(m_policy.defaultPolicy);
    Konq::Settings::self()->setCookieDomainAdvice(m_policy.domainExceptions);
    Konq::Settings::self()->save();
}

void WebEnginePartCookieJar::applyConfiguration()
{
    m_policy.cookiesEnabled = Konq::Settings::cookiesEnabled();
    m_policy.rejectThirdPartyCookies = Konq::Settings::rejectCrossDomainCookies();
    m_policy.acceptSessionCookies = Konq::Settings::acceptSessionCookies();
    m_policy.defaultPolicy = Konq::Settings::self()->cookieGlobalAdvice();
    m_policy.domainExceptions = Konq::Settings::self()->cookieDomainAdvice();
    if (!m_policy.cookiesEnabled) {
        m_cookieStore->setCookieFilter([](const QWebEngineCookieStore::FilterRequest &){return false;});
        m_cookieStore->deleteAllCookies();
    }
}

void WebEnginePartCookieJar::removeAllCookies()
{
    m_cookieStore->deleteAllCookies();
    m_cookies.clear();
    QFile::remove(cookieAdvicePath());
}

void WebEnginePartCookieJar::removeCookiesWithDomain(const QString& domain)
{
    QStringList possibleDomains{domain};
    if (domain.startsWith('.')) {
        possibleDomains.append(domain.mid(1));
    } else {
        possibleDomains.append(QString('.') + domain);
    }

    bool exceptionsRemoved = false;
    QSet<QNetworkCookie> cookiesOrig(m_cookies);
    for (auto c : cookiesOrig) {
        if (possibleDomains.contains(c.domain())) {
            m_cookieStore->deleteCookie(c);
            if (m_policy.cookieExceptions.remove(CookieIdentifier{c}) ) {
                exceptionsRemoved = true;
            }
        }
    }
    if (exceptionsRemoved) {
        saveCookieAdvice();
    }
}

void WebEnginePartCookieJar::removeCookies(const QVector<QNetworkCookie>& cookies)
{
    bool exceptionsRemoved = false;
    for (const QNetworkCookie &c : cookies) {
        m_cookieStore->deleteCookie(c);
        if (m_policy.cookieExceptions.remove(CookieIdentifier{c}) > 0) {
            exceptionsRemoved = true;
        }
    }
    if (exceptionsRemoved) {
        saveCookieAdvice();
    }
}

void WebEnginePartCookieJar::removeCookie(const QNetworkCookie& cookie, const QUrl &origin)
{
    m_cookieStore->deleteCookie(cookie, origin);
    if (m_policy.cookieExceptions.remove(CookieIdentifier{cookie}) > 0) {
        saveCookieAdvice();
    }
}

bool WebEnginePartCookieJar::filterCookie(const QWebEngineCookieStore::FilterRequest& req)
{
    if (!m_policy.cookiesEnabled) {
        return false;
    }
    if (req.thirdParty && m_policy.rejectThirdPartyCookies) {
        return false;
    }
    return true;
}

void WebEnginePartCookieJar::removeSessionCookies()
{
    for (const QNetworkCookie &c : m_cookies) {
        if (!c.expirationDate().isValid()) {
            m_cookieStore->deleteCookie(c);
        }
    }
}

WebEnginePartCookieJar::CookieAdvice WebEnginePartCookieJar::decideCookieAction(const QNetworkCookie cookie)
{
    CookieAdvice option = CookieAdvice::Unknown;
    auto cookiesExIt = m_policy.cookieExceptions.constFind(CookieIdentifier{cookie.name(), cookie.domain(), cookie.path()});
    if (cookiesExIt != m_policy.cookieExceptions.constEnd()) {
        option = cookiesExIt.value();
    }

    if (option == CookieAdvice::Unknown && m_policy.acceptSessionCookies && !cookie.expirationDate().isValid()) {
        return CookieAdvice::Accept;
    }

    auto domainExIt = m_policy.domainExceptions.constFind(cookie.domain());
    if (domainExIt != m_policy.domainExceptions.constEnd()) {
        option = domainExIt.value();
    }

    if (option == CookieAdvice::Unknown) {
        option = m_policy.defaultPolicy != CookieAdvice::Unknown ? m_policy.defaultPolicy : CookieAdvice::Accept;
    }

    if (option == CookieAdvice::Ask) {
        option = askCookieQuestion(cookie);
    }

    return option;
}

WebEnginePartCookieJar::CookieAdvice WebEnginePartCookieJar::askCookieQuestion(const QNetworkCookie cookie)
{
    CookieAlertDlg dlg(cookie, qApp->activeWindow());
    dlg.exec();
    CookieAdvice option = dlg.choice();
    switch (dlg.applyTo()) {
        case CookieAlertDlg::This:
            m_policy.cookieExceptions.insert(CookieIdentifier(cookie), option);
            break;
        case CookieAlertDlg::Domain:
            m_policy.domainExceptions.insert(cookie.domain(), option);
            break;
        case CookieAlertDlg::Cookies:
            m_policy.defaultPolicy = option;
            break;
    }
    writeConfig();
    return option;
}

void WebEnginePartCookieJar::handleCookieAdditionToStore(const QNetworkCookie& cookie)
{   
    CookieAdvice action = decideCookieAction(cookie);
    if (action == CookieAdvice::Reject)  {
        m_cookieStore->deleteCookie(cookie);
        return;
    } else if (action == CookieAdvice::AcceptForSession && cookie.expirationDate().isValid()) {
        QNetworkCookie sessionCookie(cookie);
        sessionCookie.setExpirationDate(QDateTime());
        m_cookieStore->deleteCookie(cookie);
        m_cookieStore->setCookie(sessionCookie);
        return;
    }
    m_cookies.insert(cookie);
}

QString WebEnginePartCookieJar::cookieAdvicePath()
{
    QString s_cookieAdvicePath;
    if (s_cookieAdvicePath.isEmpty()) {
        QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        dir.mkpath(QStringLiteral("."));
        s_cookieAdvicePath = dir.filePath(QStringLiteral("cookieadvice"));
    }
    return s_cookieAdvicePath;
}

QString WebEnginePartCookieJar::cookieDataPath()
{
    QString s_cookieDataPath;
    if (s_cookieDataPath.isEmpty()) {
        QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        dir.mkpath(QStringLiteral("."));
        s_cookieDataPath = dir.filePath(QStringLiteral("cookies"));
    }
    return s_cookieDataPath;
}
void WebEnginePartCookieJar::saveCookieAdvice()
{
    QFile f(cookieAdvicePath());
    if (!f.open(QFile::WriteOnly)) {
        return;
    }
    QDataStream ds(&f);
    ds << m_policy.cookieExceptions;
}

void WebEnginePartCookieJar::readCookieAdvice()
{
    QFile f(cookieAdvicePath());
    if (!f.open(QFile::ReadOnly)) {
        return;
    }
    QDataStream ds(&f);
    ds >> m_policy.cookieExceptions;
}

void WebEnginePartCookieJar::removeCookieFromSet(const QNetworkCookie& cookie)
{
    m_cookies.remove(cookie);
}

void WebEnginePartCookieJar::loadCookies()
{
    QFile f(cookieDataPath());
    if (!f.open(QFile::ReadOnly)) {
        return;
    }
    QDataStream ds(&f);
    ds >> m_cookies;
    f.close();
}

QDebug operator<<(QDebug deb, const WebEnginePartCookieJar::CookieIdentifier& id)
{
    QDebugStateSaver saver(deb);
    deb << "(" << id.name << "," << id.domain << "," << id.path << ")";
    return deb;
}

size_t qHash(const QNetworkCookie& cookie, uint seed)
{
    return qHash(QStringList{cookie.name(), cookie.domain(), cookie.path()}, seed);
}

QDataStream& operator>>(QDataStream& ds, WebEnginePartCookieJar::CookieIdentifier& id)
{
    ds >> id.name >> id.domain >> id.path;
    return ds;
}

QDataStream& operator<<(QDataStream& ds, const WebEnginePartCookieJar::CookieIdentifier& id)
{
    ds << id.name << id.domain << id.path;
    return ds;
}

QDataStream& operator>>(QDataStream& ds, QNetworkCookie& cookie)
{
    QByteArray name, value;
    QString domain, path;
    QDateTime expirationDate;
    bool secure;
    ds >> name >> value >> domain >> path >> expirationDate >> secure;
    cookie.setName(name);
    cookie.setValue(value);
    cookie.setDomain(domain);
    cookie.setPath(path);
    cookie.setExpirationDate(expirationDate);
    cookie.setSecure(secure);
    return ds;
}

QDataStream& operator<<(QDataStream& ds, const QNetworkCookie& cookie)
{
    ds << cookie.name() << cookie.value() << cookie.domain() << cookie.path() << cookie.expirationDate() << cookie.isSecure();
    return ds;
}

