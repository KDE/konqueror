/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2001 Carsten Pfeiffer <pfeiffer@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "settingsplugin.h"

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
    const bool cookies = cookiesEnabled(part->url().url());
    actionCollection()->action(QStringLiteral("cookies"))->setChecked(cookies);
    actionCollection()->action(QStringLiteral("useproxy"))->setChecked(KProtocolManager::useProxy());
// TODO KF6: check whether there's a way to implement cache settings directly using QtWebEngine settings. Also, check whether
// these settings are actually applied to WebEnginePart
#if QT_VERSION_MAJOR < 6
    actionCollection()->action(QStringLiteral("usecache"))->setChecked(KProtocolManager::useCache());
#endif

    HtmlSettingsInterface *settings = settingsInterfaceFor(part);
    if (settings) {
        actionCollection()->action(QStringLiteral("java"))->setChecked(settings->htmlSettingsProperty(HtmlSettingsInterface::JavaEnabled).toBool());
        actionCollection()->action(QStringLiteral("javascript"))->setChecked(settings->htmlSettingsProperty(HtmlSettingsInterface::JavascriptEnabled).toBool());
        actionCollection()->action(QStringLiteral("plugins"))->setChecked(settings->htmlSettingsProperty(HtmlSettingsInterface::PluginsEnabled).toBool());
        actionCollection()->action(QStringLiteral("imageloading"))->setChecked(settings->htmlSettingsProperty(HtmlSettingsInterface::AutoLoadImages).toBool());
    }

#if QT_VERSION_MAJOR < 6
    KIO::CacheControl cc = KProtocolManager::cacheControl();
    switch (cc) {
    case KIO::CC_Verify:
        static_cast<KSelectAction *>(actionCollection()->action(QStringLiteral("cachepolicy")))->setCurrentItem(0);
        break;
    case KIO::CC_CacheOnly:
        static_cast<KSelectAction *>(actionCollection()->action(QStringLiteral("cachepolicy")))->setCurrentItem(2);
        break;
    case KIO::CC_Cache:
        static_cast<KSelectAction *>(actionCollection()->action(QStringLiteral("cachepolicy")))->setCurrentItem(1);
        break;
    case KIO::CC_Reload: // nothing for now
    case KIO::CC_Refresh:
    default:
        break;
    }
#endif
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
    if (part) {
        const QString advice((checked ? QStringLiteral("Accept") : QStringLiteral("Reject")));

        // TODO generate interface from the installed org.kde.KCookieServer.xml file
        // but not until 4.3 is released, since 4.2 had "void setDomainAdvice"
        // while 4.3 has "bool setDomainAdvice".

        QDBusInterface kded(QStringLiteral("org.kde.kded5"),
                            QStringLiteral("/modules/kcookiejar"),
                            QStringLiteral("org.kde.KCookieServer"));
        QDBusReply<void> reply = kded.call(QStringLiteral("setDomainAdvice"), part->url().url(), advice);

        if (!reply.isValid())
            KMessageBox::error(part->widget(),
                               i18n("The cookie setting could not be changed, because the "
                                    "cookie daemon could not be contacted."),
                               i18nc("@title:window", "Cookie Settings Unavailable"));
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
    QDBusInterface kded(QStringLiteral("org.kde.kded5"),
                        QStringLiteral("/modules/kcookiejar"),
                        QStringLiteral("org.kde.KCookieServer"));
    QDBusReply<QString> reply = kded.call(QStringLiteral("getDomainAdvice"), url);

    bool enabled = false;

    if (reply.isValid()) {
        QString advice = reply;
        enabled = (advice == QLatin1String("Accept"));
        if (!enabled && advice == QLatin1String("Dunno")) {
            // TODO, check the global setting via dcop
            KConfig _kc(QStringLiteral("kcookiejarrc"), KConfig::NoGlobals);
            KConfigGroup kc(&_kc, "Cookie Policy");
            enabled = (kc.readEntry("CookieGlobalAdvice", "Reject") == QLatin1String("Accept"));
        }
    }

    return enabled;
}

//
// sync with kcontrol/kio/ksaveioconfig.* !
//

void SettingsPlugin::toggleProxy(bool checked)
{
    KConfigGroup grp(mConfig, QString());
    int type;

    if (checked) {
        type = grp.readEntry("SavedProxyType", static_cast<int>(KProtocolManager::ManualProxy));
    } else {
        grp.writeEntry("SavedProxyType", static_cast<int>(KProtocolManager::proxyType()));
        type = KProtocolManager::NoProxy;
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
        policy = KIO::getCacheControlString(KIO::CC_Verify);
        break;
    case 1:
        policy = KIO::getCacheControlString(KIO::CC_Cache);
        break;
    case 2:
        policy = KIO::getCacheControlString(KIO::CC_CacheOnly);
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

#include "settingsplugin.moc"
