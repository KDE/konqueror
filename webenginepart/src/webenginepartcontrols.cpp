/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2021 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "webenginepartcontrols.h"
#include "webengineparterrorschemehandler.h"
#include "webenginepartcookiejar.h"
#include "webengineurlrequestinterceptor.h"
#include "webenginepartkiohandler.h"
#include "about/konq_aboutpage.h"
#include "webenginepartcookiejar.h"
#include "spellcheckermanager.h"
#include "webenginepartdownloadmanager.h"
#include "certificateerrordialogmanager.h"
#include "webenginewallet.h"
#include "webenginepart.h"
#include "webenginepage.h"
#include "navigationrecorder.h"

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

WebEnginePartControls::WebEnginePartControls(): QObject(),
    m_profile(nullptr), m_cookieJar(nullptr), m_spellCheckerManager(nullptr), m_downloadManager(nullptr),
    m_certificateErrorDialogManager(new KonqWebEnginePart::CertificateErrorDialogManager(this)),
    m_navigationRecorder(new NavigationRecorder(this))
{
    QVector<QByteArray> localSchemes = {"error", "konq", "tar"};
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

    connect(QApplication::instance(), SIGNAL(configurationChanged()), this, SLOT(reparseConfiguration()));
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

void WebEnginePartControls::setup(QWebEngineProfile* profile)
{
    if (!profile || isReady()) {
        return;
    }
    m_profile = profile;

    m_profile->scripts()->insert({WebEngineWallet::formDetectorFunctionsScript(), WebEnginePart::detectRefreshScript()});

    m_profile->installUrlSchemeHandler("error", new WebEnginePartErrorSchemeHandler(m_profile));
    m_profile->installUrlSchemeHandler("konq", new KonqUrlSchemeHandler(m_profile));
    m_profile->installUrlSchemeHandler("help", new WebEnginePartKIOHandler(m_profile));
    m_profile->installUrlSchemeHandler("tar", new WebEnginePartKIOHandler(m_profile));

    m_profile->setUrlRequestInterceptor(new WebEngineUrlRequestInterceptor(this));

    m_cookieJar = new WebEnginePartCookieJar(profile, this);
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
    int max = std::min(languages.length(), 10);
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
    KConfigGroup grp = cfg->group("Cache");
    if (grp.readEntry("CacheEnabled", true)) {
        QWebEngineProfile::HttpCacheType type = grp.readEntry("MemoryCache", false) ? QWebEngineProfile::MemoryHttpCache : QWebEngineProfile::DiskHttpCache;
        m_profile->setHttpCacheType(type);
        m_profile->setHttpCacheMaximumSize(grp.readEntry("MaximumCacheSize", 0));
        //NOTE: According to the documentation, setCachePath resets the cache path to its default value if the argument is a null QString
        //it doesn't specify what it does if the string is empty but not null. Experimenting, it seems the behavior is the same
        m_profile->setCachePath(grp.readEntry("CustomCacheDir", QString()));

    } else {
        m_profile->setHttpCacheType(QWebEngineProfile::NoCache);
    }
}
