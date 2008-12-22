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
#include <QtGui/QLayout>
#include <QtGui/QHBoxLayout>

// KDE
#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <knotification.h>
#include <kopenwithdialog.h>
#include <kpropertiesdialog.h>
#include <kpushbutton.h>
#include <kstandarddirs.h>

// Local
#include "kserviceselectdlg.h"
#include "mimetypedata.h"

KServiceListItem::KServiceListItem( const KService::Ptr& pService, int kind )
    : QListWidgetItem(), storageId(pService->storageId()), desktopPath(pService->entryPath())
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

bool KServiceListItem::isImmutable() const
{
    return !KStandardDirs::checkAccess(localPath, W_OK);
}





KServiceListWidget::KServiceListWidget(int kind, QWidget *parent)
  : QGroupBox( kind == SERVICELIST_APPLICATIONS ? i18n("Application Preference Order")
               : i18n("Services Preference Order"), parent ),
    m_kind( kind ), m_mimeTypeData( 0L )
{
  QHBoxLayout *lay= new QHBoxLayout(this);
  lay->setMargin(KDialog::marginHint());
  lay->setSpacing(KDialog::spacingHint());

  servicesLB = new QListWidget(this);
  connect(servicesLB, SIGNAL(itemSelectionChanged()), SLOT(enableMoveButtons()));
  lay->addWidget(servicesLB);
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
          " a \"Preview with...\" option. If more than one service is associated with this file type,"
          " then the list is ordered by priority with the uppermost item taking precedence"
          " over the others."));

  setWhatsThis( wtstr );
  servicesLB->setWhatsThis( wtstr );

  QVBoxLayout *btnsLay= new QVBoxLayout();
  lay->addLayout(btnsLay);

  servUpButton = new KPushButton(i18n("Move &Up"), this);
  servUpButton->setIcon(KIcon("arrow-up"));
  servUpButton->setEnabled(false);
  connect(servUpButton, SIGNAL(clicked()), SLOT(promoteService()));
  btnsLay->addWidget(servUpButton);

  servUpButton->setWhatsThis( kind == SERVICELIST_APPLICATIONS ?
                   i18n("Assigns a higher priority to the selected\n"
                        "application, moving it up in the list. Note:  This\n"
                        "only affects the selected application if the file type is\n"
                        "associated with more than one application.") :
                   i18n("Assigns a higher priority to the selected\n"
                        "service, moving it up in the list."));

  servDownButton = new KPushButton(i18n("Move &Down"), this);
  servDownButton->setIcon(KIcon("arrow-down"));
  servDownButton->setEnabled(false);
  connect(servDownButton, SIGNAL(clicked()), SLOT(demoteService()));
  btnsLay->addWidget(servDownButton);
  servDownButton->setWhatsThis( kind == SERVICELIST_APPLICATIONS ?
                   i18n("Assigns a lower priority to the selected\n"
                        "application, moving it down in the list. Note: This \n"
                        "only affects the selected application if the file type is\n"
                        "associated with more than one application."):
                   i18n("Assigns a lower priority to the selected\n"
                        "service, moving it down in the list."));

  servNewButton = new KPushButton(i18n("Add..."), this);
  servNewButton->setIcon(KIcon("list-add"));
  servNewButton->setEnabled(false);
  connect(servNewButton, SIGNAL(clicked()), SLOT(addService()));
  btnsLay->addWidget(servNewButton);
  servNewButton->setWhatsThis( i18n( "Add a new application for this file type." ) );


  servEditButton = new KPushButton(i18n("Edit..."), this);
  servEditButton->setIcon(KIcon("edit-rename"));
  servEditButton->setEnabled(false);
  connect(servEditButton, SIGNAL(clicked()), SLOT(editService()));
  btnsLay->addWidget(servEditButton);
  servEditButton->setWhatsThis( i18n( "Edit command line of the selected application." ) );


  servRemoveButton = new KPushButton(i18n("Remove"), this);
  servRemoveButton->setIcon(KIcon("list-remove"));
  servRemoveButton->setEnabled(false);
  connect(servRemoveButton, SIGNAL(clicked()), SLOT(removeService()));
  btnsLay->addWidget(servRemoveButton);
  servRemoveButton->setWhatsThis( i18n( "Remove the selected application from the list." ) );

  btnsLay->addStretch(1);
}

void KServiceListWidget::setMimeTypeData( MimeTypeData * mimeTypeData )
{
  m_mimeTypeData = mimeTypeData;
  if ( servNewButton )
    servNewButton->setEnabled(true);
  // will need a selection
  servUpButton->setEnabled(false);
  servDownButton->setEnabled(false);

  servicesLB->clear();
  servicesLB->setEnabled(false);

    if (m_mimeTypeData) {
        const QStringList services = ( m_kind == SERVICELIST_APPLICATIONS )
                                     ? m_mimeTypeData->appServices()
                                     : m_mimeTypeData->embedServices();

        if (services.isEmpty()) {
            if (m_kind == SERVICELIST_APPLICATIONS)
                servicesLB->addItem(i18nc("No applications associated with this file type", "None"));
            else
                servicesLB->addItem(i18nc("No components associated with this file type", "None"));
        } else {
            Q_FOREACH(const QString& service, services) {
                KService::Ptr pService = KService::serviceByStorageId(service);
                if (pService)
                    servicesLB->addItem( new KServiceListItem(pService, m_kind) );
            }
            servicesLB->setEnabled(true);
        }
    }

    if (servRemoveButton)
        servRemoveButton->setEnabled(servicesLB->currentRow() > -1);
    if (servEditButton)
        servEditButton->setEnabled(servicesLB->currentRow() > -1);
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

  // Did the list simply show "None"?
  const bool hadDummyEntry = ( m_kind == SERVICELIST_APPLICATIONS )
                               ? m_mimeTypeData->appServices().isEmpty()
                               : m_mimeTypeData->embedServices().isEmpty();

  if (hadDummyEntry) {
      delete servicesLB->takeItem(0); // Remove the "None" item.
      servicesLB->setEnabled(true);
  } else {
      // check if it is a duplicate entry
      for (int index = 0; index < servicesLB->count(); index++) {
        if (static_cast<KServiceListItem*>( servicesLB->item(index) )->desktopPath
            == service->entryPath()) {
          // ##### shouldn't we make the existing entry the default one?
          return;
        }
      }
  }

  servicesLB->insertItem(0, new KServiceListItem(service, m_kind));
  servicesLB->setCurrentItem(0);

  updatePreferredServices();

  emit changed(true);
}

void KServiceListWidget::editService()
{
    if (!m_mimeTypeData)
        return;
    const int selected = servicesLB->currentRow();
    if (selected < 0)
        return;

    // Only edit applications, not services as
    // they don't have any parameters
    if (m_kind != SERVICELIST_APPLICATIONS)
        return;

    // Just like popping up an add dialog except that we
    // pass the current command line as a default
    KServiceListItem *selItem = (KServiceListItem*)servicesLB->item(selected);
    const QString desktopPath = selItem->desktopPath;

    KService::Ptr service = KService::serviceByDesktopPath(desktopPath);
    if (!service)
        return;

    QString path = service->entryPath();

    // If the path to the desktop file is relative, try to get the full
    // path from KStandardDirs.
    path = KStandardDirs::locate("apps", path); // TODO use xdgdata-apps instead?

    KFileItem item(KUrl(path), "application/x-desktop", KFileItem::Unknown);
    KPropertiesDialog dlg(item, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    // Note that at this point, ksycoca has been updated,
    // and setMimeTypeData has been called again, so all the items have been recreated.

    // Reload service
    service = KService::serviceByDesktopPath(desktopPath);
    if (!service)
        return;

    // Remove the old one...
    delete servicesLB->takeItem(selected);

    // ...check that it's not a duplicate entry...
    bool addIt = true;
    for (int index = 0; index < servicesLB->count(); index++) {
        if (static_cast<KServiceListItem*>(servicesLB->item(index))->desktopPath
            == service->entryPath()) {
            addIt = false;
            break;
        }
    }

    // ...and add it in the same place as the old one:
    if (addIt) {
        servicesLB->insertItem(selected, new KServiceListItem(service, m_kind));
        servicesLB->setCurrentRow(selected);
    }

    updatePreferredServices();

    emit changed(true);
}

void KServiceListWidget::removeService()
{
  if (!m_mimeTypeData) return;

  int selected = servicesLB->currentRow();

  if ( selected >= 0 ) {
    // Check if service is associated with this mimetype or with one of its parents
    KServiceListItem *serviceItem = static_cast<KServiceListItem *>(servicesLB->item(selected));
    if (serviceItem->isImmutable())
    {
       KMessageBox::sorry(this, i18n("You are not authorized to remove this service."));
    }
    else
    {
       delete servicesLB->takeItem( selected );
       updatePreferredServices();

       emit changed(true);
    }
  }

    // Update buttons and service list again (e.g. to re-add "None")
    setMimeTypeData(m_mimeTypeData);
}

void KServiceListWidget::updatePreferredServices()
{
  if (!m_mimeTypeData)
    return;
  QStringList sl;
  unsigned int count = servicesLB->count();

  for (unsigned int i = 0; i < count; i++) {
    KServiceListItem *sli = (KServiceListItem *) servicesLB->item(i);
    sl.append( sli->storageId );
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
    servEditButton->setEnabled( m_kind == SERVICELIST_APPLICATIONS );
}

#include "kservicelistwidget.moc"
