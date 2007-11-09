/*
   Original Authors:
   Copyright (c) Kalle Dalheimer <kalle@kde.org> 1997
   Copyright (c) David Faure <faure@kde.org> 1998
   Copyright (c) Dirk Mueller <mueller@kde.org> 2000

   Completely re-written by:
   Copyright (C) 2000- Dawit Alemayehu <adawit@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License (GPL)
   version 2 as published by the Free Software Foundation.

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
#include "useragentdlg.h"

// Local
#include "ksaveioconfig.h"
#include "useragentinfo.h"
#include "uagentproviderdlg.h"

// Qt
#include <QtGui/QLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QBoxLayout>
#include <QtGui/QTreeWidget>

// KDE
#include <kdebug.h>
#include <kconfig.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kio/http_slave_defaults.h>
#include <kgenericfactory.h>


K_PLUGIN_FACTORY_DECLARATION(KioConfigFactory)

typedef QList<QTreeWidgetItem*> SiteList;
typedef SiteList::iterator SiteListIterator;

UserAgentDlg::UserAgentDlg(QWidget *parent, const QVariantList &)
             :KCModule(KioConfigFactory::componentData(), parent),
              m_userAgentInfo(0),
              m_config(0)
{
  ui.setupUi(this);
  load();
}

UserAgentDlg::~UserAgentDlg()
{
    delete m_userAgentInfo;
    delete m_config;
}

void UserAgentDlg::on_sendUACheckBox_clicked()
{
  configChanged();
}

void UserAgentDlg::on_newButton_clicked()
{
  UserAgentConfigDlg pdlg (i18n("Add Identification"), m_userAgentInfo, this );

  if ( pdlg.exec() == QDialog::Accepted )
  {
    if ( !handleDuplicate( pdlg.siteName(), pdlg.identity(), pdlg.alias() ) )
    {
      QTreeWidgetItem* item = new QTreeWidgetItem( ui.sitePolicyTreeWidget);
      item->setText(0, pdlg.siteName());
      item->setText(1, pdlg.identity());
      item->setText(2, pdlg.alias());
      ui.sitePolicyTreeWidget->setCurrentItem( item );
      configChanged();
    }
  }
}

void UserAgentDlg::on_changeButton_clicked()
{
  on_sitePolicyTreeWidget_itemActivated(ui.sitePolicyTreeWidget->currentItem(), -1);
}

void UserAgentDlg::on_deleteButton_clicked()
{
  SiteList selectedItems = ui.sitePolicyTreeWidget->selectedItems();
  SiteListIterator endIt = selectedItems.end();

  QString siteName;
  for(SiteListIterator it = selectedItems.begin(); it != endIt; ++it)
    delete (*it);

  updateButtons();
  configChanged();
}

void UserAgentDlg::on_deleteAllButton_clicked()
{
  ui.sitePolicyTreeWidget->clear();
  updateButtons();
  configChanged();
}

void UserAgentDlg::on_osNameCheckBox_clicked()
{
  changeDefaultUAModifiers();
}

void UserAgentDlg::on_osVersionCheckBox_clicked()
{
  changeDefaultUAModifiers();
}

void UserAgentDlg::on_platformCheckBox_clicked()
{
  changeDefaultUAModifiers();
}

void UserAgentDlg::on_processorTypeCheckBox_clicked()
{
  changeDefaultUAModifiers();
}

void UserAgentDlg::on_languageCheckBox_clicked()
{
  changeDefaultUAModifiers();
}

void UserAgentDlg::on_sitePolicyTreeWidget_itemActivated(QTreeWidgetItem* item, int)
{
  if(item)
  {
    // Store the current site name...
    const QString currentSiteName = item->text(0);

    UserAgentConfigDlg pdlg ( i18n("Modify Identification"), m_userAgentInfo, this );
    pdlg.setSiteName( currentSiteName );
    pdlg.setIdentity( item->text(1) );

    if ( pdlg.exec() == QDialog::Accepted )
    {
      if ( pdlg.siteName() == currentSiteName ||
          !handleDuplicate( pdlg.siteName(), pdlg.identity(), pdlg.alias() ) )
      {
        item->setText( 0, pdlg.siteName() );
        item->setText( 1, pdlg.identity() );
        item->setText( 2, pdlg.alias() );
        configChanged();
      }
    }
  }
}

void UserAgentDlg::changeDefaultUAModifiers()
{
  m_ua_keys = ":"; // Make sure it's not empty

  if ( ui.osNameCheckBox->isChecked() )
     m_ua_keys += 'o';

  if ( ui.osVersionCheckBox->isChecked() )
     m_ua_keys += 'v';

  if ( ui.platformCheckBox->isChecked() )
     m_ua_keys += 'p';

  if ( ui.processorTypeCheckBox->isChecked() )
     m_ua_keys += 'm';

  if ( ui.languageCheckBox->isChecked() )
     m_ua_keys += 'l';

  ui.osVersionCheckBox->setEnabled(m_ua_keys.contains('o'));

  QString modVal = KProtocolManager::defaultUserAgent( m_ua_keys );
  if ( ui.defaultIdLineEdit->text() != modVal )
  {
    ui.defaultIdLineEdit->setText(modVal);
    configChanged();
  }
}

bool UserAgentDlg::handleDuplicate( const QString& site,
                                    const QString& identity,
                                    const QString& alias )
{
  SiteList list = ui.sitePolicyTreeWidget->findItems(site, Qt::MatchExactly, 0);

  if (!list.isEmpty())
  {
    QString msg = i18n("<qt><center>Found an existing identification for"
                        "<br/><b>%1</b><br/>"
                        "Do you want to replace it?</center>"
                        "</qt>", site);
    int res = KMessageBox::warningContinueCancel(this, msg,
                                        i18n("Duplicate Identification"),
                                        KGuiItem(i18n("Replace")));
    if ( res == KMessageBox::Continue )
    {
      list[0]->setText(0, site);
      list[0]->setText(1, identity);
      list[0]->setText(2, alias);
      configChanged();
    }

    return true;
  }

  return false;
}

void UserAgentDlg::configChanged(bool enable)
{
  emit changed(enable);
}

void UserAgentDlg::updateButtons()
{
  const int selectedItemCount = ui.sitePolicyTreeWidget->selectedItems().count();
  const bool hasItems = ui.sitePolicyTreeWidget->topLevelItemCount() > 0;

  ui.changeButton->setEnabled ((hasItems && selectedItemCount == 1));
  ui.deleteButton->setEnabled ((hasItems && selectedItemCount > 0));
  ui.deleteAllButton->setEnabled ( hasItems );
}

void UserAgentDlg::on_sitePolicyTreeWidget_itemSelectionChanged()
{
  updateButtons();
}

void UserAgentDlg::load()
{
  ui.sitePolicyTreeWidget->clear();

  if (!m_config)
    m_config = new KConfig("kio_httprc", KConfig::NoGlobals);
  else
    m_config->reparseConfiguration();

  if (!m_userAgentInfo)
    m_userAgentInfo = new UserAgentInfo();

  QStringList list = m_config->groupList();
  QStringList::ConstIterator endIt = list.end();
  QString agentStr;

  for ( QStringList::Iterator it = list.begin(); it != endIt; ++it )
  {
    if ( (*it) == "<default>")
        continue;

    KConfigGroup cg(m_config, *it);
    agentStr = cg.readEntry("UserAgent");
    if (!agentStr.isEmpty())
    {
      QTreeWidgetItem* item = new QTreeWidgetItem(ui.sitePolicyTreeWidget);
      item->setText(0, (*it).toLower());
      item->setText(1, m_userAgentInfo->aliasStr(agentStr));
      item->setText(2, agentStr);
    }
  }

  // Update buttons and checkboxes...
  KConfigGroup cg2(m_config, QString());
  bool b = cg2.readEntry("SendUserAgent", true);
  ui.sendUACheckBox->setChecked( b );
  m_ua_keys = cg2.readEntry("UserAgentKeys", DEFAULT_USER_AGENT_KEYS).toLower();
  ui.defaultIdLineEdit->setText( KProtocolManager::defaultUserAgent( m_ua_keys ) );
  ui.osNameCheckBox->setChecked( m_ua_keys.contains('o') );
  ui.osVersionCheckBox->setChecked( m_ua_keys.contains('v') );
  ui.platformCheckBox->setChecked( m_ua_keys.contains('p') );
  ui.processorTypeCheckBox->setChecked( m_ua_keys.contains('m') );
  ui.languageCheckBox->setChecked( m_ua_keys.contains('l') );

  updateButtons();
  configChanged(false);
}

void UserAgentDlg::defaults()
{
  ui.sitePolicyTreeWidget->clear();
  m_ua_keys = DEFAULT_USER_AGENT_KEYS;
  ui.defaultIdLineEdit->setText( KProtocolManager::defaultUserAgent(m_ua_keys) );
  ui.osNameCheckBox->setChecked( m_ua_keys.contains('o') );
  ui.osVersionCheckBox->setChecked( m_ua_keys.contains('v') );
  ui.platformCheckBox->setChecked( m_ua_keys.contains('p') );
  ui.processorTypeCheckBox->setChecked( m_ua_keys.contains('m') );
  ui.languageCheckBox->setChecked( m_ua_keys.contains('l') );
  ui.sendUACheckBox->setChecked( true );

  updateButtons();
  configChanged();
}

void UserAgentDlg::save()
{
  Q_ASSERT(m_config);

  // Put all the groups except the default into the delete list.
  QStringList deleteList = m_config->groupList();

  //Remove all the groups that DO NOT contain a "UserAgent" entry...
  QStringList::ConstIterator endIt = deleteList.end();
  for (QStringList::ConstIterator it = deleteList.begin(); it != endIt; ++it)
  {
    if ( (*it) == QLatin1String("<default>") )
      continue;

    KConfigGroup cg(m_config, *it);
    if (!cg.hasKey("UserAgent"))
      deleteList.remove(*it);
  }

  QString domain;
  QTreeWidgetItem* item;
  int itemCount = ui.sitePolicyTreeWidget->topLevelItemCount();

  // Save and remove from the delete list all the groups that were 
  // not deleted by the end user.
  for(int i = 0; i < itemCount; i++)
  {
    item = ui.sitePolicyTreeWidget->topLevelItem(i);
    domain = item->text(0);
    KConfigGroup cg(m_config, domain);
    cg.writeEntry("UserAgent", item->text(2));
    deleteList.removeAll(domain);
    qDebug("UserAgentDlg::save: Removed [%s] from delete list", domain.toLatin1().constData());
  }

  // Write the global configuration information...
  KConfigGroup cg(m_config, QString());
  cg.writeEntry("SendUserAgent", ui.sendUACheckBox->isChecked());
  cg.writeEntry("UserAgentKeys", m_ua_keys );

  // Sync up all the changes so far...
  m_config->sync();

  // If delete list is not empty, delete the specified domains.
  if (!deleteList.isEmpty())
  {
    // Remove entries from local file.
    endIt = deleteList.end();
    KConfig cfg ("kio_httprc", KConfig::SimpleConfig);

    for ( QStringList::ConstIterator it = deleteList.begin(); it != endIt; ++it )
    {
      KConfigGroup cg(&cfg, *it);
      cg.deleteEntry("UserAgent");
      qDebug("UserAgentDlg::save: Deleting UserAgent of group [%s]", (*it).toLatin1().constData());
      if (cg.keyList().count() < 1)
        cg.deleteGroup();
    }

    // Sync up the configuration...
    cfg.sync();

    // Check everything is gone, reset to blank otherwise.
    m_config->reparseConfiguration();
    endIt = deleteList.end();
    for (QStringList::ConstIterator it = deleteList.begin(); it != endIt; ++it )
    {
      KConfigGroup cg(m_config, *it);
      if (cg.hasKey("UserAgent"))
        cg.writeEntry("UserAgent", QString());
    }

    // Sync up the configuration...
    m_config->sync();
  }

  KSaveIOConfig::updateRunningIOSlaves (this);
  configChanged( false );
}

QString UserAgentDlg::quickHelp() const
{
  return i18n( "<p><h1>Browser Identification</h1> "
               "The browser-identification module allows you to have full "
               "control over how Konqueror will identify itself to web "
               "sites you browse.</p>"
               "<p>This ability to fake identification is necessary because "
               "some web sites do not display properly when they detect that "
               "they are not talking to current versions of either Netscape "
               "Navigator or Internet Explorer, even if the browser actually "
               "supports all the necessary features to render those pages "
               "properly. "
               "For such sites, you can use this feature to try to browse "
               "them. Please understand that this might not always work, since "
               "such sites might be using non-standard web protocols and or "
               "specifications.</p>"
               "<p><u>NOTE:</u> To obtain specific help on a particular section "
               "of the dialog box, simply click on the quick help button on "
               "the window title bar, then click on the section "
               "for which you are seeking help.</p>" );
}

#include "useragentdlg.moc"
