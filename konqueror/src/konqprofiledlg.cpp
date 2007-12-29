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
#include "ui_konqprofiledlg_base.h"

#include <QtGui/QCheckBox>
#include <QtCore/QDir>
#include <kvbox.h>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QListWidgetItem>

#include <kdebug.h>
#include <kstandardguiitem.h>
#include <kio/global.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kconfig.h>
#include <kseparator.h>
#include <klistwidget.h>
#include <kpushbutton.h>

class KonqProfileDlg::KonqProfileItem : public QListWidgetItem
{
public:
  KonqProfileItem( KListWidget *, const QString & );
  ~KonqProfileItem() {}

  QString m_profileName;
};


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

KonqProfileDlg::KonqProfileItem::KonqProfileItem( KListWidget *parent, const QString & text )
    : QListWidgetItem( text, parent ), m_profileName( text )
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

class KonqProfileDlg::KonqProfileDlgPrivate : public QWidget, public Ui::KonqProfileDlgBase
{
public:
  KonqProfileDlgPrivate( KonqViewManager *manager, QWidget *parent = 0 )
    : QWidget( parent )
    , m_pViewManager( manager )
  {
    setupUi( this );
  }

  KonqViewManager * const m_pViewManager;

  KonqProfileMap m_mapEntries;
};

#define BTN_RENAME KDialog::User1
#define BTN_DELETE KDialog::User2
#define BTN_SAVE   KDialog::User3

KonqProfileDlg::KonqProfileDlg( KonqViewManager *manager, const QString & preselectProfile, QWidget *parent )
: KDialog( parent )
, d( new KonqProfileDlgPrivate( manager, this ) )
{
  d->layout()->setMargin( 0 );
  setMainWidget( d );

  setObjectName( "konq_profile_dialog" );
  setModal( true );
  setCaption( i18n( "Profile Management" ) );
  setButtons( Close | BTN_RENAME | BTN_DELETE | BTN_SAVE );
  setDefaultButton( BTN_SAVE );
  showButtonSeparator( true );
  setButtonGuiItem( BTN_RENAME, KGuiItem( i18n( "&Rename Profile" ) ) );
  setButtonGuiItem( BTN_DELETE, KGuiItem( i18n( "&Delete Profile" ), "edit-delete" ) );
  setButtonGuiItem( BTN_SAVE, KStandardGuiItem::save() );

  d->m_pProfileNameLineEdit->setFocus();

  connect( d->m_pListView, SIGNAL( itemChanged( QListWidgetItem * ) ),
            SLOT( slotItemRenamed( QListWidgetItem * ) ) );

  loadAllProfiles( preselectProfile );
  d->m_pListView->setMinimumSize( d->m_pListView->sizeHint() );

  d->m_pHomeURLRequester->setPath( d->m_pViewManager->profileHomeURL() );
  d->m_pHomeURLRequester->setMode( KFile::Directory );
  d->m_cbSaveURLs->setChecked( KonqSettings::saveURLInProfile() );
  d->m_cbSaveSize->setChecked( KonqSettings::saveWindowSizeInProfile() );

  connect( d->m_pListView, SIGNAL( itemSelectionChanged() ),
           this, SLOT( slotSelectionChanged() ) );

  connect( d->m_pProfileNameLineEdit, SIGNAL( textChanged( const QString & ) ),
           this, SLOT( slotTextChanged( const QString & ) ) );

  enableButton( BTN_RENAME, d->m_pListView->currentItem() != 0 );
  enableButton( BTN_DELETE, d->m_pListView->currentItem() != 0 );

  connect( this,SIGNAL(user1Clicked()),SLOT(slotRenameProfile()));
  connect( this,SIGNAL(user2Clicked()),SLOT(slotDeleteProfile()));
  connect( this,SIGNAL(user3Clicked()),SLOT(slotSave()));

  resize( sizeHint() );
}

KonqProfileDlg::~KonqProfileDlg()
{
  KonqSettings::setSaveURLInProfile( d->m_cbSaveURLs->isChecked() );
  KonqSettings::setSaveWindowSizeInProfile( d->m_cbSaveSize->isChecked() );
}

void KonqProfileDlg::loadAllProfiles(const QString & preselectProfile)
{
    bool profileFound = false;
    d->m_mapEntries.clear();
    d->m_pListView->clear();
    d->m_mapEntries = readAllProfiles();
    KonqProfileMap::ConstIterator eIt = d->m_mapEntries.begin();
    KonqProfileMap::ConstIterator eEnd = d->m_mapEntries.end();
    for (; eIt != eEnd; ++eIt )
    {
        QListWidgetItem *item = new KonqProfileItem( d->m_pListView, eIt.key() );
        QString filename = eIt.value().mid( eIt.value().lastIndexOf( '/' ) + 1 );
        kDebug(1202) << filename;
        if ( filename == preselectProfile )
        {
            profileFound = true;
            d->m_pProfileNameLineEdit->setText( eIt.key() );
            d->m_pListView->setCurrentItem( item );
        }
    }
    if (!profileFound)
        d->m_pProfileNameLineEdit->setText( preselectProfile);

    slotTextChanged( d->m_pProfileNameLineEdit->text() );  // really disable save button if text empty
}

void KonqProfileDlg::slotSave()
{
  QString name = KIO::encodeFileName( d->m_pProfileNameLineEdit->text() ); // in case of '/'

  // Reuse filename of existing item, if any
  if ( d->m_pListView->currentItem() )
  {
    KonqProfileMap::Iterator it = d->m_mapEntries.find( d->m_pListView->currentItem()->text() );
    if ( it != d->m_mapEntries.end() )
    {
      QFileInfo info( it.value() );
      name = info.baseName();
    }
  }

  kDebug(1202) << "Saving as " << name;
  d->m_pViewManager->saveViewProfileToFile( name, d->m_pProfileNameLineEdit->text(), d->m_pHomeURLRequester->url(),
            d->m_cbSaveURLs->isChecked(), d->m_cbSaveSize->isChecked() );

  accept();
}

void KonqProfileDlg::slotDeleteProfile()
{
    if(!d->m_pListView->currentItem())
        return;
  KonqProfileMap::Iterator it = d->m_mapEntries.find( d->m_pListView->currentItem()->text() );

  if ( it != d->m_mapEntries.end() && QFile::remove( it.value() ) )
      loadAllProfiles();

  enableButton( BTN_RENAME, d->m_pListView->currentItem() != 0 );
  enableButton( BTN_DELETE, d->m_pListView->currentItem() != 0 );
}

void KonqProfileDlg::slotRenameProfile()
{
  QListWidgetItem *item = d->m_pListView->currentItem();

  if ( item )
    d->m_pListView->editItem( item );
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
    KonqProfileMap::ConstIterator it = d->m_mapEntries.find( oldName );

    if ( it != d->m_mapEntries.end() )
    {
      QString fileName = it.value();
      KConfig _cfg( fileName, KConfig::SimpleConfig );
      KConfigGroup cfg(&_cfg, "Profile" );
      cfg.writeEntry( "Name", newName );
      cfg.sync();
      // Didn't find how to change a key...
      d->m_mapEntries.remove( oldName );
      d->m_mapEntries.insert( newName, fileName );
      d->m_pProfileNameLineEdit->setText( newName );
      profileItem->m_profileName = newName;
    }
  }
}

void KonqProfileDlg::slotSelectionChanged()
{
  if ( d->m_pListView->currentItem() )
    d->m_pProfileNameLineEdit->setText( d->m_pListView->currentItem()->text() );
}

void KonqProfileDlg::slotTextChanged( const QString & text )
{
  enableButton( BTN_SAVE, !text.isEmpty() );

  // If we type the name of a profile, select it in the list

  QList<QListWidgetItem*> items = d->m_pListView->findItems(text, Qt::MatchCaseSensitive);
  QListWidgetItem * item = !items.isEmpty() ? items.first() : 0;
  d->m_pListView->setCurrentItem(item);

  bool itemSelected = item;
  if ( itemSelected )
  {
    KConfig config( d->m_mapEntries[text], KConfig::SimpleConfig );
    KConfigGroup profile( &config, "Profile" );

    d->m_pHomeURLRequester->setUrl( profile.readEntry( "HomeURL", QString() ) );

    QFileInfo fi( d->m_mapEntries[ item->text() ] );
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
