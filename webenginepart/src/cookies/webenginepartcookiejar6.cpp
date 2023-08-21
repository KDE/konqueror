/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2018 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "webenginepartcookiejar6.h"
#include "settings/webenginesettings.h"
#include <webenginepart_debug.h>
#include "cookiealertdlg.h"
#include "interfaces/browser.h"

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

#include <kio_version.h>
#include <KSharedConfig>
#include <KConfigGroup>


WebEnginePartCookieJar6::CookieIdentifier::CookieIdentifier(const QNetworkCookie& cookie):
    name(cookie.name()), domain(cookie.domain()), path(cookie.path())
{
}

WebEnginePartCookieJar6::CookieIdentifier::CookieIdentifier(const QString& n, const QString& d, const QString& p):
    name(n), domain(d), path(p)
{
}

WebEnginePartCookieJar6::WebEnginePartCookieJar6(QWebEngineProfile *prof, QObject *parent):
    CookieJar(parent), m_cookieStore(prof->cookieStore())
{
    auto filter = [this](const QWebEngineCookieStore::FilterRequest &req){return filterCookie(req);};
    m_cookieStore->setCookieFilter(filter);

    connect(m_cookieStore, &QWebEngineCookieStore::cookieAdded, this, &WebEnginePartCookieJar6::handleCookieAdditionToStore);
    connect(m_cookieStore, &QWebEngineCookieStore::cookieRemoved, this, &WebEnginePartCookieJar6::removeCookieFromSet);
    connect(qApp, &QApplication::lastWindowClosed, this, &WebEnginePartCookieJar6::saveCookieAdvice);
    KonqInterfaces::Browser *br = KonqInterfaces::Browser::browser(qApp);
    if (br) {
        connect(br, &KonqInterfaces::Browser::configurationChanged, this, &WebEnginePartCookieJar6::applyConfiguration);
    }

    //WARNING: call this *before* applyConfiguration(), otherwise, if the default policy is Ask, the user will be asked again for all cookies
    //already in the jar
    readCookieAdvice();
    m_cookieStore->loadAllCookies();
    applyConfiguration();
}

WebEnginePartCookieJar6::~WebEnginePartCookieJar6()
{
}

QSet<QNetworkCookie> WebEnginePartCookieJar6::cookies() const
{
    return m_cookies;
}

void WebEnginePartCookieJar6::writeConfig()
{
    KSharedConfig::Ptr cfg = KSharedConfig::openConfig();
    KConfigGroup grp = cfg->group("Cookie Policy");
    writeAdviceConfigEntry(grp, "CookieGlobalAdvice", m_policy.defaultPolicy);
    QJsonObject obj;
    for (auto it = m_policy.domainExceptions.constBegin(); it != m_policy.domainExceptions.constEnd(); ++it) {
        obj.insert(it.key(), adviceToInt(it.value()));
    }
    grp.writeEntry("CookieDomainAdvice", QJsonDocument(obj).toJson());
    cfg->sync();
}

void WebEnginePartCookieJar6::applyConfiguration()
{
    KConfigGroup grp = KSharedConfig::openConfig()->group("Cookie Policy");
    m_policy.cookiesEnabled = grp.readEntry("Cookies", true);
    m_policy.rejectThirdPartyCookies = grp.readEntry("RejectCrossDomainCookies", true);
    m_policy.acceptSessionCookies = grp.readEntry("AcceptSessionCookies", true);
    m_policy.defaultPolicy = readAdviceConfigEntry(grp, "CookieGlobalAdvice", CookieAdvice::Accept);
    QJsonObject obj = QJsonDocument::fromJson(grp.readEntry("CookieDomainAdvice", QByteArray())).object();
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        m_policy.domainExceptions.insert(it.key(), intToAdvice(it.value().toInt(0), CookieAdvice::Unknown));
    }
    if (!m_policy.cookiesEnabled) {
        m_cookieStore->setCookieFilter([](const QWebEngineCookieStore::FilterRequest &){return false;});
        m_cookieStore->deleteAllCookies();
    }
}

void WebEnginePartCookieJar6::removeAllCookies()
{
    QSet<QNetworkCookie> cookiesOrig(m_cookies);
    for (auto c : cookiesOrig) {
        m_cookieStore->deleteCookie(c);
    }
    QFile::remove(cookieAdvicePath());
}

void WebEnginePartCookieJar6::removeCookiesWithDomain(const QString& domain)
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
            if (m_policy.cookieExceptions.remove(CookieIdentifier{c}) > 1) {
                exceptionsRemoved = true;
            }
        }
    }
    if (exceptionsRemoved) {
        saveCookieAdvice();
    }
}

void WebEnginePartCookieJar6::removeCookies(const QVector<QNetworkCookie>& cookies)
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

void WebEnginePartCookieJar6::removeCookie(const QNetworkCookie& cookie, const QUrl &origin)
{
    m_cookieStore->deleteCookie(cookie, origin);
    if (m_policy.cookieExceptions.remove(CookieIdentifier{cookie}) > 0) {
        saveCookieAdvice();
    }
}

bool WebEnginePartCookieJar6::filterCookie(const QWebEngineCookieStore::FilterRequest& req)
{
    if (!m_policy.cookiesEnabled) {
        return false;
    }
    if (req.thirdParty && m_policy.rejectThirdPartyCookies) {
        return false;
    }
    return true;
}

void WebEnginePartCookieJar6::removeSessionCookies()
{
    for (const QNetworkCookie &c : m_cookies) {
        if (!c.expirationDate().isValid()) {
            m_cookieStore->deleteCookie(c);
        }
    }
}

WebEnginePartCookieJar6::CookieAdvice WebEnginePartCookieJar6::decideCookieAction(const QNetworkCookie cookie)
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

WebEnginePartCookieJar6::CookieAdvice WebEnginePartCookieJar6::askCookieQuestion(const QNetworkCookie cookie)
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

void WebEnginePartCookieJar6::handleCookieAdditionToStore(const QNetworkCookie& cookie)
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

QString WebEnginePartCookieJar6::cookieAdvicePath()
{
    QString s_cookieAdvicePath;
    if (s_cookieAdvicePath.isEmpty()) {
        QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        dir.mkpath(QStringLiteral("."));
        s_cookieAdvicePath = dir.filePath(QStringLiteral("cookieadvice"));
    }
    return s_cookieAdvicePath;
}

void WebEnginePartCookieJar6::saveCookieAdvice()
{
    QFile f(cookieAdvicePath());
    if (!f.open(QFile::WriteOnly)) {
        return;
    }
    QDataStream ds(&f);
    ds << m_policy.cookieExceptions;
}

void WebEnginePartCookieJar6::readCookieAdvice()
{
    QFile f(cookieAdvicePath());
    if (!f.open(QFile::ReadOnly)) {
        return;
    }
    QDataStream ds(&f);
    ds >> m_policy.cookieExceptions;
}

void WebEnginePartCookieJar6::removeCookieFromSet(const QNetworkCookie& cookie)
{
    m_cookies.remove(cookie);
}

QDebug operator<<(QDebug deb, const WebEnginePartCookieJar6::CookieIdentifier& id)
{
    QDebugStateSaver saver(deb);
    deb << "(" << id.name << "," << id.domain << "," << id.path << ")";
    return deb;
}

qHashReturnType qHash(const QNetworkCookie& cookie, uint seed)
{
    return qHash(QStringList{cookie.name(), cookie.domain(), cookie.path()}, seed);
}

QDataStream& operator>>(QDataStream& ds, WebEnginePartCookieJar6::CookieIdentifier& id)
{
    ds >> id.name >> id.domain >> id.path;
    return ds;
}

QDataStream& operator<<(QDataStream& ds, const WebEnginePartCookieJar6::CookieIdentifier& id)
{
    ds << id.name << id.domain << id.path;
    return ds;
}
