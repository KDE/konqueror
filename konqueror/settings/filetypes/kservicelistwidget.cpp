/* This file is part of the KDE project
   Copyright (C) 2003 Waldo Bastian <bastian@kde.org>
   Copyright (C) 2003 David Faure <faure@kde.org>
   Copyright (C) 2002 Daniel Molkentin <molkentin@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

// Own
#include "kservicelistwidget.h"

// std
#include <unistd.h>

// Qt
#include <QtGui/QPushButton>
#include <QtGui/QLayout>
#include <QtGui/QGridLayout>

// KDE
#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <knotification.h>
#include <kopenwithdialog.h>
#include <kpropertiesdialog.h>
#include <kstandarddirs.h>

// Local
#include "kserviceselectdlg.h"
#include "mimetypedata.h"

KServiceListItem::KServiceListItem( KService::Ptr pService, int kind )
    : QListWidgetItem(), desktopPath(pService->entryPath())
{
    if ( kind == KServiceListWidget::SERVICELIST_APPLICATIONS )
        setText( pService->name() );
    else
        setText( i18n( "%1 (%2)", pService->name(), pService->desktopEntryName() ) );

    if (!pService->isApplication())
      localPath = KStandardDirs::locateLocal("services", desktopPath);
    else
      localPath = pService->locateLocal();
}

bool KServiceListItem::isImmutable()
{
    return !KStandardDirs::checkAccess(localPath, W_OK);
}

KServiceListWidget::KServiceListWidget(int kind, QWidget *parent, const char *name)
  : Q3GroupBox( kind == SERVICELIST_APPLICATIONS ? i18n("Application Preference Order")
               : i18n("Services Preference Order"), parent, name ),
    m_kind( kind ), m_mimeTypeData( 0L )
{
  QWidget * gb = this;
  QGridLayout * grid = new QGridLayout(gb);
  grid->setMargin(KDialog::marginHint());
  grid->setSpacing(KDialog::spacingHint());

  servicesLB = new QListWidget(gb);
  connect(servicesLB, SIGNAL(itemSelectionChanged()), SLOT(enableMoveButtons()));
  grid->addWidget(servicesLB, 0, 0, 6, 1);
  connect( servicesLB, SIGNAL( itemDoubleClicked(QListWidgetItem*)), this, SLOT( editService()));

  QString wtstr =
    (kind == SERVICELIST_APPLICATIONS ?
     i18n("This is a list of applications associated with files of the selected"
          " file type. This list is shown in Konqueror's context menus when you select"
          " \"Open With...\". If more than one application is associated with this file type,"
          " then the list is ordered by priority with the uppermost item taking precedence"
          " over the others.") :
     i18n("This is a list of services associated with files of the selected"
          " file type. This list is shown in Konqueror's context menus when you select"
          " a \"Preview with...\" option. If more than one application is associated with this file type,"
          " then the list is ordered by priority with the uppermost item taking precedence"
          " over the others."));

  gb->setWhatsThis( wtstr );
  servicesLB->setWhatsThis( wtstr );

  servUpButton = new QPushButton(i18n("Move &Up"), gb);
  servUpButton->setEnabled(false);
  connect(servUpButton, SIGNAL(clicked()), SLOT(promoteService()));
  grid->addWidget(servUpButton, 1, 1);

  servUpButton->setWhatsThis( kind == SERVICELIST_APPLICATIONS ?
                   i18n("Assigns a higher priority to the selected\n"
                        "application, moving it up in the list. Note:  This\n"
                        "only affects the selected application if the file type is\n"
                        "associated with more than one application.") :
                   i18n("Assigns a higher priority to the selected\n"
                        "service, moving it up in the list."));

  servDownButton = new QPushButton(i18n("Move &Down"), gb);
  servDownButton->setEnabled(false);
  connect(servDownButton, SIGNAL(clicked()), SLOT(demoteService()));
  grid->addWidget(servDownButton, 2, 1);

  servDownButton->setWhatsThis( kind == SERVICELIST_APPLICATIONS ?
                   i18n("Assigns a lower priority to the selected\n"
                        "application, moving it down in the list. Note: This \n"
                        "only affects the selected application if the file type is\n"
                        "associated with more than one application."):
                   i18n("Assigns a lower priority to the selected\n"
                        "service, moving it down in the list."));

  servNewButton = new QPushButton(i18n("Add..."), gb);
  servNewButton->setEnabled(false);
  connect(servNewButton, SIGNAL(clicked()), SLOT(addService()));
  grid->addWidget(servNewButton, 0, 1);

  servNewButton->setWhatsThis( i18n( "Add a new application for this file type." ) );


  servEditButton = new QPushButton(i18n("Edit..."), gb);
  servEditButton->setEnabled(false);
  connect(servEditButton, SIGNAL(clicked()), SLOT(editService()));
  grid->addWidget(servEditButton, 3, 1);

  servEditButton->setWhatsThis( i18n( "Edit command line of the selected application." ) );


  servRemoveButton = new QPushButton(i18n("Remove"), gb);
  servRemoveButton->setEnabled(false);
  connect(servRemoveButton, SIGNAL(clicked()), SLOT(removeService()));
  grid->addWidget(servRemoveButton, 4, 1);

  servRemoveButton->setWhatsThis( i18n( "Remove the selected application from the list." ) );

  grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding), 5, 1);
}

void KServiceListWidget::setMimeTypeData( MimeTypeData * mimeTypeData )
{
  m_mimeTypeData = mimeTypeData;
  if ( servNewButton )
    servNewButton->setEnabled(true);
  // will need a selection
  servUpButton->setEnabled(false);
  servDownButton->setEnabled(false);

  if ( servRemoveButton )
    servRemoveButton->setEnabled(false);
  if ( servEditButton )
    servEditButton->setEnabled(false);

  servicesLB->clear();
  servicesLB->setEnabled(false);

  if ( m_mimeTypeData )
  {
    QStringList services = ( m_kind == SERVICELIST_APPLICATIONS )
      ? m_mimeTypeData->appServices()
      : m_mimeTypeData->embedServices();

    if (services.count() == 0) {
      servicesLB->addItem(i18n("None"));
    } else {
      for ( QStringList::Iterator it = services.begin();
            it != services.end(); it++ )
      {
        KService::Ptr pService = KService::serviceByDesktopPath( *it );

        if (pService)
          servicesLB->addItem( new KServiceListItem(pService, m_kind) );
      }
      servicesLB->setEnabled(true);
    }
  }
}

void KServiceListWidget::promoteService()
{
  if (!servicesLB->isEnabled()) {
    KNotification::beep();
    return;
  }

  int selIndex = servicesLB->currentRow();
  if (selIndex == 0) {
    KNotification::beep();
    return;
  }

  QListWidgetItem *selItem = servicesLB->item(selIndex);
  servicesLB->takeItem(selIndex);
  servicesLB->insertItem(selIndex-1,selItem);
  servicesLB->setCurrentRow(selIndex - 1);

  updatePreferredServices();

  emit changed(true);
}

void KServiceListWidget::demoteService()
{
  if (!servicesLB->isEnabled()) {
    KNotification::beep();
    return;
  }

  int selIndex = servicesLB->currentRow();
  if (selIndex == servicesLB->count() - 1) {
    KNotification::beep();
    return;
  }

  QListWidgetItem *selItem = servicesLB->item(selIndex);
  servicesLB->takeItem(selIndex);
  servicesLB->insertItem(selIndex + 1, selItem);
  servicesLB->setCurrentRow(selIndex + 1);

  updatePreferredServices();

  emit changed(true);
}

void KServiceListWidget::addService()
{
  if (!m_mimeTypeData)
      return;

  KService::Ptr service;
  if ( m_kind == SERVICELIST_APPLICATIONS )
  {
      KOpenWithDialog dlg(m_mimeTypeData->name(), QString(), this);
      dlg.setSaveNewApplications(true);
      if (dlg.exec() != QDialog::Accepted)
          return;

      service = dlg.service();

      Q_ASSERT(service);
      if (!service)
          return; // Don't crash if KOpenWith wasn't able to create service.
  }
  else
  {
      KServiceSelectDlg dlg(m_mimeTypeData->name(), QString(), this);
      if (dlg.exec() != QDialog::Accepted)
          return;
       service = dlg.service();
       Q_ASSERT(service);
       if (!service)
           return;
  }

  // if None is the only item, then there currently is no default
  if (servicesLB->item(0)->text() == i18n("None")) {
      delete servicesLB->takeItem(0);
      servicesLB->setEnabled(true);
  }
  else
  {
      // check if it is a duplicate entry
      for (int index = 0; index < servicesLB->count(); index++)
        if (static_cast<KServiceListItem*>( servicesLB->item(index) )->desktopPath
            == service->entryPath())
          return;
  }

  servicesLB->insertItem( 0 , new KServiceListItem(service, m_kind) );
  servicesLB->setCurrentItem(0);

  updatePreferredServices();

  emit changed(true);
}

void KServiceListWidget::editService()
{
  if (!m_mimeTypeData)
      return;
  int selected = servicesLB->currentRow();
  if ( selected >= 0 ) {

    // Only edit applications, not services as
    // they don't have any parameters
    if ( m_kind == SERVICELIST_APPLICATIONS )
    {
      // Just like popping up an add dialog except that we
      // pass the current command line as a default
      QListWidgetItem *selItem = servicesLB->item(selected);

      KService::Ptr service = KService::serviceByDesktopPath(
          ((KServiceListItem*)selItem)->desktopPath );
      if (!service)
        return;

      QString path = service->entryPath();

      // If the path to the desktop file is relative, try to get the full
      // path from KStdDirs.
      path = KStandardDirs::locate("apps", path);
      KUrl serviceURL;
      serviceURL.setPath( path );
      KFileItem item( serviceURL, "application/x-desktop", KFileItem::Unknown );
      KPropertiesDialog dlg( item, this );
      if ( dlg.exec() != QDialog::Accepted )
        return;

      // Reload service
      service = KService::serviceByDesktopPath(
          ((KServiceListItem*)selItem)->desktopPath );
      if (!service)
        return;

      // Remove the old one...
      delete servicesLB->takeItem( selected );

      // ...check that it's not a duplicate entry...
      bool addIt = true;
      for ( int index = 0; index < servicesLB->count(); index++ ) {
        if (static_cast<KServiceListItem*>( servicesLB->item(index) )->desktopPath
                == service->entryPath()) {
          addIt = false;
          break;
        }
      }

      // ...and add it in the same place as the old one:
      if ( addIt ) {
        servicesLB->insertItem( selected , new KServiceListItem(service, m_kind) );
        servicesLB->setCurrentRow(selected);
      }

      updatePreferredServices();

      emit changed(true);
    }
  }
}

void KServiceListWidget::removeService()
{
  if (!m_mimeTypeData) return;

  int selected = servicesLB->currentRow();

  if ( selected >= 0 ) {
    // Check if service is associated with this mimetype or with one of its parents
    KServiceListItem *serviceItem = static_cast<KServiceListItem *>(servicesLB->item(selected));
    KMimeType::Ptr mimetype = m_mimeTypeData->findImplicitAssociation(serviceItem->desktopPath);
    if (serviceItem->isImmutable())
    {
       KMessageBox::sorry(this, i18n("You are not authorized to remove this service."));
    }
    else if (mimetype)
    {
       KMessageBox::sorry(this, "<qt>"+
                                i18n("The service <b>%1</b> can not be removed.", serviceItem->text())+
                                "<p>"+
                                i18n("The service is listed here because it has been associated "
                                     "with the <b>%1</b> (%2) file type and files of type "
                                     "<b>%3</b> (%4) are per definition also of type "
                                     "<b>%5</b>.", mimetype->name(), mimetype->comment(),
                                     m_mimeTypeData->name(), m_mimeTypeData->comment(), mimetype->name())+
                                "<p>"+
                                i18n("Either select the <b>%1</b> file type to remove the "
                                     "service from there or move the service down "
                                     "to deprecate it.", mimetype->name()));

                                // i18n("Do you want to remove the service from the <b>%1</b> "
                                //      "file type or from the <b>%2</b> file type?", ???, ???);
    }
    else
    {
       delete servicesLB->takeItem( selected );
       updatePreferredServices();

       emit changed(true);
    }
  }

  if ( servRemoveButton && servicesLB->currentRow() == -1 )
    servRemoveButton->setEnabled(false);

  if ( servEditButton && servicesLB->currentRow() == -1 )
    servEditButton->setEnabled(false);
}

void KServiceListWidget::updatePreferredServices()
{
  if (!m_mimeTypeData)
    return;
  QStringList sl;
  unsigned int count = servicesLB->count();

  for (unsigned int i = 0; i < count; i++) {
    KServiceListItem *sli = (KServiceListItem *) servicesLB->item(i);
    sl.append( sli->desktopPath );
  }
  if ( m_kind == SERVICELIST_APPLICATIONS )
    m_mimeTypeData->setAppServices(sl);
  else
    m_mimeTypeData->setEmbedServices(sl);
}

void KServiceListWidget::enableMoveButtons()
{
  int idx = servicesLB->currentRow();
  if (servicesLB->model()->rowCount() <= 1)
  {
    servUpButton->setEnabled(false);
    servDownButton->setEnabled(false);
  }
  else if ( idx == (servicesLB->model()->rowCount() - 1) )
  {
    servUpButton->setEnabled(true);
    servDownButton->setEnabled(false);
  }
  else if (idx == 0)
  {
    servUpButton->setEnabled(false);
    servDownButton->setEnabled(true);
  }
  else
  {
    servUpButton->setEnabled(true);
    servDownButton->setEnabled(true);
  }

  if ( servRemoveButton )
    servRemoveButton->setEnabled(true);

  if ( servEditButton )
    servEditButton->setEnabled(true && ( m_kind == SERVICELIST_APPLICATIONS ) );
}

#include "kservicelistwidget.moc"
