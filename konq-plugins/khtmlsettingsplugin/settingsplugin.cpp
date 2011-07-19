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
#include "settingsplugin.h"
#include <QtDBus>
#include <kicon.h>

static const KAboutData aboutdata("khtmlsettingsplugin", 0, ki18n("HTML Settings") , "1.0" );
K_PLUGIN_FACTORY( SettingsPluginFactory, registerPlugin<SettingsPlugin>(); )
K_EXPORT_PLUGIN( SettingsPluginFactory( aboutdata ) )

SettingsPlugin::SettingsPlugin( QObject* parent,
                                const QVariantList & )
    : KParts::Plugin( parent ), mConfig(0)
{

    setComponentData(SettingsPluginFactory::componentData());


    KActionMenu *menu = new KActionMenu(KIcon("configure"), i18n("HTML Settings"),
					 actionCollection() );
    actionCollection()->addAction( "action menu", menu );
    menu->setDelayed( false );

    KToggleAction *action;


    action = actionCollection()->add<KToggleAction>( "javascript" );
    action->setText( i18n("Java&Script") );
    connect( action, SIGNAL( toggled( bool ) ), SLOT( toggleJavascript( bool ) ) );
    menu->addAction( action );

    action = actionCollection()->add<KToggleAction>( "java" );
    action->setText(i18n("&Java")  );
    connect( action, SIGNAL( toggled( bool ) ), SLOT( toggleJava( bool ) ) );
    menu->addAction( action );

    action = actionCollection()->add<KToggleAction>( "cookies" );
    action->setText(i18n("&Cookies")  );
    connect( action, SIGNAL( toggled( bool ) ), SLOT( toggleCookies( bool ) ) );
    menu->addAction( action );

    action = actionCollection()->add<KToggleAction>( "plugins" );
    action->setText(i18n("&Plugins")  );
    connect( action, SIGNAL( toggled( bool ) ), SLOT( togglePlugins( bool ) ) );
    menu->addAction( action );

    action = actionCollection()->add<KToggleAction>( "imageloading" );
    action->setText(i18n("Autoload &Images")  );
    connect( action, SIGNAL( toggled( bool ) ), SLOT( toggleImageLoading( bool ) ) );
    menu->addAction( action );

    //menu->addAction( new KSeparatorAction(actionCollection()) );

    action = actionCollection()->add<KToggleAction>( "useproxy" );
    action->setText(i18n("Enable Pro&xy") );
    connect( action, SIGNAL( toggled( bool ) ), SLOT( toggleProxy( bool ) ) );
    menu->addAction( action );

    action = actionCollection()->add<KToggleAction>( "usecache" );
    action->setText(i18n("Enable Cac&he") );
    connect( action, SIGNAL( toggled( bool ) ), SLOT( toggleCache( bool ) ) );
    menu->addAction( action );


    KSelectAction* sAction = actionCollection()->add<KSelectAction>( "cachepolicy" );
    sAction->setText(i18n("Cache Po&licy") );
    QStringList policies;
    policies += i18n( "&Keep Cache in Sync" );
    policies += i18n( "&Use Cache if Possible" );
    policies += i18n( "&Offline Browsing Mode" );
    sAction->setItems( policies );
    connect( sAction, SIGNAL( triggered( int ) ), SLOT( cachePolicyChanged(int) ) );

    menu->addAction( sAction );

    connect( menu->menu(), SIGNAL( aboutToShow() ), SLOT( showPopup() ));
}

SettingsPlugin::~SettingsPlugin()
{
  delete mConfig;
}

void SettingsPlugin::showPopup()
{
    KHTMLPart *part = qobject_cast<KHTMLPart *>( parent() );
    if (!part) {
        return;
    }

  if (!mConfig)
    mConfig = new KConfig("settingspluginrc", KConfig::NoGlobals);


  KProtocolManager::reparseConfiguration();
  const bool cookies = cookiesEnabled( part->url().url() );

  ((KToggleAction*)actionCollection()->action("useproxy"))->setChecked(KProtocolManager::useProxy());
  ((KToggleAction*)actionCollection()->action("java"))->setChecked( part->javaEnabled() );
  ((KToggleAction*)actionCollection()->action("javascript"))->setChecked( part->jScriptEnabled() );
  ((KToggleAction*)actionCollection()->action("cookies"))->setChecked( cookies );
  ((KToggleAction*)actionCollection()->action("plugins"))->setChecked( part->pluginsEnabled() );
  ((KToggleAction*)actionCollection()->action("imageloading"))->setChecked( part->autoloadImages() );
  ((KToggleAction*)actionCollection()->action("usecache"))->setChecked(KProtocolManager::useCache());

  KIO::CacheControl cc = KProtocolManager::cacheControl();
  switch ( cc )
  {
      case KIO::CC_Verify:
          ((KSelectAction*)actionCollection()->action("cachepolicy"))->setCurrentItem( 0 );
          break;
      case KIO::CC_CacheOnly:
          ((KSelectAction*)actionCollection()->action("cachepolicy"))->setCurrentItem( 2 );
          break;
      case KIO::CC_Cache:
          ((KSelectAction*)actionCollection()->action("cachepolicy"))->setCurrentItem( 1 );
          break;
      case KIO::CC_Reload: // nothing for now
      case KIO::CC_Refresh:
      default:
          break;

  }
}

void SettingsPlugin::toggleJava( bool checked )
{
    if (KHTMLPart *part = qobject_cast<KHTMLPart *>(parent())) {
        part->setJavaEnabled( checked );
    }
}

void SettingsPlugin::toggleJavascript( bool checked )
{
    if (KHTMLPart *part = qobject_cast<KHTMLPart *>(parent())) {
        part->setJScriptEnabled( checked );
    }
}

void SettingsPlugin::toggleCookies( bool checked )
{
    if (KHTMLPart *part = qobject_cast<KHTMLPart *>(parent())) {
        const QString advice = checked ? "Accept" : "Reject";

        // TODO generate interface from the installed org.kde.KCookieServer.xml file
        // but not until 4.3 is released, since 4.2 had "void setDomainAdvice"
        // while 4.3 has "bool setDomainAdvice".

        QDBusInterface kded("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer");
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
    if (KHTMLPart *part = qobject_cast<KHTMLPart *>(parent())) {
        part->setPluginsEnabled( checked );
    }
}

void SettingsPlugin::toggleImageLoading( bool checked )
{
    if (KHTMLPart *part = qobject_cast<KHTMLPart *>(parent())) {
        part->setAutoloadImages( checked );
    }
}

bool SettingsPlugin::cookiesEnabled( const QString& url )
{
    QDBusInterface kded("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer");
    QDBusReply<QString> reply = kded.call( "getDomainAdvice", url);

    bool enabled = false;

    if ( reply.isValid() )
    {
        QString advice = reply;
        enabled = ( advice == "Accept" );
        if ( !enabled && advice == "Dunno" ) {
            // TODO, check the global setting via dcop
            KConfig _kc( "kcookiejarrc", KConfig::NoGlobals  );
            KConfigGroup kc(&_kc, "Cookie Policy" );
            enabled =
          (kc.readEntry( "CookieGlobalAdvice", "Reject" ) == "Accept");
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
        type = grp.readEntry( "SavedProxyType", int(KProtocolManager::ManualProxy) );
    } else {
        grp.writeEntry( "SavedProxyType", int(KProtocolManager::proxyType()) );
        type = KProtocolManager::NoProxy;
    }

    KConfig _config("kioslaverc", KConfig::NoGlobals);
    KConfigGroup config(&_config, "Proxy Settings" );
    config.writeEntry( "ProxyType", type );

    ((KToggleAction*)actionCollection()->action("useproxy"))->setChecked(checked);
    updateIOSlaves();
}


void SettingsPlugin::toggleCache( bool checked )
{
    KConfig config("kio_httprc", KConfig::NoGlobals);
    KConfigGroup grp(&config, QString());
    grp.writeEntry( "UseCache", checked );
    ((KToggleAction*)actionCollection()->action("usecache"))->setChecked( checked );

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
    QDBusMessage message =
            QDBusMessage::createSignal("/KIO/Scheduler", "org.kde.KIO.Scheduler", "reparseSlaveConfiguration");
    message << QString();
    QDBusConnection::sessionBus().send(message);
}

#include "settingsplugin.moc"
