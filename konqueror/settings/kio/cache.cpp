/*
   cache.cpp - Proxy configuration dialog

   Copyright (C) 2001,02,03 Dawit Alemayehu <adawit@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License (GPL) version 2 as published by the Free Software
   Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

// Own
#include "cache.h"

// Qt
#include <QtGui/QBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>

// KDE
#include <kprocess.h>
#include <kdebug.h>
#include <kgenericfactory.h>
#include <kio/http_slave_defaults.h>
#include <klocale.h>
#include <knuminput.h>

// Local
#include "ksaveioconfig.h"

K_PLUGIN_FACTORY_DECLARATION(KioConfigFactory)

CacheConfigModule::CacheConfigModule(QWidget *parent, const QVariantList &)
                  :KCModule(KioConfigFactory::componentData(), parent)
{
  ui.setupUi(this);
  load();
}

CacheConfigModule::~CacheConfigModule()
{
}

void CacheConfigModule::load()
{
  ui.cbUseCache->setChecked(KProtocolManager::useCache());
  ui.sbMaxCacheSize->setValue( KProtocolManager::maxCacheSize() );

  KIO::CacheControl cc = KProtocolManager::cacheControl();

  if (cc==KIO::CC_Verify)
      ui.rbVerifyCache->setChecked( true );
  else if (cc==KIO::CC_Refresh)
      ui.rbVerifyCache->setChecked( true );
  else if (cc==KIO::CC_CacheOnly)
      ui.rbOfflineMode->setChecked( true );
  else if (cc==KIO::CC_Cache)
      ui.rbCacheIfPossible->setChecked( true );

  // Config changed notifications...
  connect ( ui.cbUseCache, SIGNAL(toggled(bool)), SLOT(configChanged()) );
  connect ( ui.rbVerifyCache, SIGNAL(toggled(bool)), SLOT(configChanged()) );
  connect ( ui.rbOfflineMode, SIGNAL(toggled(bool)), SLOT(configChanged()) );
  connect ( ui.rbCacheIfPossible, SIGNAL(toggled(bool)), SLOT(configChanged()) );
  connect ( ui.sbMaxCacheSize, SIGNAL(valueChanged(int)), SLOT(configChanged()) );
  emit changed( false );
} 

void CacheConfigModule::save()
{
  KSaveIOConfig::setUseCache( ui.cbUseCache->isChecked() );
  KSaveIOConfig::setMaxCacheSize( ui.sbMaxCacheSize->value() );

  if ( !ui.cbUseCache->isChecked() )
      KSaveIOConfig::setCacheControl(KIO::CC_Refresh);
  else if ( ui.rbVerifyCache->isChecked() )
      KSaveIOConfig::setCacheControl(KIO::CC_Refresh);
  else if ( ui.rbOfflineMode->isChecked() )
      KSaveIOConfig::setCacheControl(KIO::CC_CacheOnly);
  else if ( ui.rbCacheIfPossible->isChecked() )
      KSaveIOConfig::setCacheControl(KIO::CC_Cache);

  // Update running io-slaves...
  KSaveIOConfig::updateRunningIOSlaves (this);

  emit changed( false );
}

void CacheConfigModule::defaults()
{
  ui.cbUseCache->setChecked( true );
  ui.rbVerifyCache->setChecked( true );
  ui.sbMaxCacheSize->setValue( DEFAULT_MAX_CACHE_SIZE );

  emit changed( true );
}

QString CacheConfigModule::quickHelp() const
{
  return i18n( "<h1>Cache</h1><p>This module lets you configure your cache settings.</p>"
                "<p>The cache is an internal memory in Konqueror where recently "
                "read web pages are stored. If you want to retrieve a web "
                "page again that you have recently read, it will not be "
                "downloaded from the Internet, but rather retrieved from the "
                "cache, which is a lot faster.</p>" );
}

void CacheConfigModule::configChanged()
{
  emit changed( true );
}

void CacheConfigModule::on_clearCacheButton_clicked()
{
  KProcess::startDetached(QLatin1String("kio_http_cache_cleaner"),
                          QStringList(QLatin1String("--clear-all")));
}

#include "cache.moc"
