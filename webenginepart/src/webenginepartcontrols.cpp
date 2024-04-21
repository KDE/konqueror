/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2021 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "webenginepartcontrols.h"
#include "schemehandlers/errorschemehandler.h"
#include "webengineurlrequestinterceptor.h"
#include "schemehandlers/kiohandler.h"
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
#include "schemehandlers/execschemehandler.h"

#include <KProtocolInfo>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include <QWebEngineProfile>
#include <QWebEngineUrlScheme>
#include <QWebEngineSettings>
#include <QWebEngineScriptCollection>
#include <QApplication>
#include <QLocale>
#include <QSettings>
#include <QJsonDocument>
#include <QMessageBox>

using namespace KonqInterfaces;
using namespace WebEngine;

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

    QWebEngineUrlScheme execScheme(QByteArrayLiteral("exec"));
    execScheme.setFlags(QWebEngineUrlScheme::LocalScheme);
    execScheme.setSyntax(QWebEngineUrlScheme::Syntax::Path);
    QWebEngineUrlScheme::registerScheme(execScheme);

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

    m_profile->installUrlSchemeHandler("error", new ErrorSchemeHandler(m_profile));
    m_profile->installUrlSchemeHandler("konq", new KonqUrlSchemeHandler(m_profile));
    m_profile->installUrlSchemeHandler("help", new KIOHandler(m_profile));
    m_profile->installUrlSchemeHandler("tar", new KIOHandler(m_profile));
    m_profile->installUrlSchemeHandler("bookmarks", new KIOHandler(m_profile));
    m_profile->installUrlSchemeHandler("exec", new ExecSchemeHandler(m_profile));

    m_profile->setUrlRequestInterceptor(new WebEngineUrlRequestInterceptor(this));

    m_cookieJar = new WebEnginePartCookieJar(profile, this);
    Browser *browser = Browser::browser(qApp);
    if (browser) {
        m_profile->setHttpUserAgent(browser->userAgent());
        browser->setCookieJar(m_cookieJar);
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

    updateUserStyleSheetScript();
}

void WebEnginePartControls::updateUserStyleSheetScript()
{
    QList<QWebEngineScript> oldScripts = m_profile->scripts()->find(s_userStyleSheetScriptName);
    bool hadUserStyleSheet = !oldScripts.isEmpty();
    //Remove old style sheets. Note that oldScripts should either be empty or contain only one element
    //In theory, we could reuse the previous script, if it turns out to be the same as the new one, but
    //I think this way is faster (besides being easier)
    for (const QWebEngineScript &s : oldScripts) {
        m_profile->scripts()->remove(s);
    }
    QUrl userStyleSheetUrl(WebEngineSettings::self()->userStyleSheet());
    bool userStyleSheetEnabled = !userStyleSheetUrl.isEmpty();
    //If the user stylesheet is disable and it was already disabled, there's nothing to do; if there was a custom stylesheet in use,
    //we'll have to remove it, so we can't return
    if (!userStyleSheetEnabled && !hadUserStyleSheet) {
        return;
    }

    QString css;
    if (userStyleSheetEnabled) {
        //Read the contents of the custom style sheet
        QFile cssFile(userStyleSheetUrl.path());
        cssFile.open(QFile::ReadOnly);
        if (cssFile.isOpen()) {
            css = cssFile.readAll();
            cssFile.close();
        }
        else {
            auto msg = i18n("Couldn't open the file <tt>%1</tt> containing the user style sheet. The default style sheet will be used", userStyleSheetUrl.path());
            QMessageBox::warning(qApp->activeWindow(), QString(), msg);
            //Only return if no custom stylesheet was in use
            if (!hadUserStyleSheet) {
                return;
            }
            userStyleSheetEnabled = false;
        }
    }

    //Create the js code
    QFile applyUserCssFile(":/applyuserstylesheet.js");
    applyUserCssFile.open(QFile::ReadOnly);
    Q_ASSERT(applyUserCssFile.isOpen());
    QString code{QString(applyUserCssFile.readAll()).arg(s_userStyleSheetScriptName).arg(css.simplified())};
    applyUserCssFile.close();

    //Tell pages to update their stylesheet. If `css` is empty, the script will remove the <style> element from the pages;
    //if `css` is not empty, it will update or create the appropriate <style> element
    emit updateStyleSheet(code);
    if (!userStyleSheetEnabled) {
        return;
    }

    //Create a script to inject in new pages
    QWebEngineScript applyUserCss;
    applyUserCss.setName(s_userStyleSheetScriptName);
    applyUserCss.setInjectionPoint(QWebEngineScript::DocumentReady);
    applyUserCss.setWorldId(QWebEngineScript::ApplicationWorld);
    applyUserCss.setSourceCode(code);
    m_profile->scripts()->insert(applyUserCss);
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
