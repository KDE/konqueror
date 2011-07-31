/*
    Copyright (c) 2001 Dawit Alemayehu <adawit@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License (LGPL) as published by the Free Software Foundation;
    either version 2 of the License, or (at your option) any later
    version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <sys/utsname.h>

#include <qregexp.h>
#include <QDBusConnection>
#include <QDBusMessage>
#include <kicon.h>
#include <kactionmenu.h>
#include <kservicetypetrader.h>
#include <krun.h>
#include <kdebug.h>
#include <kaction.h>
#include <klocale.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kio/job.h>
#include <kio/scheduler.h>
#include <kservice.h>
#include <kcomponentdata.h>
#include <kmenu.h>
#include <KConfigGroup>
#include <kparts/part.h>
#include <kpluginfactory.h>
#include <kprotocolmanager.h>
#include <kaboutdata.h>
#include <kactioncollection.h>
#include "uachangerplugin.h"

static const KAboutData aboutdata("uachangerplugin", 0, ki18n("Change Browser Identification") , "1.0" );
K_PLUGIN_FACTORY(UAChangerPluginFactory, registerPlugin<UAChangerPlugin>();)
K_EXPORT_PLUGIN(UAChangerPluginFactory (aboutdata))


#define UA_PTOS(x) (*it)->property(x).toString()
#define QFL1(x) QLatin1String(x)


UAChangerPlugin::UAChangerPlugin( QObject* parent,
                                  const QVariantList & )
                :KParts::Plugin( parent ),
                  m_bSettingsLoaded(false), m_part(0L), m_config(0L)
{
  setComponentData(UAChangerPlugin::componentData());

  m_pUAMenu = new KActionMenu( KIcon("preferences-web-browser-identification"), i18n("Change Browser &Identification"),
                               actionCollection() );
  actionCollection()->addAction("changeuseragent", m_pUAMenu);
  m_pUAMenu->setDelayed( false );
  connect( m_pUAMenu->menu(), SIGNAL(aboutToShow()),
           this, SLOT(slotAboutToShow()) );

  if (parent) {
      m_part = qobject_cast<KParts::ReadOnlyPart *>(parent );
      connect(m_part, SIGNAL(started(KIO::Job*)), this,
              SLOT(slotEnableMenu()) );
      connect(m_part, SIGNAL(completed()), this,
              SLOT(slotEnableMenu()));
      connect(m_part, SIGNAL(completed(bool)), this,
              SLOT(slotEnableMenu()));
  }
}

UAChangerPlugin::~UAChangerPlugin()
{
  saveSettings();
  slotReloadDescriptions();
}

void UAChangerPlugin::slotReloadDescriptions()
{
  delete m_config;
  m_config = 0L;
}

void UAChangerPlugin::parseDescFiles()
{
  const KService::List list = KServiceTypeTrader::self()->query("UserAgentStrings");
  if (list.isEmpty())
    return;

  m_mapAlias.clear();
  m_lstAlias.clear();
  m_lstIdentity.clear();

  struct utsname utsn;
  uname( &utsn );

  QStringList languageList = KGlobal::locale()->languageList();
  if ( !languageList.isEmpty() )
  {
     const int index = languageList.indexOf(QFL1("C"));
     if(index > -1)
     {
       if( languageList.contains( QFL1("en") ) )
         languageList.removeAt(index);
       else
         languageList[index] = QFL1("en");
     }
  }

  KService::List::ConstIterator it = list.constBegin();
  KService::List::ConstIterator lastItem = list.constEnd();

  for ( ; it != lastItem; ++it )
  {
    QString ua  = UA_PTOS("X-KDE-UA-FULL");
    QString tag = UA_PTOS("X-KDE-UA-TAG");

    // The menu groups thing by tag, with the menu name being the X-KDE-UA-NAME by default. We make groups for
    // IE, NS, Firefox, Safari, and Opera, and put everything else into "Other"
    QString menuName;
    MenuGroupSortKey menuKey; // key for the group..
    if (tag != "IE" && tag != "NN" && tag != "FF" && tag != "SAF" && tag != "OPR")
    {
      tag = "OTHER";
      menuName = i18n("Other");
      menuKey = MenuGroupSortKey(tag, true);
    }
    else
    {
      menuName = UA_PTOS("X-KDE-UA-NAME");
      menuKey  = MenuGroupSortKey(tag, false);
    }

    if ( (*it)->property("X-KDE-UA-DYNAMIC-ENTRY").toBool() )
    {
      ua.replace( QFL1("appSysName"), QFL1(utsn.sysname) );
      ua.replace( QFL1("appSysRelease"), QFL1(utsn.release) );
      ua.replace( QFL1("appMachineType"), QFL1(utsn.machine) );
      ua.replace( QFL1("appLanguage"), languageList.join(QFL1(", ")) );
      ua.replace( QFL1("appPlatform"), QFL1("X11") );
    }

    if ( m_lstIdentity.contains(ua) )
      continue; // Ignore dups!

    m_lstIdentity << ua;

    // Compute what to display for our menu entry --- including platform name if it's available,
    // and avoiding repeating the browser name in categories other than 'other'.
    QString platform = QString("%1 %2").arg(UA_PTOS("X-KDE-UA-SYSNAME")).arg(UA_PTOS("X-KDE-UA-SYSRELEASE"));

    QString alias;
    if ( platform.trimmed().isEmpty() )
    {
      if(!menuKey.isOther)
        alias = i18nc("%1 = browser version (e.g. 2.0)", "Version %1", UA_PTOS("X-KDE-UA-VERSION"));
      else
        alias = i18nc("%1 = browser name, %2 = browser version (e.g. Firefox, 2.0)",
                      "%1 %2", UA_PTOS("X-KDE-UA-NAME"), UA_PTOS("X-KDE-UA-VERSION"));
    }
    else
    {
      if(!menuKey.isOther)
        alias = i18nc("%1 = browser version, %2 = platform (e.g. 2.0, Windows XP)",
                      "Version %1 on %2", UA_PTOS("X-KDE-UA-VERSION"), platform);
      else
        alias = i18nc("%1 = browser name, %2 = browser version, %3 = platform (e.g. Firefox, 2.0, Windows XP)",
                    "%1 %2 on %3", UA_PTOS("X-KDE-UA-NAME"), UA_PTOS("X-KDE-UA-VERSION"), platform);
    }

    m_lstAlias << alias;

    /* sort in this UA Alias alphabetically */
    BrowserGroup ualist = m_mapAlias[menuKey];
    BrowserGroup::Iterator e = ualist.begin();
    while ( !alias.isEmpty() && e != ualist.end() )
    {
      if ( m_lstAlias[(*e)] > alias ) {
         ualist.insert( e, m_lstAlias.count()-1 );
         alias.clear();
      }
      ++e;
    }

    if ( !alias.isEmpty() )
      ualist.append( m_lstAlias.count()-1 );

    m_mapAlias[menuKey]   = ualist;
    m_mapBrowser[menuKey] = menuName;
  }
}

void UAChangerPlugin::slotEnableMenu()
{
  m_currentURL = m_part->url();

  // This plugin works on local files, http[s], and webdav[s].
  QString proto = m_currentURL.protocol();
  if (m_currentURL.isLocalFile() ||
      proto.startsWith("http") || proto.startsWith("webdav")) {
    if (!m_pUAMenu->isEnabled())
      m_pUAMenu->setEnabled ( true );
  } else {
    m_pUAMenu->setEnabled ( false );
  }
}

void UAChangerPlugin::slotAboutToShow()
{
  if (!m_config)
  {
    m_config = new KConfig( "kio_httprc" );
    parseDescFiles();
  }

  if (!m_bSettingsLoaded) {
    loadSettings();
  }

  if (m_pUAMenu->menu()->actions().isEmpty()) // need to create the actions
  {
      m_pUAMenu->menu()->addTitle(i18n("Identify As")); // imho title doesn't need colon..

      m_defaultAction = new QAction(i18n("Default Identification"), this);
      m_defaultAction->setCheckable(true);
      connect(m_defaultAction, SIGNAL(triggered()), this, SLOT(slotDefault()));
      m_pUAMenu->menu()->addAction(m_defaultAction);

      m_pUAMenu->menu()->addSeparator();

      m_actionGroup = new QActionGroup(m_pUAMenu->menu());
      AliasConstIterator map = m_mapAlias.constBegin();
      for( ; map != m_mapAlias.constEnd(); ++map )
      {
          QMenu* browserMenu = m_pUAMenu->menu()->addMenu( m_mapBrowser.value(map.key()) );
          BrowserGroup::ConstIterator e = map.value().begin();
          for( ; e != map.value().end(); ++e )
          {
              QAction* action = new QAction(m_lstAlias[*e], m_actionGroup);
              action->setCheckable(true);
              action->setData(*e);
              browserMenu->addAction(action);
          }
      }
      connect(m_actionGroup, SIGNAL(triggered(QAction*)), this, SLOT(slotItemSelected(QAction*)));

      m_pUAMenu->menu()->addSeparator();

      /* useless here, imho..
         m_pUAMenu->menu()->insertItem( i18n("Reload Identifications"), this,
         SLOT(slotReloadDescriptions()),
         0, ++count );*/

      m_applyEntireSiteAction = new QAction(i18n("Apply to Entire Site"), this);
      m_applyEntireSiteAction->setCheckable(true);
      connect(m_applyEntireSiteAction, SIGNAL(triggered()), this, SLOT(slotApplyToDomain()));
      m_pUAMenu->menu()->addAction(i18n("Apply to Entire Site"));

      m_pUAMenu->menu()->addAction( i18n("Configure..."), this,
                                    SLOT(slotConfigure()));
  }

  // Reflect current settings in the actions

  QString host = m_currentURL.isLocalFile() ? QFL1("localhost") : m_currentURL.host();
  m_currentUserAgent = KProtocolManager::userAgentForHost(host);
  //kDebug(90130) << "User Agent: " << m_currentUserAgent;
  m_defaultAction->setChecked(m_currentUserAgent == KProtocolManager::defaultUserAgent());

  m_applyEntireSiteAction->setChecked(m_bApplyToDomain);
  Q_FOREACH(QAction* action, m_actionGroup->actions()) {
      const int id = action->data().toInt();
      action->setChecked(m_lstIdentity[id] == m_currentUserAgent);
  }

}

void UAChangerPlugin::slotConfigure()
{
  KService::Ptr service = KService::serviceByDesktopName ("useragent");
  if (service)
    KRun::runCommand (service->exec (), m_part->widget());
}

void UAChangerPlugin::slotItemSelected(QAction* action)
{
  const int id = action->data().toInt();
  if (m_lstIdentity[id] == m_currentUserAgent) return;

  m_currentUserAgent = m_lstIdentity[id];
  QString host = m_currentURL.isLocalFile() ? QFL1("localhost") : filterHost( m_currentURL.host() );

  KConfigGroup grp = m_config->group( host.toLower() );
  grp.writeEntry( "UserAgent", m_currentUserAgent );
  //kDebug(90130) << "Writing out UserAgent=" << m_currentUserAgent << "for host=" << host;
  grp.sync();

  // Reload the page with the new user-agent string
  reloadPage();
}

void UAChangerPlugin::slotDefault()
{
  if( m_currentUserAgent == KProtocolManager::defaultUserAgent() ) return; // don't flicker!
  // We have no choice but delete all higher domain level settings here since it
  // affects what will be matched.
  QStringList partList = m_currentURL.host().split(' ',QString::SkipEmptyParts);
  if ( !partList.isEmpty() )
  {
    partList.removeFirst();

    QStringList domains;
    // Remove the exact name match...
    domains << m_currentURL.host ();

    while (partList.count())
    {
      if (partList.count() == 2)
        if (partList[0].length() <=2 && partList[1].length() ==2)
          break;

      if (partList.count() == 1)
        break;

      domains << partList.join(QFL1("."));
      partList.removeFirst();
    }

    KConfigGroup grp( m_config, QString());
    for (QStringList::Iterator it = domains.begin(); it != domains.end(); it++)
    {
      //kDebug () << "Domain to remove: " << *it;
      if ( grp.hasGroup(*it) )
        grp.deleteGroup(*it);
      else if( grp.hasKey(*it) )
        grp.deleteEntry(*it);
    }
  }
  else
      if ( m_currentURL.isLocalFile() && m_config->hasGroup( "localhost" ) )
          m_config->deleteGroup( "localhost" );

  m_config->sync();

  // Reset some internal variables and inform the http io-slaves of the changes.
  m_currentUserAgent = KProtocolManager::defaultUserAgent();

  // Reload the page with the default user-agent
  reloadPage();
}

void UAChangerPlugin::reloadPage()
{
  // Inform running http(s) io-slaves about the change...
  KIO::Scheduler::emitReparseSlaveConfiguration();

  KParts::OpenUrlArguments args = m_part->arguments();
  args.setReload(true);
  m_part->setArguments(args);
  m_part->openUrl(m_currentURL);
}

QString UAChangerPlugin::filterHost(const QString &hostname)
{
  QRegExp rx;

  // Check for IPv4 address
  rx.setPattern ("[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}");
  if (rx.exactMatch (hostname))
    return hostname;

  // Check for IPv6 address here...
  rx.setPattern ("^\\[.*\\]$");
  if (rx.exactMatch (hostname))
    return hostname;

  // Return the TLD if apply to domain or
  return (m_bApplyToDomain ? findTLD(hostname): hostname);
}

QString UAChangerPlugin::findTLD (const QString &hostname)
{
  QStringList domains;
  QStringList partList =  hostname.split(' ',QString::SkipEmptyParts);

  if (partList.count())
      partList.removeFirst(); // Remove hostname

  while(partList.count())
  {
    // We only have a TLD left.
    if (partList.count() == 1)
        break;

    if( partList.count() == 2 )
    {
        // The .name domain uses <name>.<surname>.name
        // Although the TLD is striclty speaking .name, for our purpose
        // it should be <surname>.name since people should not be able
        // to set cookies for everyone with the same surname.
        // Matches <surname>.name
        if( partList[1].toLower() == QFL1("name") )
        {
          break;
        }
        else if( partList[1].length() == 2 )
        {
          // If this is a TLD, we should stop. (e.g. co.uk)
          // We assume this is a TLD if it ends with .xx.yy or .x.yy
          if (partList[0].length() <= 2)
              break; // This is a TLD.

          // Catch some TLDs that we miss with the previous check
          // e.g. com.au, org.uk, mil.co
          QByteArray t = partList[0].toLower().toUtf8();
          if ((t == "com") || (t == "net") || (t == "org") || (t == "gov") ||
              (t == "edu") || (t == "mil") || (t == "int"))
              break;
        }
    }

    domains.append(partList.join(QFL1(".")));
    partList.removeFirst(); // Remove part
  }

  if( domains.isEmpty() )
    return hostname;

  return domains[0];
}

void UAChangerPlugin::saveSettings()
{
  if(!m_bSettingsLoaded) return;

  KConfig cfg ("uachangerrc", KConfig::NoGlobals);
  KConfigGroup grp = cfg.group ("General");

  grp.writeEntry ("applyToDomain", m_bApplyToDomain);
}

void UAChangerPlugin::loadSettings()
{
  KConfig cfg ("uachangerrc", KConfig::NoGlobals);
  KConfigGroup grp = cfg.group ("General");

  m_bApplyToDomain = grp.readEntry ("applyToDomain", true);
  m_bSettingsLoaded = true;
}

void UAChangerPlugin::slotApplyToDomain()
{
  m_bApplyToDomain = !m_bApplyToDomain;
}

#include "uachangerplugin.moc"
