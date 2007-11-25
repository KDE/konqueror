/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include "konqprofiledlg.h"
#include "konqviewmanager.h"
#include "konqsettingsxt.h"

#include <QtGui/QCheckBox>
#include <QtCore/QDir>
#include <kvbox.h>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>

#include <kdebug.h>
#include <kstandardguiitem.h>
#include <kio/global.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kconfig.h>
#include <kseparator.h>
#include <klistwidget.h>
#include <kpushbutton.h>

KonqProfileMap KonqProfileDlg::readAllProfiles()
{
  KonqProfileMap mapProfiles;

  QStringList profiles = KGlobal::dirs()->findAllResources( "data", "konqueror/profiles/*", KStandardDirs::NoDuplicates );
  QStringList::ConstIterator pIt = profiles.begin();
  QStringList::ConstIterator pEnd = profiles.end();
  for (; pIt != pEnd; ++pIt )
  {
    QFileInfo info( *pIt );
    QString profileName = KIO::decodeFileName( info.baseName() );
    KConfig cfg( *pIt, KConfig::SimpleConfig);
    if ( cfg.hasGroup( "Profile" ) )
    {
      KConfigGroup profileGroup( &cfg, "Profile" );
      if ( profileGroup.hasKey( "Name" ) )
        profileName = profileGroup.readEntry( "Name" );

      mapProfiles.insert( profileName, *pIt );
    }
  }

  return mapProfiles;
}

KonqProfileItem::KonqProfileItem( KListWidget *parent, const QString & text )
    : QListWidgetItem( text, parent ), m_profileName( text )
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

#define BTN_RENAME KDialog::User1
#define BTN_DELETE KDialog::User2
#define BTN_SAVE   KDialog::User3

KonqProfileDlg::KonqProfileDlg( KonqViewManager *manager, const QString & preselectProfile, QWidget *parent )
: KDialog( parent )
{
  setObjectName( "konq_profile_dialog" );
  setModal( true );
  setCaption( i18n( "Profile Management" ) );
  setButtons( Close | BTN_RENAME | BTN_DELETE | BTN_SAVE );
  setDefaultButton( BTN_SAVE );
  showButtonSeparator( true );
  setButtonGuiItem( BTN_RENAME, KGuiItem( i18n( "&Rename Profile" ) ) );
  setButtonGuiItem( BTN_DELETE, KGuiItem( i18n( "&Delete Profile" ), "edit-delete" ) );
  setButtonGuiItem( BTN_SAVE, KStandardGuiItem::save() );

  m_pViewManager = manager;

  KVBox* box = new KVBox( this );
  box->setSpacing( KDialog::spacingHint() );
  setMainWidget( box );

  QLabel *lblName = new QLabel( i18n(  "&Profile name:" ), box );

  m_pProfileNameLineEdit = new QLineEdit( box );
  m_pProfileNameLineEdit->setFocus();

  lblName->setBuddy( m_pProfileNameLineEdit );

  m_pListView = new KListWidget( box );
  m_pListView->setSelectionBehavior(QAbstractItemView::SelectItems);
  m_pListView->setSelectionMode(QAbstractItemView::SingleSelection);

  box->setStretchFactor( m_pListView, 1 );

  connect( m_pListView, SIGNAL( itemChanged( QListWidgetItem * ) ),
            SLOT( slotItemRenamed( QListWidgetItem * ) ) );

  loadAllProfiles( preselectProfile );
  m_pListView->setMinimumSize( m_pListView->sizeHint() );

  m_cbSaveURLs = new QCheckBox( i18n("Save &URLs in profile"), box );
  m_cbSaveURLs->setChecked( KonqSettings::saveURLInProfile() );

  m_cbSaveSize = new QCheckBox( i18n("Save &window size in profile"), box );
  m_cbSaveSize->setChecked( KonqSettings::saveWindowSizeInProfile() );

  connect( m_pListView, SIGNAL( itemSelectionChanged() ),
           this, SLOT( slotSelectionChanged() ) );

  connect( m_pProfileNameLineEdit, SIGNAL( textChanged( const QString & ) ),
           this, SLOT( slotTextChanged( const QString & ) ) );

  enableButton( BTN_RENAME, m_pListView->currentItem() != 0 );
  enableButton( BTN_DELETE, m_pListView->currentItem() != 0 );

  connect( this,SIGNAL(user1Clicked()),SLOT(slotRenameProfile()));
  connect( this,SIGNAL(user2Clicked()),SLOT(slotDeleteProfile()));
  connect( this,SIGNAL(user3Clicked()),SLOT(slotSave()));

  resize( sizeHint() );
}

KonqProfileDlg::~KonqProfileDlg()
{
  KonqSettings::setSaveURLInProfile( m_cbSaveURLs->isChecked() );
  KonqSettings::setSaveWindowSizeInProfile( m_cbSaveSize->isChecked() );
}

void KonqProfileDlg::loadAllProfiles(const QString & preselectProfile)
{
    bool profileFound = false;
    m_mapEntries.clear();
    m_pListView->clear();
    m_mapEntries = readAllProfiles();
    KonqProfileMap::ConstIterator eIt = m_mapEntries.begin();
    KonqProfileMap::ConstIterator eEnd = m_mapEntries.end();
    for (; eIt != eEnd; ++eIt )
    {
        QListWidgetItem *item = new KonqProfileItem( m_pListView, eIt.key() );
        QString filename = eIt.value().mid( eIt.value().lastIndexOf( '/' ) + 1 );
        kDebug(1202) << filename;
        if ( filename == preselectProfile )
        {
            profileFound = true;
            m_pProfileNameLineEdit->setText( eIt.key() );
            m_pListView->setCurrentItem( item );
        }
    }
    if (!profileFound)
        m_pProfileNameLineEdit->setText( preselectProfile);
}

void KonqProfileDlg::slotSave()
{
  QString name = KIO::encodeFileName( m_pProfileNameLineEdit->text() ); // in case of '/'

  // Reuse filename of existing item, if any
  if ( m_pListView->currentItem() )
  {
    KonqProfileMap::Iterator it = m_mapEntries.find( m_pListView->currentItem()->text() );
    if ( it != m_mapEntries.end() )
    {
      QFileInfo info( it.value() );
      name = info.baseName();
    }
  }

  kDebug(1202) << "Saving as " << name;
  m_pViewManager->saveViewProfileToFile( name, m_pProfileNameLineEdit->text(),
            m_cbSaveURLs->isChecked(), m_cbSaveSize->isChecked() );

  accept();
}

void KonqProfileDlg::slotDeleteProfile()
{
    if(!m_pListView->currentItem())
        return;
  KonqProfileMap::Iterator it = m_mapEntries.find( m_pListView->currentItem()->text() );

  if ( it != m_mapEntries.end() && QFile::remove( it.value() ) )
      loadAllProfiles();

  enableButton( BTN_RENAME, m_pListView->currentItem() != 0 );
  enableButton( BTN_DELETE, m_pListView->currentItem() != 0 );
}

void KonqProfileDlg::slotRenameProfile()
{
  QListWidgetItem *item = m_pListView->currentItem();

  if ( item )
    m_pListView->editItem( item );
}

void KonqProfileDlg::slotItemRenamed( QListWidgetItem * item )
{
  KonqProfileItem * profileItem = static_cast<KonqProfileItem *>( item );

  QString newName = profileItem->text();
  QString oldName = profileItem->m_profileName;

  if ( newName == oldName )
    return;

  if (!newName.isEmpty())
  {
    KonqProfileMap::ConstIterator it = m_mapEntries.find( oldName );

    if ( it != m_mapEntries.end() )
    {
      QString fileName = it.value();
      KConfig _cfg( fileName, KConfig::SimpleConfig );
      KConfigGroup cfg(&_cfg, "Profile" );
      cfg.writeEntry( "Name", newName );
      cfg.sync();
      // Didn't find how to change a key...
      m_mapEntries.remove( oldName );
      m_mapEntries.insert( newName, fileName );
      m_pProfileNameLineEdit->setText( newName );
      profileItem->m_profileName = newName;
    }
  }
}

void KonqProfileDlg::slotSelectionChanged()
{
  if ( m_pListView->currentItem() )
    m_pProfileNameLineEdit->setText( m_pListView->currentItem()->text() );
}

void KonqProfileDlg::slotTextChanged( const QString & text )
{
  enableButton( BTN_SAVE, !text.isEmpty() );

  // If we type the name of a profile, select it in the list

  QList<QListWidgetItem*> items = m_pListView->findItems(text, Qt::MatchCaseSensitive);
  QListWidgetItem * item = !items.isEmpty() ? items.first() : 0;
  m_pListView->setCurrentItem(item);

  bool itemSelected = item;
  if ( itemSelected )
  {
    QFileInfo fi( m_mapEntries[ item->text() ] );
    itemSelected = itemSelected && fi.isWritable();
    if ( itemSelected )
      item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable );
  }

  enableButton( BTN_RENAME, itemSelected );
  enableButton( BTN_DELETE, itemSelected );
}

#undef BTN_RENAME
#undef BTN_DELETE
#undef BTN_SAVE

#include "konqprofiledlg.moc"
