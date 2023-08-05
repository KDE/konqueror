/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2018 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "webenginepartcookiejar_kio.h"
#include "settings/webenginesettings.h"
#include <webenginepart_debug.h>

#include <QWebEngineProfile>
#include <QStringList>
#include <QDBusReply>
#include <QDebug>
#include <QWidget>
#include <QDateTime>
#include <QTimeZone>
#include <QApplication>
#include <kio_version.h>

const QVariant WebEnginePartCookieJarKIO::s_findCookieFields = QVariant::fromValue(QList<int>{
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

WebEnginePartCookieJarKIO::CookieIdentifier::CookieIdentifier(const QNetworkCookie& cookie):
    name(cookie.name()), domain(cookie.domain()), path(cookie.path())
{
}

WebEnginePartCookieJarKIO::CookieIdentifier::CookieIdentifier(const QString& n, const QString& d, const QString& p):
    name(n), domain(d), path(p)
{
}

WebEnginePartCookieJarKIO::WebEnginePartCookieJarKIO(QWebEngineProfile *prof, QObject *parent):
    QObject(parent), m_cookieStore(prof->cookieStore()),
    m_cookieServer("org.kde.kcookiejar5", "/modules/kcookiejar", "org.kde.KCookieServer")
{
    prof->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);
    connect(qApp, &QApplication::lastWindowClosed, this, &WebEnginePartCookieJarKIO::deleteSessionCookies);
    connect(m_cookieStore, &QWebEngineCookieStore::cookieAdded, this, &WebEnginePartCookieJarKIO::addCookie);
    connect(m_cookieStore, &QWebEngineCookieStore::cookieRemoved, this, &WebEnginePartCookieJarKIO::removeCookie);
    if(!m_cookieServer.isValid()){
        qCDebug(WEBENGINEPART_LOG) << "Couldn't connect to KCookieServer";
    }
    
    loadKIOCookies();
    
    auto filter = [this](const QWebEngineCookieStore::FilterRequest &req){return filterCookie(req);};
    m_cookieStore->setCookieFilter(filter);
}

WebEnginePartCookieJarKIO::~WebEnginePartCookieJarKIO()
{
}

bool WebEnginePartCookieJarKIO::filterCookie(const QWebEngineCookieStore::FilterRequest& req)
{
    return WebEngineSettings::self()->acceptCrossDomainCookies() || !req.thirdParty;
}

void WebEnginePartCookieJarKIO::deleteSessionCookies()
{
    if (!m_cookieServer.isValid()) {
        return;
    }
    foreach(qlonglong id, m_windowsWithSessionCookies) {
        m_cookieServer.call(QDBus::NoBlock, "deleteSessionCookies", id);
    }
}

QUrl WebEnginePartCookieJarKIO::constructUrlForCookie(const QNetworkCookie& cookie) const
{
    QUrl url;
    QString domain = cookie.domain().startsWith(".") ? cookie.domain().mid(1) : cookie.domain();
    if (!domain.isEmpty()) {
        url.setScheme("http");
        url.setHost(domain);
        url.setPath(cookie.path());
    } else {
        qCDebug(WEBENGINEPART_LOG) << "EMPTY COOKIE DOMAIN for" << cookie.name();
    }
    return url;
}

qlonglong WebEnginePartCookieJarKIO::findWinID()
{
    QWidget *mainWindow = qApp->activeWindow();
    if (mainWindow && !mainWindow->windowFlags().testFlag(Qt::Dialog)) {
        return mainWindow->winId();
    } else {
        QWidgetList windows = qApp->topLevelWidgets();
        foreach(QWidget *w, windows){
            if (!w->windowFlags().testFlag(Qt::Dialog)) {
                return w->winId();
            }
        }
    }
    return 0;
}

void WebEnginePartCookieJarKIO::removeCookieDomain(QNetworkCookie& cookie)
{
    if (!cookie.domain().startsWith('.')) {
        cookie.setDomain(QString());
    }
}

void WebEnginePartCookieJarKIO::addCookie(const QNetworkCookie& _cookie)
{   
    //If the added cookie is in m_cookiesLoadedFromKCookieServer, it means
    //we're loading the cookie from KCookieServer (from the call to loadKIOCookies
    //in the constructor (QWebEngineCookieStore::setCookie is asynchronous, though,
    //so we're not in the constructor anymore)), so don't attempt to add
    //the cookie back to KCookieServer; instead, remove it from the list.
    if (m_cookiesLoadedFromKCookieServer.removeOne(_cookie)) {
        return;
    }
    
#ifdef BUILD_TESTING
        m_testCookies.clear();
#endif
    
    QNetworkCookie cookie(_cookie);
    CookieIdentifier id(cookie);
    
    if (!m_cookieServer.isValid()) {
        return;
    }
    
    if (cookie.expirationDate().isValid()) {
    //There's a bug in KCookieJar which causes the expiration date to be interpreted as local time
    //instead of GMT as it should. The bug is fixed in KIO 5.50
    }
    QUrl url = constructUrlForCookie(cookie);
    if (url.isEmpty()) {
        return;
    }
    //NOTE: the removal of the domain (when not starting with a dot) must be done *after* creating
    //the URL, as constructUrlForCookie needs the domain
    removeCookieDomain(cookie);
    QByteArray header("Set-Cookie: ");
    header += cookie.toRawForm();
    header += "\n";
    qlonglong winId = findWinID();
    if (!cookie.expirationDate().isValid()) {
        m_windowsWithSessionCookies.insert(winId);
    }
//     qCDebug(WEBENGINEPART_LOG) << url;
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
            qCDebug(WEBENGINEPART_LOG) << m_cookieServer.lastError();
            return;
        }
        if (!advice.startsWith("Accept") && !cookieInKCookieJar(id, url)) {
            m_pendingRejectedCookies << id;
            m_cookieStore->deleteCookie(_cookie);
        } 
    }
}

QString WebEnginePartCookieJarKIO::askAdvice(const QUrl& url)
{
    if (!m_cookieServer.isValid()) {
        return QString();
    }
    QDBusReply<QString> rep = m_cookieServer.call(QDBus::Block, "getDomainAdvice", url.toString());
    if (rep.isValid()) {
        return rep.value();
    } else {
        qCDebug(WEBENGINEPART_LOG) << rep.error().message();
        return QString();
    }
}

bool WebEnginePartCookieJarKIO::cookieInKCookieJar(const WebEnginePartCookieJarKIO::CookieIdentifier& id, const QUrl& url)
{
    if (!m_cookieServer.isValid()) {
        return false;
    }
    QList<int> fields = { 
        static_cast<int>(CookieDetails::name),
        static_cast<int>(CookieDetails::domain),
        static_cast<int>(CookieDetails::path)
    };
    QDBusReply<QStringList> rep = m_cookieServer.call(QDBus::Block, "findCookies", QVariant::fromValue(fields), id.domain, url.toString(QUrl::FullyEncoded), id.path, id.name);
    if (!rep.isValid()) {
        qCDebug(WEBENGINEPART_LOG) << rep.error().message();
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

void WebEnginePartCookieJarKIO::removeCookie(const QNetworkCookie& _cookie)
{
    
    int pos = m_pendingRejectedCookies.indexOf(CookieIdentifier(_cookie));
    //Ignore pending cookies
    if (pos >= 0) {
        m_pendingRejectedCookies.takeAt(pos);
        return;
    }
    
    if (!m_cookieServer.isValid()) {
        return;
    }
    
    QNetworkCookie cookie(_cookie);
    QUrl url = constructUrlForCookie(cookie);
    if(url.isEmpty()){
        qCDebug(WEBENGINEPART_LOG) << "Can't remove cookie" << cookie.name() << "because its URL isn't known";
        return;
    }
    removeCookieDomain(cookie);
    
    QDBusPendingCall pcall = m_cookieServer.asyncCall("deleteCookie", cookie.domain(), url.host(), cookie.path(), QString(cookie.name()));
    QDBusPendingCallWatcher *w = new QDBusPendingCallWatcher(pcall, this);
    connect(w, &QDBusPendingCallWatcher::finished, this, &WebEnginePartCookieJarKIO::cookieRemovalFailed);
}

void WebEnginePartCookieJarKIO::cookieRemovalFailed(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<> r = *watcher;
    if (r.isError()){
        qCDebug(WEBENGINEPART_LOG) << "DBus error:" << r.error().message();
    }
    watcher->deleteLater();
}

void WebEnginePartCookieJarKIO::loadKIOCookies()
{
    const CookieUrlList cookies = findKIOCookies();
    for (const CookieWithUrl& cookieWithUrl : cookies){
        QNetworkCookie cookie = cookieWithUrl.cookie;
        QDateTime currentTime = QDateTime::currentDateTime();
        //Don't attempt to add expired cookies
        if (cookie.expirationDate().isValid() && cookie.expirationDate() < currentTime) {
            continue;
        }
        QNetworkCookie normalizedCookie(cookie);
        normalizedCookie.normalize(cookieWithUrl.url);
        m_cookiesLoadedFromKCookieServer << cookie;
#ifdef BUILD_TESTING
        m_testCookies << cookie;
#endif
        m_cookieStore->setCookie(cookie, cookieWithUrl.url);
    }
}

WebEnginePartCookieJarKIO::CookieUrlList WebEnginePartCookieJarKIO::findKIOCookies()
{
    CookieUrlList res;
    if (!m_cookieServer.isValid()) {
        return res;
    }
    QDBusReply<QStringList> rep = m_cookieServer.call(QDBus::Block, "findDomains");
    if(!rep.isValid()){
        qCDebug(WEBENGINEPART_LOG) << rep.error().message();
        return res;
    }
    QStringList domains = rep.value();
    uint fieldsCount = 8;
    foreach( const QString &d, domains){
    QDBusReply<QStringList> rep = m_cookieServer.call(QDBus::Block, "findCookies", s_findCookieFields, d, "", "", "");
        if (!rep.isValid()) {
            qCDebug(WEBENGINEPART_LOG) << rep.error().message();
            return res;
        }
        QStringList data = rep.value();
        for(int i = 0; i < data.count(); i+=fieldsCount){
            res << parseKIOCookie(data, i);
        }
    }
    return res;
}

//This function used to return a normalized cookie. However, doing so doesn't work correctly because QWebEngineCookieStore::setCookie
//in turns normalizes the cookie. One could think that calling normalize twice wouldn't be a problem, but that would be wrong. If the cookie
//domain is originally empty, after a call to normalize it will contain the cookie origin host. The second call to normalize will see a cookie
//whose domain is not empty and doesn't start with a dot and will add a dot to it.
WebEnginePartCookieJarKIO::CookieWithUrl WebEnginePartCookieJarKIO::parseKIOCookie(const QStringList& data, int start)
{
    QNetworkCookie c;
    auto extractField = [data, start](CookieDetails field){return data.at(start + static_cast<int>(field));};
    c.setDomain(extractField(CookieDetails::domain));
    c.setExpirationDate(QDateTime::fromSecsSinceEpoch(extractField(CookieDetails::expirationDate).toInt()));
    c.setName(extractField(CookieDetails::name).toUtf8());
    QString path = extractField(CookieDetails::path);
    c.setPath(path);
    c.setSecure(extractField(CookieDetails::secure).toInt()); //1 for true, 0 for false
    c.setValue(extractField(CookieDetails::value).toUtf8());

    QString host = extractField(CookieDetails::host);
    QUrl url;
    url.setScheme(c.isSecure() ? "https" : "http");
    url.setHost(host);
    url.setPath(path);
    return CookieWithUrl{c, url};
}

QDebug operator<<(QDebug deb, const WebEnginePartCookieJarKIO::CookieIdentifier& id)
{
    QDebugStateSaver saver(deb);
    deb << "(" << id.name << "," << id.domain << "," << id.path << ")";
    return deb;
}
