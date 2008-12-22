/*
   kproxydlg.cpp - Proxy configuration dialog

   Copyright (C) 2001- Dawit Alemayehu <adawit@kde.org>

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
#include "kproxydlg.h"

// Qt
#include <QtCore/QRegExp>
#include <QtGui/QBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QRadioButton>
#include <QtGui/QTabWidget>

// KDE
#include <kgenericfactory.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>

// Local
#include "kenvvarproxydlg.h"
#include "kmanualproxydlg.h"
#include "ksaveioconfig.h"

K_PLUGIN_FACTORY_DECLARATION(KioConfigFactory)

KProxyDialog::KProxyDialog(QWidget *parent, const QVariantList &args)
    : KCModule(KioConfigFactory::componentData(), parent)
{
  mUi.setupUi(this);
  
  // signals and slots connections
  connect( mUi.rbNoProxy, SIGNAL( toggled(bool) ),
            SLOT( slotUseProxyChanged() ) );
  
  connect( mUi.rbAutoDiscover, SIGNAL( toggled(bool) ),
            SLOT( slotChanged() ) );
  connect( mUi.rbAutoScript, SIGNAL( toggled(bool) ),
            SLOT( slotChanged() ) );
  
  connect( mUi.rbPrompt, SIGNAL( toggled(bool) ),
            SLOT( slotChanged() ) );
  connect( mUi.rbPresetLogin, SIGNAL( toggled(bool) ),
            SLOT( slotChanged() ) );
  
  connect( mUi.cbPersConn, SIGNAL( toggled(bool) ),
            SLOT( slotChanged() ) );
  
  connect( mUi.location, SIGNAL( textChanged(const QString&) ),
            SLOT( slotChanged() ) );
  
  connect( mUi.pbEnvSetup, SIGNAL( clicked() ), SLOT( setupEnvProxy() ) );
  connect( mUi.pbManSetup, SIGNAL( clicked() ), SLOT( setupManProxy() ) );
  
}

KProxyDialog::~KProxyDialog()
{
}

void KProxyDialog::load()
{
  mDefaultData = false;

  KProtocolManager proto;
  bool useProxy = proto.useProxy();
  mData.type = proto.proxyType();
  mData.proxyList["http"] = proto.proxyFor( "http" );
  mData.proxyList["https"] = proto.proxyFor( "https" );
  mData.proxyList["ftp"] = proto.proxyFor( "ftp" );
  mData.proxyList["script"] = proto.proxyConfigScript();
  mData.useReverseProxy = proto.useReverseProxy();
  mData.noProxyFor = proto.noProxyFor().split( QRegExp("[',''\t'' ']"), QString::SkipEmptyParts);

  mUi.gbAuth->setEnabled( useProxy );
  mUi.gbOptions->setEnabled( useProxy );

  mUi.cbPersConn->setChecked( proto.persistentProxyConnection() );

  if ( !mData.proxyList["script"].isEmpty() )
    mUi.location->lineEdit()->setText( mData.proxyList["script"] );

  switch ( mData.type )
  {
    case KProtocolManager::WPADProxy:
      mUi.rbAutoDiscover->setChecked( true );
      break;
    case KProtocolManager::PACProxy:
      mUi.rbAutoScript->setChecked( true );
      break;
    case KProtocolManager::ManualProxy:
      mUi.rbManual->setChecked( true );
      break;
    case KProtocolManager::EnvVarProxy:
      mUi.rbEnvVar->setChecked( true );
      break;
    case KProtocolManager::NoProxy:
    default:
      mUi.rbNoProxy->setChecked( true );
      break;
  }

  switch( proto.proxyAuthMode() )
  {
    case KProtocolManager::Prompt:
      mUi.rbPrompt->setChecked( true );
      break;
    case KProtocolManager::Automatic:
      mUi.rbPresetLogin->setChecked( true );
    default:
      break;
  }
}

void KProxyDialog::save()
{
  bool updateProxyScout = false;

  if (mDefaultData)
    mData.reset ();

  if ( mUi.rbNoProxy->isChecked() )
  {
    KSaveIOConfig::setProxyType( KProtocolManager::NoProxy );
  }
  else
  {
    if ( mUi.rbAutoDiscover->isChecked() )
    {
      KSaveIOConfig::setProxyType( KProtocolManager::WPADProxy );
      updateProxyScout = true;
    }
    else if ( mUi.rbAutoScript->isChecked() )
    {
      KUrl u( mUi.location->lineEdit()->text() );

      if ( !u.isValid() )
      {
        showInvalidMessage( i18n("The address of the automatic proxy "
                                 "configuration script is invalid. Please "
                                 "correct this problem before proceeding. "
                                 "Otherwise, your changes will be "
                                 "ignored.") );
        return;
      }
      else
      {
        KSaveIOConfig::setProxyType( KProtocolManager::PACProxy );
        mData.proxyList["script"] = u.url();
        updateProxyScout = true;
      }
    }
    else if ( mUi.rbManual->isChecked() )
    {
      if ( mData.type != KProtocolManager::ManualProxy )
      {
        // Let's try a bit harder to determine if the previous
        // proxy setting was indeed a manual proxy
        KUrl u ( mData.proxyList["http"] );
        bool validProxy = (u.isValid() && u.port() > 0);
        u = mData.proxyList["https"];
        validProxy = validProxy || (u.isValid() && u.port() > 0);
        u = mData.proxyList["ftp"];
        validProxy = validProxy || (u.isValid() && u.port() > 0);

        if (!validProxy)
        {
          showInvalidMessage();
          return;
        }

        mData.type = KProtocolManager::ManualProxy;
      }

      KSaveIOConfig::setProxyType( KProtocolManager::ManualProxy );
    }
    else if ( mUi.rbEnvVar->isChecked() )
    {
      if ( mData.type != KProtocolManager::EnvVarProxy )
      {
        showInvalidMessage();
        return;
      }

      KSaveIOConfig::setProxyType( KProtocolManager::EnvVarProxy );
    }

    if ( mUi.rbPrompt->isChecked() )
      KSaveIOConfig::setProxyAuthMode( KProtocolManager::Prompt );
    else if ( mUi.rbPresetLogin->isChecked() )
      KSaveIOConfig::setProxyAuthMode( KProtocolManager::Automatic );
  }

  KSaveIOConfig::setPersistentProxyConnection( mUi.cbPersConn->isChecked() );

  // Save the common proxy setting...
  KSaveIOConfig::setProxyFor( "ftp", mData.proxyList["ftp"] );
  KSaveIOConfig::setProxyFor( "http", mData.proxyList["http"] );
  KSaveIOConfig::setProxyFor( "https", mData.proxyList["https"] );

  KSaveIOConfig::setProxyConfigScript( mData.proxyList["script"] );
  KSaveIOConfig::setUseReverseProxy( mData.useReverseProxy );
  KSaveIOConfig::setNoProxyFor( mData.noProxyFor.join(",") );


  KSaveIOConfig::updateRunningIOSlaves (this);
  if ( updateProxyScout )
    KSaveIOConfig::updateProxyScout( this );

  emit changed( false );
}

void KProxyDialog::defaults()
{
  mDefaultData = true;
  mUi.rbNoProxy->setChecked( true );
  mUi.location->lineEdit()->clear();
  mUi.cbPersConn->setChecked( false );
}

void KProxyDialog::setupManProxy()
{
  KManualProxyDlg dlgManual( this );

  dlgManual.setProxyData( mData );

  if ( dlgManual.exec() == QDialog::Accepted )
  {
    mData = dlgManual.data();
    mUi.rbManual->setChecked(true);
    emit changed( true );
  }
}

void KProxyDialog::setupEnvProxy()
{
  KEnvVarProxyDlg dlgEnv( this );

  dlgEnv.setProxyData( mData );

  if ( dlgEnv.exec() == QDialog::Accepted )
  {
    mData = dlgEnv.data();
    mUi.rbEnvVar->setChecked(true);
    emit changed( true );
  }
}

void KProxyDialog::slotChanged()
{
  mDefaultData = false;
  emit changed( true );
}

void KProxyDialog::slotUseProxyChanged()
{
  mDefaultData = false;
  bool useProxy = !(mUi.rbNoProxy->isChecked());
  mUi.gbAuth->setEnabled(useProxy);
  mUi.gbOptions->setEnabled(useProxy);
  emit changed( true );
}

QString KProxyDialog::quickHelp() const
{
  return i18n( "<h1>Proxy</h1>"
               "<p>A proxy server is an intermediate program that sits between "
               "your machine and the Internet and provides services such as "
               "web page caching and/or filtering.</p>"
               "<p>Caching proxy servers give you faster access to sites you have "
               "already visited by locally storing or caching the content of those "
               "pages; filtering proxy servers, on the other hand, provide the "
               "ability to block out requests for ads, spam, or anything else you "
               "want to block.</p>"
               "<p><u>Note:</u> Some proxy servers provide both services.</p>" );
}

void KProxyDialog::showInvalidMessage( const QString& _msg )
{
  QString msg;

  if( !_msg.isEmpty() )
    msg = _msg;
  else
    msg = i18n( "<qt>The proxy settings you specified are invalid."
                "<br /><br />Please click on the <b>Setup...</b> "
                "button and correct the problem before proceeding; "
                "otherwise your changes will be ignored.</qt>" );

  KMessageBox::error( this, msg, i18n("Invalid Proxy Setup") );
}

#include "kproxydlg.moc"
