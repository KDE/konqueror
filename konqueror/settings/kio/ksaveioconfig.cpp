/*
   Copyright (C) 2001 Dawit Alemayehu <adawit@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

// Own
#include "ksaveioconfig.h"

// Qt
#include <QtDBus/QtDBus>

// KDE
#include <kconfig.h>
#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kconfiggroup.h>
#include <kio/ioslave_defaults.h>

class KSaveIOConfigPrivate
{
public:
  KSaveIOConfigPrivate ();
  ~KSaveIOConfigPrivate ();

  KConfig* config;
  KConfig* http_config;
};

K_GLOBAL_STATIC(KSaveIOConfigPrivate, d)

KSaveIOConfigPrivate::KSaveIOConfigPrivate ()
                     : config(0), http_config(0)
{
}

KSaveIOConfigPrivate::~KSaveIOConfigPrivate ()
{
  delete config;
  delete http_config;
}

static KConfig* config()
{
  if (!d->config)
     d->config = new KConfig("kioslaverc", KConfig::NoGlobals);

  return d->config;
}

static KConfig* http_config()
{
  if (!d->http_config)
     d->http_config = new KConfig("kio_httprc", KConfig::NoGlobals);

  return d->http_config;
}

int KSaveIOConfig::proxyDisplayUrlFlags()
{
    KConfigGroup cfg (config(), QString());
    return cfg.readEntry("ProxyUrlDisplayFlags", 0);
}

void KSaveIOConfig::setProxyDisplayUrlFlags (int flags)
{
    KConfigGroup cfg (config(), QString());
    cfg.writeEntry("ProxyUrlDisplayFlags", flags);
    cfg.sync();
}

void KSaveIOConfig::reparseConfiguration ()
{
  delete d->config;
  d->config = 0;
  delete d->http_config;
  d->http_config = 0;
}

void KSaveIOConfig::setReadTimeout( int _timeout )
{
  KConfigGroup cfg (config(), QString());
  cfg.writeEntry("ReadTimeout", qMax(MIN_TIMEOUT_VALUE,_timeout));
  cfg.sync();
}

void KSaveIOConfig::setConnectTimeout( int _timeout )
{
  KConfigGroup cfg (config(), QString());
  cfg.writeEntry("ConnectTimeout", qMax(MIN_TIMEOUT_VALUE,_timeout));
  cfg.sync();
}

void KSaveIOConfig::setProxyConnectTimeout( int _timeout )
{
  KConfigGroup cfg (config(), QString());
  cfg.writeEntry("ProxyConnectTimeout", qMax(MIN_TIMEOUT_VALUE,_timeout));
  cfg.sync();
}

void KSaveIOConfig::setResponseTimeout( int _timeout )
{
  KConfigGroup cfg (config(), QString());
  cfg.writeEntry("ResponseTimeout", qMax(MIN_TIMEOUT_VALUE,_timeout));
  cfg.sync();
}


void KSaveIOConfig::setMarkPartial( bool _mode )
{
  KConfigGroup cfg (config(), QString());
  cfg.writeEntry( "MarkPartial", _mode );
  cfg.sync();
}

void KSaveIOConfig::setMinimumKeepSize( int _size )
{
  KConfigGroup cfg (config(), QString());
  cfg.writeEntry( "MinimumKeepSize", _size );
  cfg.sync();
}

void KSaveIOConfig::setAutoResume( bool _mode )
{
  KConfigGroup cfg (config(), QString());
  cfg.writeEntry( "AutoResume", _mode );
  cfg.sync();
}

void KSaveIOConfig::setUseCache( bool _mode )
{
  KConfigGroup cfg (http_config(), QString());
  cfg.writeEntry( "UseCache", _mode );
  cfg.sync();
}

void KSaveIOConfig::setMaxCacheSize( int cache_size )
{
  KConfigGroup cfg (http_config(), QString());
  cfg.writeEntry( "MaxCacheSize", cache_size );
  cfg.sync();
}

void KSaveIOConfig::setCacheControl(KIO::CacheControl policy)
{
  KConfigGroup cfg (http_config(), QString());
  QString tmp = KIO::getCacheControlString(policy);
  cfg.writeEntry("cache", tmp);
  cfg.sync();
}

void KSaveIOConfig::setMaxCacheAge( int cache_age )
{
  KConfigGroup cfg (http_config(), QString());
  cfg.writeEntry( "MaxCacheAge", cache_age );
  cfg.sync();
}

void KSaveIOConfig::setUseReverseProxy( bool mode )
{
  KConfigGroup cfg (config(), "Proxy Settings");
  cfg.writeEntry("ReversedException", mode);
  cfg.sync();
}

void KSaveIOConfig::setProxyType(KProtocolManager::ProxyType type)
{
  KConfigGroup cfg (config(), "Proxy Settings");
  cfg.writeEntry("ProxyType", static_cast<int>(type));
  cfg.sync();
}

void KSaveIOConfig::setNoProxyFor( const QString& _noproxy )
{
  KConfigGroup cfg (config(), "Proxy Settings");
  cfg.writeEntry("NoProxyFor", _noproxy);
  cfg.sync();
}

void KSaveIOConfig::setProxyFor( const QString& protocol,
                                 const QString& _proxy )
{
  KConfigGroup cfg (config(), "Proxy Settings");
  cfg.writeEntry(protocol.toLower() + "Proxy", _proxy);
  cfg.sync();
}

void KSaveIOConfig::setProxyConfigScript( const QString& _url )
{
  KConfigGroup cfg (config(), "Proxy Settings");
  cfg.writeEntry("Proxy Config Script", _url);
  cfg.sync();
}

void KSaveIOConfig::updateRunningIOSlaves (QWidget *parent)
{
  // Inform all running io-slaves about the changes...
  // if we cannot update, ioslaves inform the end user...
  QDBusMessage message = QDBusMessage::createSignal("/KIO/Scheduler", "org.kde.KIO.Scheduler", "reparseSlaveConfiguration");
  message << QString();
  if (!QDBusConnection::sessionBus().send(message))
  {
    KMessageBox::information (parent,
                              i18n("You have to restart the running applications "
                                   "for these changes to take effect."),
                              i18nc("@title:window", "Update Failed"));
  }
}

void KSaveIOConfig::updateProxyScout(QWidget * parent)
{
  // Inform the proxyscout kded module about changes if we cannot update,
  // ioslaves inform the end user...
  QDBusInterface kded("org.kde.kded", "/modules/proxyscout", "org.kde.KPAC.ProxyScout");
  QDBusReply<void> reply = kded.call("reset");
  if (!reply.isValid())
  {
    KMessageBox::information (parent,
                              i18n("You have to restart KDE for these changes to take effect."),
                              i18nc("@title:window", "Update Failed"));
  }
}
