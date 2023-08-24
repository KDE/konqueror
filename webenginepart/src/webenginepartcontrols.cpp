/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2021 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "webenginepartcontrols.h"
#include "webengineparterrorschemehandler.h"
#include "webengineurlrequestinterceptor.h"
#include "webenginepartkiohandler.h"
#include "about/konq_aboutpage.h"
#include "spellcheckermanager.h"
#include "webenginepartdownloadmanager.h"
#include "certificateerrordialogmanager.h"
#include "webenginewallet.h"
#include "webenginepart.h"
#include "webenginepage.h"
#include "navigationrecorder.h"
#include <webenginepart_debug.h>
#include "profile.h"
#include "interfaces/browser.h"

#include <KProtocolInfo>
#include <KSharedConfig>
#include <KConfigGroup>

#include <QWebEngineProfile>
#include <QWebEngineUrlScheme>
#include <QWebEngineSettings>
#include <QWebEngineScriptCollection>
#include <QApplication>
#include <QLocale>
#include <QSettings>
#include <QJsonDocument>

using namespace KonqInterfaces;

WebEnginePartControls::WebEnginePartControls(): QObject(),
    m_profile(nullptr), m_cookieJar(nullptr), m_spellCheckerManager(nullptr), m_downloadManager(nullptr),
    m_certificateErrorDialogManager(new KonqWebEnginePart::CertificateErrorDialogManager(this)),
    m_navigationRecorder(new NavigationRecorder(this))
{
    QVector<QByteArray> localSchemes = {"error", "konq", "tar", "bookmarks"};
    const QStringList protocols = KProtocolInfo::protocols();
    for(const QString &prot : protocols){
        if (KProtocolInfo::defaultMimetype(prot) == "text/html") {
            localSchemes.append(QByteArray(prot.toLatin1()));
        }
    }
    for (const QByteArray &name : qAsConst(localSchemes)){
        QWebEngineUrlScheme scheme(name);
        scheme.setFlags(QWebEngineUrlScheme::LocalScheme|QWebEngineUrlScheme::LocalAccessAllowed);
        scheme.setSyntax(QWebEngineUrlScheme::Syntax::Path);
        QWebEngineUrlScheme::registerScheme(scheme);
    }

    Browser *browser = Browser::browser(qApp);
    if (browser) {
        connect(browser, &Browser::configurationChanged, this, &WebEnginePartControls::reparseConfiguration);
        connect(browser, &Browser::userAgentChanged, this, &WebEnginePartControls::setHttpUserAgent);
    }
}

WebEnginePartControls::~WebEnginePartControls()
{
}

WebEnginePartControls* WebEnginePartControls::self()
{
    static WebEnginePartControls s_self;
    return &s_self;
}

bool WebEnginePartControls::isReady() const
{
    return m_profile;
}

void WebEnginePartControls::registerScripts()
{
    if (!m_profile) {
        qCDebug(WEBENGINEPART_LOG) << "Attempting to register scripts before setting the profile";
        return;
    }

    QFile jsonFile(QStringLiteral(":/scripts.json"));
    jsonFile.open(QIODevice::ReadOnly);
    QJsonObject obj = QJsonDocument::fromJson(jsonFile.readAll()).object();
    Q_ASSERT(!obj.isEmpty());
    jsonFile.close();
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        QJsonObject scriptData = it.value().toObject();
        Q_ASSERT(!scriptData.isEmpty());
        QWebEngineScript script = scriptFromJson(it.key(), scriptData);
        if (!script.name().isEmpty()) {
            m_profile->scripts()->insert(script);
        }
    }
}

QWebEngineScript WebEnginePartControls::scriptFromJson(const QString& name, const QJsonObject& obj)
{
    QWebEngineScript script;
    QString file = obj.value(QLatin1String("file")).toString();
    if (file.isEmpty()) {
        return script;
    }
    QFile sourceFile(file);
    sourceFile.open(QIODevice::ReadOnly);
    Q_ASSERT(sourceFile.isOpen());
    script.setSourceCode(QString(sourceFile.readAll()));
    QJsonValue val = obj.value(QLatin1String("injectionPoint"));
    if (!val.isNull()) {
        int injectionPoint = val.toInt(-1);
        //NOTE:keep in sync with the values of QWebEngineScript::InjectionPoint
        Q_ASSERT(injectionPoint >=QWebEngineScript::Deferred && injectionPoint <= QWebEngineScript::DocumentCreation);
        script.setInjectionPoint(static_cast<QWebEngineScript::InjectionPoint>(injectionPoint));
    }
    val = obj.value(QLatin1String("worldId"));
    if (!val.isNull()) {
        int world= val.toInt(-1);
        //NOTE: keep in sync with the values of QWebEngineScript::ScriptWorldId
        Q_ASSERT(world >= QWebEngineScript::MainWorld);
        script.setWorldId(world);
    }
    val = obj.value(QStringLiteral("runsOnSubFrames"));
    if (!val.isBool()) {
        script.setRunsOnSubFrames(val.toBool());
    }
    script.setName(name);
    return script;
}

void WebEnginePartControls::setup(KonqWebEnginePart::Profile* profile)
{
    if (!profile || isReady()) {
        return;
    }
    m_profile = profile;

    registerScripts();

    m_profile->installUrlSchemeHandler("error", new WebEnginePartErrorSchemeHandler(m_profile));
    m_profile->installUrlSchemeHandler("konq", new KonqUrlSchemeHandler(m_profile));
    m_profile->installUrlSchemeHandler("help", new WebEnginePartKIOHandler(m_profile));
    m_profile->installUrlSchemeHandler("tar", new WebEnginePartKIOHandler(m_profile));
    m_profile->installUrlSchemeHandler("bookmarks", new WebEnginePartKIOHandler(m_profile));

    m_profile->setUrlRequestInterceptor(new WebEngineUrlRequestInterceptor(this));

    m_cookieJar = new WebEnginePartCookieJar(profile, this);
    Browser *browser = Browser::browser(qApp);
    if (browser) {
        m_profile->setHttpUserAgent(browser->userAgent());
#ifdef MANAGE_COOKIES_INTERNALLY
        browser->setCookieJar(m_cookieJar);
#endif
    }
    m_spellCheckerManager = new SpellCheckerManager(profile, this);
    m_downloadManager= new WebEnginePartDownloadManager(profile, this);
    m_profile->settings()->setAttribute(QWebEngineSettings::ScreenCaptureEnabled, true);
    QString langHeader = determineHttpAcceptLanguageHeader();
    if (!langHeader.isEmpty()) {
        m_profile->setHttpAcceptLanguage(langHeader);
    }

    reparseConfiguration();
}

WebEnginePartDownloadManager* WebEnginePartControls::downloadManager() const
{
    return m_downloadManager;
}

SpellCheckerManager* WebEnginePartControls::spellCheckerManager() const
{
    return m_spellCheckerManager;
}

bool WebEnginePartControls::handleCertificateError(const QWebEngineCertificateError& ce, WebEnginePage* page)
{
    return m_certificateErrorDialogManager->handleCertificateError(ce, page);
}

QString WebEnginePartControls::determineHttpAcceptLanguageHeader() const
{
    //According to comments in KSwitchLanguageDialog, the settings are stored in an INI format using QSettings,
    //rather than using KConfig
    QSettings appLangSettings(QStandardPaths::locate(QStandardPaths::GenericConfigLocation, "klanguageoverridesrc"), QSettings::IniFormat);
    appLangSettings.beginGroup(QStringLiteral("Language"));
    QString lang(appLangSettings.value(QApplication::instance()->applicationName()).toByteArray());
    if (lang.isEmpty()) {
        KSharedConfig::Ptr cfg;
        cfg = KSharedConfig::openConfig(QStringLiteral("plasma-localerc"));
        lang = cfg->group(QStringLiteral("Translations")).readEntry(QStringLiteral("LANGUAGE"));
        if (lang.isEmpty()) {
            lang = QLocale::system().name();
        }
        if (lang.isEmpty()) {
            return QString();
        }
    }
    QStringList languages = lang.split(':');
    QString header = languages.at(0);
    //In Qt6, QList::length() returns a qsizetype, which means you can't pass it to std::min.
    //Casting it to an int shouldn't be a problem, since the number of languages should be
    //small
    int max = std::min(static_cast<int>(languages.length()), 10);
    //The counter starts from 1 because the first entry has already been inserted above
    for (int i = 1; i < max; ++i) {
        header.append(QString(", %1;q=0.%2").arg(languages.at(i)).arg(10-i));
    }
    return header;
}

NavigationRecorder * WebEnginePartControls::navigationRecorder() const
{
    return m_navigationRecorder;
}

void WebEnginePartControls::reparseConfiguration()
{
    if (!m_profile) {
        return;
    }
    KSharedConfig::Ptr cfg = KSharedConfig::openConfig();
    KConfigGroup grp = cfg->group(QStringLiteral("Cache"));
    if (grp.readEntry(QStringLiteral("CacheEnabled"), true)) {
        QWebEngineProfile::HttpCacheType type = grp.readEntry(QStringLiteral("MemoryCache"), false) ? QWebEngineProfile::MemoryHttpCache : QWebEngineProfile::DiskHttpCache;
        m_profile->setHttpCacheType(type);
        m_profile->setHttpCacheMaximumSize(grp.readEntry(QStringLiteral("MaximumCacheSize"), 0));
        //NOTE: According to the documentation, setCachePath resets the cache path to its default value if the argument is a null QString
        //it doesn't specify what it does if the string is empty but not null. Experimenting, it seems the behavior is the same
        m_profile->setCachePath(grp.readEntry(QStringLiteral("CustomCacheDir"), QString()));

    } else {
        m_profile->setHttpCacheType(QWebEngineProfile::NoCache);
    }
}

QString WebEnginePartControls::httpUserAgent() const
{
    return m_profile ? m_profile->httpUserAgent() : QString();
}

void WebEnginePartControls::setHttpUserAgent(const QString& uaString)
{
    if (!m_profile || m_profile->httpUserAgent() == uaString) {
        return;
    }
    m_profile->setHttpUserAgent(uaString);
    emit userAgentChanged(uaString);
}

QString WebEnginePartControls::defaultHttpUserAgent() const
{
    return m_defaultUserAgent;
}
