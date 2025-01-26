/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2001 Carsten Pfeiffer <pfeiffer@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "settingsplugin.h"
#include "interfaces/browser.h"
#include "interfaces/cookiejar.h"

#include <kwidgetsaddons_version.h>
#include <kconfig.h>
#include <KLocalizedString>
#include <kmessagebox.h>
#include <QMenu>
#include <kprotocolmanager.h>
#include <kpluginfactory.h>
#include <KPluginMetaData>
#include <ktoggleaction.h>
#include <kactioncollection.h>
#include <kselectaction.h>
#include <kactionmenu.h>

#include <kparts/part.h>
#include <kparts/readonlypart.h>
#include <KConfigGroup>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QNetworkProxy>

#include <htmlextension.h>
#include <htmlsettingsinterface.h>

K_PLUGIN_CLASS_WITH_JSON(SettingsPlugin, "khtmlsettingsplugin.json")

SettingsPlugin::SettingsPlugin(QObject *parent,
                               const KPluginMetaData& metaData,
                               const QVariantList &)
    : KonqParts::Plugin(parent), mConfig(nullptr)
{
    setMetaData(metaData);
    KActionMenu *menu = new KActionMenu(QIcon::fromTheme(QStringLiteral("configure")), i18n("HTML Settings"), actionCollection());
    actionCollection()->addAction(QStringLiteral("action menu"), menu);
    menu->setPopupMode(QToolButton::InstantPopup);

    KToggleAction *action = actionCollection()->add<KToggleAction>(QStringLiteral("javascript"));
    action->setText(i18n("Java&Script"));
    connect(action, SIGNAL(triggered(bool)), SLOT(toggleJavascript(bool)));
    menu->addAction(action);

    action = actionCollection()->add<KToggleAction>(QStringLiteral("java"));
    action->setText(i18n("&Java"));
    connect(action, SIGNAL(triggered(bool)), SLOT(toggleJava(bool)));
    menu->addAction(action);

    action = actionCollection()->add<KToggleAction>(QStringLiteral("cookies"));
    action->setText(i18n("&Cookies"));
    connect(action, SIGNAL(triggered(bool)), SLOT(toggleCookies(bool)));
    menu->addAction(action);

    action = actionCollection()->add<KToggleAction>(QStringLiteral("plugins"));
    action->setText(i18n("&Plugins"));
    connect(action, SIGNAL(triggered(bool)), SLOT(togglePlugins(bool)));
    menu->addAction(action);

    action = actionCollection()->add<KToggleAction>(QStringLiteral("imageloading"));
    action->setText(i18n("Autoload &Images"));
    connect(action, SIGNAL(triggered(bool)), SLOT(toggleImageLoading(bool)));
    menu->addAction(action);

    //menu->addAction( new KSeparatorAction(actionCollection()) );

    action = actionCollection()->add<KToggleAction>(QStringLiteral("useproxy"));
    action->setText(i18n("Enable Pro&xy"));
    connect(action, SIGNAL(triggered(bool)), SLOT(toggleProxy(bool)));
    menu->addAction(action);

    action = actionCollection()->add<KToggleAction>(QStringLiteral("usecache"));
    action->setText(i18n("Enable Cac&he"));
    connect(action, SIGNAL(triggered(bool)), SLOT(toggleCache(bool)));
    menu->addAction(action);

    KSelectAction *sAction = actionCollection()->add<KSelectAction>(QStringLiteral("cachepolicy"));
    sAction->setText(i18n("Cache Po&licy"));
    QStringList policies;
    policies += i18n("&Keep Cache in Sync");
    policies += i18n("&Use Cache if Possible");
    policies += i18n("&Offline Browsing Mode");
    sAction->setItems(policies);
    connect(sAction, &KSelectAction::indexTriggered, this, &SettingsPlugin::cachePolicyChanged);

    menu->addAction(sAction);

    connect(menu->menu(), SIGNAL(aboutToShow()), SLOT(showPopup()));
}

SettingsPlugin::~SettingsPlugin()
{
    delete mConfig;
}

KonqInterfaces::CookieJar* SettingsPlugin::cookieJar() const
{
    KonqInterfaces::Browser *browser = KonqInterfaces::Browser::browser(qApp);
    return browser ? browser->cookieJar() : nullptr;
}

static HtmlSettingsInterface *settingsInterfaceFor(QObject *obj)
{
    HtmlExtension *extension = HtmlExtension::childObject(obj);
    return qobject_cast< HtmlSettingsInterface *>(extension);
}

void SettingsPlugin::showPopup()
{
    if (!mConfig) {
        mConfig = new KConfig(QStringLiteral("settingspluginrc"), KConfig::NoGlobals);
    }

    KParts::ReadOnlyPart *part = qobject_cast<KParts::ReadOnlyPart *>(parent());

    KProtocolManager::reparseConfiguration();
    const bool cookies = cookiesEnabled(part->url().host());
    actionCollection()->action(QStringLiteral("cookies"))->setChecked(cookies);
    actionCollection()->action(QStringLiteral("useproxy"))->setChecked(QNetworkProxy::applicationProxy().type() != QNetworkProxy::NoProxy);

// TODO KF6: check whether there's a way to implement cache settings directly using QtWebEngine settings. Also, check whether
// these settings are actually applied to WebEnginePart
// #if QT_VERSION_MAJOR < 6
//     actionCollection()->action(QStringLiteral("usecache"))->setChecked(KProtocolManager::useCache());
// #endif

    HtmlSettingsInterface *settings = settingsInterfaceFor(part);
    if (settings) {
        actionCollection()->action(QStringLiteral("java"))->setChecked(settings->htmlSettingsProperty(HtmlSettingsInterface::JavaEnabled).toBool());
        actionCollection()->action(QStringLiteral("javascript"))->setChecked(settings->htmlSettingsProperty(HtmlSettingsInterface::JavascriptEnabled).toBool());
        actionCollection()->action(QStringLiteral("plugins"))->setChecked(settings->htmlSettingsProperty(HtmlSettingsInterface::PluginsEnabled).toBool());
        actionCollection()->action(QStringLiteral("imageloading"))->setChecked(settings->htmlSettingsProperty(HtmlSettingsInterface::AutoLoadImages).toBool());
    }
}

void SettingsPlugin::toggleJava(bool checked)
{
    HtmlSettingsInterface *settings = settingsInterfaceFor(parent());
    if (settings) {
        settings->setHtmlSettingsProperty(HtmlSettingsInterface::JavaEnabled, checked);
    }
}

void SettingsPlugin::toggleJavascript(bool checked)
{
    HtmlSettingsInterface *settings = settingsInterfaceFor(parent());
    if (settings) {
        settings->setHtmlSettingsProperty(HtmlSettingsInterface::JavascriptEnabled, checked);
    }
}

void SettingsPlugin::toggleCookies(bool checked)
{
    KParts::ReadOnlyPart *part = qobject_cast<KParts::ReadOnlyPart *>(parent());
    if (!part) {
        return;
    }

    using Advice = Konq::Settings::CookieAdvice;
    Advice advice = checked ? Advice::Accept : Advice::Reject;
    KonqInterfaces::Browser *browser = KonqInterfaces::Browser::browser(qApp);
    if (!browser) {
        return;
    }
    KonqInterfaces::CookieJar *jar = browser->cookieJar();
    if (jar) {
        jar->addDomainException(part->url().url(), advice);
    }
}

void SettingsPlugin::togglePlugins(bool checked)
{
    HtmlSettingsInterface *settings = settingsInterfaceFor(parent());
    if (settings) {
        settings->setHtmlSettingsProperty(HtmlSettingsInterface::PluginsEnabled, checked);
    }
}

void SettingsPlugin::toggleImageLoading(bool checked)
{
    HtmlSettingsInterface *settings = settingsInterfaceFor(parent());
    if (settings) {
        settings->setHtmlSettingsProperty(HtmlSettingsInterface::AutoLoadImages, checked);
    }
}

bool SettingsPlugin::cookiesEnabled(const QString &url)
{
    KonqInterfaces::CookieJar *jar = cookieJar();
    if (!jar) {
        return false;
    }
    return jar->adviceForDomain(url) == Konq::SettingsBase::CookieAdvice::Accept;
}

//
// sync with kcontrol/kio/ksaveioconfig.* !
//

void SettingsPlugin::toggleProxy(bool checked)
{
    KConfigGroup grp(mConfig, QString());
    int type;

    if (checked) {
        //According with kioextras/kcms/ksaveioconfig.h, 1 is ManualProxy
        type = grp.readEntry("SavedProxyType", 1);
    } else {
        grp.writeEntry("SavedProxyType",proxyType());
        //According with kioextras/kcms/ksaveioconfig.h, 1 is NoProxy
        type = 0;
    }

    KConfig _config(QStringLiteral("kioslaverc"), KConfig::NoGlobals);
    KConfigGroup config(&_config, "Proxy Settings");
    config.writeEntry("ProxyType", type);

    actionCollection()->action(QStringLiteral("useproxy"))->setChecked(checked);
    updateIOSlaves();
}

void SettingsPlugin::toggleCache(bool checked)
{
    KConfig config(QStringLiteral("kio_httprc"), KConfig::NoGlobals);
    KConfigGroup grp(&config, QString());
    grp.writeEntry("UseCache", checked);
    actionCollection()->action(QStringLiteral("usecache"))->setChecked(checked);

    updateIOSlaves();
}

void SettingsPlugin::cachePolicyChanged(int p)
{
    QString policy;

    switch (p) {
    case 0:
        policy = QStringLiteral("Verify");
        break;
    case 1:
        policy = QStringLiteral("Cache");
        break;
    case 2:
        policy = QStringLiteral("CacheOnly");
        break;
    };

    if (!policy.isEmpty()) {
        KConfig config(QStringLiteral("kio_httprc"), KConfig::NoGlobals);
        KConfigGroup grp(&config, QString());
        grp.writeEntry("cache", policy);

        updateIOSlaves();
    }
}

void SettingsPlugin::updateIOSlaves()
{
    QDBusMessage message = QDBusMessage::createSignal(QStringLiteral("/KIO/Scheduler"),
                           QStringLiteral("org.kde.KIO.Scheduler"),
                           QStringLiteral("reparseSlaveConfiguration"));
    message << QString();
    QDBusConnection::sessionBus().send(message);
}

int SettingsPlugin::proxyType()
{
    return KConfig(QStringLiteral("kioslaverc"), KConfig::NoGlobals).group("Proxy Settings").readEntry("ProxyType", 0);
}

#include "settingsplugin.moc"
