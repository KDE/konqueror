/*
 *    This file is part of the KDE project
 *    Copyright (C) 2018 Stefano Crocco <stefano.crocco@alice.it>
 *
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License as
 *    published by the Free Software Foundation; either version 2 of
 *    the License or (at your option) version 3 or any later version
 *    accepted by the membership of KDE e.V. (or its successor approved
 *    by the membership of KDE e.V.), which shall act as a proxy
 *    defined in Section 14 of version 3 of the license.
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Library General Public License for more details.
 *
 *    You should have received a copy of the GNU Library General Public License
 *    along with this library; see the file COPYING.LIB.  If not, write to
 *    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *    Boston, MA 02110-1301, USA.
 * 
*/

#include "webenginepartcookiejar.h"
#include "settings/webenginesettings.h"

#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <QStringList>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>
#include <QWidget>
#include <QDateTime>
#include <QTimeZone>
#include <QApplication>
#include <kio_version.h>

const QVariant WebEnginePartCookieJar::s_findCookieFields = QVariant::fromValue(QList<int>{
        static_cast<int>(CookieDetails::domain),
        static_cast<int>(CookieDetails::path),
        static_cast<int>(CookieDetails::name),
        static_cast<int>(CookieDetails::host),
        static_cast<int>(CookieDetails::value),
        static_cast<int>(CookieDetails::expirationDate),
        static_cast<int>(CookieDetails::protocolVersion),
        static_cast<int>(CookieDetails::secure)
    }
);

WebEnginePartCookieJar::CookieIdentifier::CookieIdentifier(const QNetworkCookie& cookie):
    name(cookie.name()), domain(cookie.domain()), path(cookie.path())
{
}

WebEnginePartCookieJar::CookieIdentifier::CookieIdentifier(const QString& n, const QString& d, const QString& p):
    name(n), domain(d), path(p)
{
}

WebEnginePartCookieJar::WebEnginePartCookieJar(QWebEngineProfile *prof, QObject *parent):
    QObject(parent), m_cookieStore(prof->cookieStore()),
    m_cookieServer("org.kde.kcookiejar5", "/modules/kcookiejar", "org.kde.KCookieServer")
{
    prof->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);
    connect(qApp, &QApplication::lastWindowClosed, this, &WebEnginePartCookieJar::deleteSessionCookies);
    connect(m_cookieStore, &QWebEngineCookieStore::cookieAdded, this, &WebEnginePartCookieJar::addCookie);
    connect(m_cookieStore, &QWebEngineCookieStore::cookieRemoved, this, &WebEnginePartCookieJar::removeCookie);
    if(!m_cookieServer.isValid()){
        qDebug() << "Couldn't connect to KCookieServer";
    }
    loadKIOCookies();
    
    //QWebEngineCookieStore::setCookieFilter only exists from Qt 5.11.0
#if QTWEBENGINE_VERSION >= QT_VERSION_CHECK(5,11,0)
    auto filter = [this](const QWebEngineCookieStore::FilterRequest &req){return filterCookie(req);};
    m_cookieStore->setCookieFilter(filter);
#endif //QTWEBENGINE_VERSION >= QT_VERSION_CHECK(5,11,0)
}

WebEnginePartCookieJar::~WebEnginePartCookieJar()
{
}

#if QTWEBENGINE_VERSION >= QT_VERSION_CHECK(5,11,0)
bool WebEnginePartCookieJar::filterCookie(const QWebEngineCookieStore::FilterRequest& req)
{
    return WebEngineSettings::self()->acceptCrossDomainCookies() || !req.thirdParty;
}
#endif //QTWEBENGINE_VERSION >= QT_VERSION_CHECK(5,11,0)

void WebEnginePartCookieJar::deleteSessionCookies()
{
    if (!m_cookieServer.isValid()) {
        return;
    }
    foreach(qlonglong id, m_windowsWithSessionCookies) {
        m_cookieServer.call(QDBus::NoBlock, "deleteSessionCookies", id);
    }
}

QUrl WebEnginePartCookieJar::constructUrlForCookie(const QNetworkCookie& cookie) const
{
    QUrl url;
    QString domain = cookie.domain().startsWith(".") ? cookie.domain().mid(1) : cookie.domain();
    if (!domain.isEmpty()) {
        url.setScheme("http");
        url.setHost(domain);
        url.setPath(cookie.path());
    } else {
        qDebug() << "EMPTY COOKIE DOMAIN for" << cookie.name();
    }
    return url;
}

qlonglong WebEnginePartCookieJar::findWinID()
{
    QWidget *mainWindow = qApp->activeWindow();
    if (mainWindow && !(mainWindow->windowFlags() & Qt::Dialog)) {
        return mainWindow->winId();
    } else {
        QWidgetList windows = qApp->topLevelWidgets();
        foreach(QWidget *w, windows){
            if (w->isWindow() && !(w->windowFlags() & Qt::Dialog)) {
                return w->winId();
            }
        }
    }
    return 0;
}

void WebEnginePartCookieJar::addCookie(const QNetworkCookie& _cookie)
{   
    //If the added cookie is in m_cookiesLoadedFromKCookieServer, it means
    //we're loading the cookie from KCookieServer (from the call to loadKIOCookies
    //in the constructor (QWebEngineCookieStore::setCookie is asynchronous, though,
    //so we're not in the constructor anymore)), so don't attempt to add
    //the cookie back to KCookieServer; instead, remove it from the list.
    if (m_cookiesLoadedFromKCookieServer.removeOne(_cookie)) {
        return;
    } 
    
    QNetworkCookie cookie(_cookie);
    CookieIdentifier id(cookie);
    
    if (!m_cookieServer.isValid()) {
        return;
    }
    
    if (cookie.expirationDate().isValid()) {
    //There's a bug in KCookieJar which causes the expiration date to be interpreted as local time
    //instead of GMT as it should. The bug is fixed in KIO 5.50
#if KIO_VERSION < QT_VERSION_CHECK(5,50,0)
        QTimeZone local = QTimeZone::systemTimeZone();
        int offset = local.offsetFromUtc(QDateTime::currentDateTime());
        QDateTime dt = cookie.expirationDate();
        dt.setTime(dt.time().addSecs(offset));
        cookie.setExpirationDate(dt);
#endif
    }
    QUrl url = constructUrlForCookie(cookie);
    if (url.isEmpty()) {
        return;
    }
    QByteArray header("Set-Cookie: ");
    header += cookie.toRawForm();
    header += "\n";
    qlonglong winId = findWinID();
    if (!cookie.expirationDate().isValid()) {
        m_windowsWithSessionCookies.insert(winId);
    }
//     qDebug() << url;
    QString advice = askAdvice(url);
    if (advice == "Reject"){
        m_pendingRejectedCookies << CookieIdentifier(_cookie);
        m_cookieStore->deleteCookie(_cookie);
    } else if (advice == "AcceptForSession" && !cookie.isSessionCookie()) {
        cookie.setExpirationDate(QDateTime());
        addCookie(cookie);
    } else {
        int oldTimeout = m_cookieServer.timeout();
        if (advice == "Ask") {
            //Give the user time (10 minutes = 600 000ms) to analyze the cookie
            m_cookieServer.setTimeout(10*60*1000);
        }
        m_cookieServer.call(QDBus::Block, "addCookies", url.toString(), header, winId);
        m_cookieServer.setTimeout(oldTimeout);
        if (m_cookieServer.lastError().isValid()) {
            qDebug() << m_cookieServer.lastError();
            return;
        }
        if (!advice.startsWith("Accept") && !cookieInKCookieJar(id, url)) {
            m_pendingRejectedCookies << id;
            m_cookieStore->deleteCookie(_cookie);
        } 
    }
}

QString WebEnginePartCookieJar::askAdvice(const QUrl& url)
{
    if (!m_cookieServer.isValid()) {
        return QString();
    }
    QDBusReply<QString> rep = m_cookieServer.call(QDBus::Block, "getDomainAdvice", url.toString());
    if (rep.isValid()) {
        return rep.value();
    } else {
        qDebug() << rep.error().message();
        return QString();
    }
}

bool WebEnginePartCookieJar::cookieInKCookieJar(const WebEnginePartCookieJar::CookieIdentifier& _id, const QUrl& url)
{
    if (!m_cookieServer.isValid()) {
        return false;
    }
    CookieIdentifier id(_id.name, prependDotToDomain(_id.domain), _id.path);
    QList<int> fields = { 
        static_cast<int>(CookieDetails::name),
        static_cast<int>(CookieDetails::domain),
        static_cast<int>(CookieDetails::path)
    };
    QDBusReply<QStringList> rep = m_cookieServer.call(QDBus::Block, "findCookies", QVariant::fromValue(fields), id.domain, url.toString(QUrl::FullyEncoded), id.path, id.name);
    if (!rep.isValid()) {
        qDebug() << rep.error().message();
        return false;
    }
    QStringList cookies = rep.value();
    for(int i = 0; i < cookies.length()-2; i+=3){
        if (CookieIdentifier(cookies.at(i), cookies.at(i+1), cookies.at(i+2)) == id) {
            return true;
        }
    }
    return false;
}

void WebEnginePartCookieJar::removeCookie(const QNetworkCookie& cookie)
{
    CookieIdentifier id(cookie);
    
    int pos = m_pendingRejectedCookies.indexOf(id);
    //Ignore pending cookies
    if (pos >= 0) {
        m_pendingRejectedCookies.takeAt(pos);
        return;
    }
    
    if (!m_cookieServer.isValid()) {
        return;
    }
    
    QUrl url = constructUrlForCookie(cookie);
    if(url.isEmpty()){
        qDebug() << "Can't remove cookie" << cookie.name() << "because its URL isn't known";
        return;
    }
    
    //Add leading dot to domain, if necessary
    id.domain = prependDotToDomain(id.domain);
    
    m_cookieServer.call(QDBus::NoBlock, "deleteCookie", cookie.domain(), url.host().toLower(), cookie.path(), cookie.name());
    if (m_cookieServer.lastError().isValid()) {
        qDebug() << m_cookieServer.lastError();
    }
}

void WebEnginePartCookieJar::loadKIOCookies()
{
    CookieList cookies = findKIOCookies();
    foreach(const QNetworkCookie& cookie, cookies){
        QDateTime currentTime = QDateTime::currentDateTime();
        //Don't attempt to add expired cookies
        if (cookie.expirationDate().isValid() && cookie.expirationDate() < currentTime) {
            continue;
        }
        m_cookiesLoadedFromKCookieServer << cookie;
        m_cookieStore->setCookie(cookie);
    }
}

WebEnginePartCookieJar::CookieList WebEnginePartCookieJar::findKIOCookies()
{
    CookieList res;
    if (!m_cookieServer.isValid()) {
        return res;
    }
    QDBusReply<QStringList> rep = m_cookieServer.call(QDBus::Block, "findDomains");
    if(!rep.isValid()){
        qDebug() << rep.error().message();
        return res;
    }
    QStringList domains = rep.value();
    uint fieldsCount = 8;
    foreach( const QString &d, domains){
    QDBusReply<QStringList> rep = m_cookieServer.call(QDBus::Block, "findCookies", s_findCookieFields, d, "", "", "");
        if (!rep.isValid()) {
            qDebug() << rep.error().message();
            return res;
        }
        QStringList data = rep.value();
        for(int i = 0; i < data.count(); i+=fieldsCount){
            res << parseKIOCookie(data, i);
        }
    }
    return res;
}

QNetworkCookie WebEnginePartCookieJar::parseKIOCookie(const QStringList& data, int start)
{
    QNetworkCookie c;
    auto extractField = [data, start](CookieDetails field){return data.at(start + static_cast<int>(field));};
    c.setDomain(data.at(start+static_cast<int>(CookieDetails::domain)).toUtf8());
    c.setExpirationDate(QDateTime::fromSecsSinceEpoch(extractField(CookieDetails::expirationDate).toInt()));
    c.setName(extractField(CookieDetails::name).toUtf8());
    c.setPath(extractField(CookieDetails::path).toUtf8());
    c.setSecure(extractField(CookieDetails::secure).toInt()); //1 for true, 0 for false
    c.setValue(extractField(CookieDetails::value).toUtf8());
    return c;
}

QDebug operator<<(QDebug deb, const WebEnginePartCookieJar::CookieIdentifier& id)
{
    QDebugStateSaver saver(deb);
    deb << "(" << id.name << "," << id.domain << "," << id.path << ")";
    return deb;
}
