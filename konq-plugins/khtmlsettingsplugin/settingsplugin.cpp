/* This file is part of the KDE project
   Copyright (C) 2001 Carsten Pfeiffer <pfeiffer@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "settingsplugin.h"

#include <kaction.h>
#include <kconfig.h>
#include <kglobal.h>
#include <khtml_part.h>
#include <kcomponentdata.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmenu.h>
#include <kprotocolmanager.h>
#include <kpluginfactory.h>
#include <kaboutdata.h>
#include <ktoggleaction.h>
#include <kactioncollection.h>
#include <kselectaction.h>
#include <kactionmenu.h>
#include <kicon.h>
#include <kparts/part.h>
#include <kparts/htmlextension.h>

#include <QtDBus>

static const KAboutData aboutdata("khtmlsettingsplugin", 0, ki18n("HTML Settings") , "1.0" );
K_PLUGIN_FACTORY( SettingsPluginFactory, registerPlugin<SettingsPlugin>(); )
K_EXPORT_PLUGIN( SettingsPluginFactory( aboutdata ) )

SettingsPlugin::SettingsPlugin( QObject* parent,
                                const QVariantList & )
    : KParts::Plugin( parent ), mConfig(0)
{

    setComponentData(SettingsPluginFactory::componentData());

    KActionMenu *menu = new KActionMenu(KIcon("configure"), i18n("HTML Settings"), actionCollection() );
    actionCollection()->addAction( "action menu", menu );
    menu->setDelayed( false );

    KToggleAction* action = actionCollection()->add<KToggleAction>(QLatin1String("javascript"));
    action->setText( i18n("Java&Script") );
    connect( action, SIGNAL(triggered(bool)), SLOT(toggleJavascript(bool)) );
    menu->addAction( action );

    action = actionCollection()->add<KToggleAction>(QLatin1String("java"));
    action->setText(i18n("&Java")  );
    connect( action, SIGNAL(triggered(bool)), SLOT(toggleJava(bool)) );
    menu->addAction( action );

    action = actionCollection()->add<KToggleAction>(QLatin1String("cookies"));
    action->setText(i18n("&Cookies")  );
    connect( action, SIGNAL(triggered(bool)), SLOT(toggleCookies(bool)) );
    menu->addAction( action );

    action = actionCollection()->add<KToggleAction>(QLatin1String("plugins"));
    action->setText(i18n("&Plugins")  );
    connect( action, SIGNAL(triggered(bool)), SLOT(togglePlugins(bool)) );
    menu->addAction( action );

    action = actionCollection()->add<KToggleAction>(QLatin1String("imageloading"));
    action->setText(i18n("Autoload &Images")  );
    connect( action, SIGNAL(triggered(bool)), SLOT(toggleImageLoading(bool)) );
    menu->addAction( action );

    //menu->addAction( new KSeparatorAction(actionCollection()) );

    action = actionCollection()->add<KToggleAction>(QLatin1String("useproxy"));
    action->setText(i18n("Enable Pro&xy") );
    connect( action, SIGNAL(triggered(bool)), SLOT(toggleProxy(bool)) );
    menu->addAction( action );

    action = actionCollection()->add<KToggleAction>(QLatin1String("usecache"));
    action->setText(i18n("Enable Cac&he") );
    connect( action, SIGNAL(triggered(bool)), SLOT(toggleCache(bool)) );
    menu->addAction( action );


    KSelectAction* sAction = actionCollection()->add<KSelectAction>(QLatin1String("cachepolicy"));
    sAction->setText(i18n("Cache Po&licy") );
    QStringList policies;
    policies += i18n( "&Keep Cache in Sync" );
    policies += i18n( "&Use Cache if Possible" );
    policies += i18n( "&Offline Browsing Mode" );
    sAction->setItems( policies );
    connect( sAction, SIGNAL(triggered(int)), SLOT(cachePolicyChanged(int)) );

    menu->addAction( sAction );

    connect( menu->menu(), SIGNAL(aboutToShow()), SLOT(showPopup()));
}

SettingsPlugin::~SettingsPlugin()
{
    delete mConfig;
}

static KParts::HtmlSettingsInterface* settingsInterfaceFor(QObject* obj)
{
    KParts::HtmlExtension* extension = KParts::HtmlExtension::childObject(obj);
    return qobject_cast< KParts::HtmlSettingsInterface*>(extension);
}

void SettingsPlugin::showPopup()
{
    if (!mConfig) {
        mConfig = new KConfig("settingspluginrc", KConfig::NoGlobals);
    }

    KParts::ReadOnlyPart* part = qobject_cast<KParts::ReadOnlyPart*>(parent());

    KProtocolManager::reparseConfiguration();
    const bool cookies = cookiesEnabled( part->url().url() );
    actionCollection()->action(QLatin1String("cookies"))->setChecked(cookies);
    actionCollection()->action(QLatin1String("useproxy"))->setChecked(KProtocolManager::useProxy());
    actionCollection()->action(QLatin1String("usecache"))->setChecked(KProtocolManager::useCache());

    KParts::HtmlSettingsInterface* settings = settingsInterfaceFor(part);
    if (settings) {
        actionCollection()->action(QLatin1String("java"))->setChecked(settings->htmlSettingsProperty(KParts::HtmlSettingsInterface::JavaEnabled).toBool());
        actionCollection()->action(QLatin1String("javascript"))->setChecked(settings->htmlSettingsProperty(KParts::HtmlSettingsInterface::JavascriptEnabled).toBool());
        actionCollection()->action(QLatin1String("plugins"))->setChecked(settings->htmlSettingsProperty(KParts::HtmlSettingsInterface::PluginsEnabled).toBool());
        actionCollection()->action(QLatin1String("imageloading"))->setChecked(settings->htmlSettingsProperty(KParts::HtmlSettingsInterface::AutoLoadImages).toBool());
    }

    KIO::CacheControl cc = KProtocolManager::cacheControl();
    switch ( cc )
    {
        case KIO::CC_Verify:
            static_cast<KSelectAction*>(actionCollection()->action(QLatin1String("cachepolicy")))->setCurrentItem( 0 );
            break;
        case KIO::CC_CacheOnly:
            static_cast<KSelectAction*>(actionCollection()->action(QLatin1String("cachepolicy")))->setCurrentItem( 2 );
            break;
        case KIO::CC_Cache:
            static_cast<KSelectAction*>(actionCollection()->action(QLatin1String("cachepolicy")))->setCurrentItem( 1 );
            break;
        case KIO::CC_Reload: // nothing for now
        case KIO::CC_Refresh:
        default:
            break;
    }
}

void SettingsPlugin::toggleJava( bool checked )
{
    KParts::HtmlSettingsInterface* settings = settingsInterfaceFor(parent());
    if (settings) {
        settings->setHtmlSettingsProperty(KParts::HtmlSettingsInterface::JavaEnabled, checked);
    }
}

void SettingsPlugin::toggleJavascript( bool checked )
{
    KParts::HtmlSettingsInterface* settings = settingsInterfaceFor(parent());
    if (settings) {
        settings->setHtmlSettingsProperty(KParts::HtmlSettingsInterface::JavascriptEnabled, checked);
    }
}

void SettingsPlugin::toggleCookies( bool checked )
{
    KParts::ReadOnlyPart* part = qobject_cast<KParts::ReadOnlyPart*>(parent());
    if (part) {
        const QString advice((checked ? QLatin1String("Accept") : QLatin1String("Reject")));

        // TODO generate interface from the installed org.kde.KCookieServer.xml file
        // but not until 4.3 is released, since 4.2 had "void setDomainAdvice"
        // while 4.3 has "bool setDomainAdvice".

        QDBusInterface kded(QLatin1String("org.kde.kded"),
                            QLatin1String("/modules/kcookiejar"),
                            QLatin1String("org.kde.KCookieServer"));
        QDBusReply<void> reply = kded.call("setDomainAdvice", part->url().url(), advice);

        if ( !reply.isValid() )
            KMessageBox::sorry( part->widget(),
                                i18n("Cookies could not be enabled, because the "
                                     "cookie daemon could not be started."),
                                i18nc("@title:window", "Cookies Disabled"));
    }
}

void SettingsPlugin::togglePlugins( bool checked )
{
    KParts::HtmlSettingsInterface* settings = settingsInterfaceFor(parent());
    if (settings) {
        settings->setHtmlSettingsProperty(KParts::HtmlSettingsInterface::PluginsEnabled, checked);
    }
}

void SettingsPlugin::toggleImageLoading( bool checked )
{
    KParts::HtmlSettingsInterface* settings = settingsInterfaceFor(parent());
    if (settings) {
        settings->setHtmlSettingsProperty(KParts::HtmlSettingsInterface::AutoLoadImages, checked);
    }
}

bool SettingsPlugin::cookiesEnabled( const QString& url )
{
    QDBusInterface kded(QLatin1String("org.kde.kded"),
                        QLatin1String("/modules/kcookiejar"),
                        QLatin1String("org.kde.KCookieServer"));
    QDBusReply<QString> reply = kded.call( "getDomainAdvice", url);

    bool enabled = false;

    if ( reply.isValid() )
    {
        QString advice = reply;
        enabled = ( advice == QLatin1String("Accept") );
        if ( !enabled && advice == QLatin1String("Dunno") ) {
            // TODO, check the global setting via dcop
            KConfig _kc( "kcookiejarrc", KConfig::NoGlobals  );
            KConfigGroup kc(&_kc, "Cookie Policy" );
            enabled = (kc.readEntry("CookieGlobalAdvice", "Reject") == QLatin1String("Accept"));
        }
    }

    return enabled;
}


//
// sync with kcontrol/kio/ksaveioconfig.* !
//

void SettingsPlugin::toggleProxy( bool checked )
{
    KConfigGroup grp(mConfig, QString());
    int type;

    if (checked) {
        type = grp.readEntry("SavedProxyType", static_cast<int>(KProtocolManager::ManualProxy));
    } else {
        grp.writeEntry("SavedProxyType", static_cast<int>(KProtocolManager::proxyType()));
        type = KProtocolManager::NoProxy;
    }

    KConfig _config("kioslaverc", KConfig::NoGlobals);
    KConfigGroup config(&_config, "Proxy Settings" );
    config.writeEntry( "ProxyType", type );

    actionCollection()->action(QLatin1String("useproxy"))->setChecked(checked);
    updateIOSlaves();
}


void SettingsPlugin::toggleCache( bool checked )
{
    KConfig config("kio_httprc", KConfig::NoGlobals);
    KConfigGroup grp(&config, QString());
    grp.writeEntry( "UseCache", checked );
    actionCollection()->action(QLatin1String("usecache"))->setChecked( checked );

    updateIOSlaves();
}

void SettingsPlugin::cachePolicyChanged( int p )
{
    QString policy;

    switch ( p ) {
    case 0:
        policy = KIO::getCacheControlString( KIO::CC_Verify );
        break;
    case 1:
        policy = KIO::getCacheControlString( KIO::CC_Cache );
        break;
    case 2:
        policy = KIO::getCacheControlString( KIO::CC_CacheOnly );
        break;
    };

    if ( !policy.isEmpty() ) {
        KConfig config("kio_httprc", KConfig::NoGlobals);
        KConfigGroup grp(&config, QString());
        grp.writeEntry("cache", policy);

        updateIOSlaves();
    }
}

void SettingsPlugin::updateIOSlaves()
{
    QDBusMessage message = QDBusMessage::createSignal(QLatin1String("/KIO/Scheduler"),
                                                      QLatin1String("org.kde.KIO.Scheduler"),
                                                      QLatin1String("reparseSlaveConfiguration"));
    message << QString();
    QDBusConnection::sessionBus().send(message);
}

#include "settingsplugin.moc"
