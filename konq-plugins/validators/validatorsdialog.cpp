/* This file is part of the KDE project

   Copyright (C) 2001 Andreas Schlapbach <schlpbch@iam.unibe.ch>
   Copyright (C) 2008 Pino Toscano <pino@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "validatorsdialog.h"

#include "settings.h"

#include <kicon.h>
#include <klocale.h>

ValidatorsDialog::ValidatorsDialog(QWidget *parent )
  : KPageDialog( parent)
{
  setButtons(Ok|Cancel);
  setDefaultButton(Ok);
  setModal(false);
  showButtonSeparator(true);
  setCaption(i18nc("@title:window", "Configure Validator Plugin"));
  setMinimumWidth(400);

#ifdef HAVE_TIDY
  QWidget* internalConfiguration = new QWidget();
  m_internalUi.setupUi(internalConfiguration);
  internalConfiguration->layout()->setMargin(0);
  KPageWidgetItem *internalConfigurationItem = addPage(internalConfiguration, i18n("Internal Validation"));
  internalConfigurationItem->setIcon(KIcon("validators"));
#endif

  QWidget* remoteConfiguration = new QWidget();
  m_remoteUi.setupUi(remoteConfiguration);
  remoteConfiguration->layout()->setMargin(0);
  KPageWidgetItem *remoteConfigurationItem = addPage(remoteConfiguration, i18n("Remote Validation"));
  remoteConfigurationItem->setIcon(KIcon("validators"));

  connect(this,SIGNAL(okClicked()),this,SLOT(slotOk()));
  connect(this,SIGNAL(cancelClicked()),this,SLOT(slotCancel()));
  load();
}

ValidatorsDialog::~ValidatorsDialog()
{
}

void ValidatorsDialog::load()
{
  m_remoteUi.m_WWWValidatorCB->addItems(ValidatorsSettings::wWWValidatorUrl());
  m_remoteUi.m_WWWValidatorCB->setCurrentIndex(ValidatorsSettings::wWWValidatorUrlIndex());

  m_remoteUi.m_CSSValidatorCB->addItems(ValidatorsSettings::cSSValidatorUrl());
  m_remoteUi.m_CSSValidatorCB->setCurrentIndex(ValidatorsSettings::cSSValidatorUrlIndex());

  m_remoteUi.m_linkValidatorCB->addItems(ValidatorsSettings::linkValidatorUrl());
  m_remoteUi.m_linkValidatorCB->setCurrentIndex(ValidatorsSettings::linkValidatorUrlIndex());

  m_remoteUi.m_WWWValidatorUploadCB->addItems(ValidatorsSettings::wWWValidatorUploadUrl());
  m_remoteUi.m_WWWValidatorUploadCB->setCurrentIndex(ValidatorsSettings::wWWValidatorUploadUrlIndex());

  m_remoteUi.m_CSSValidatorUploadCB->addItems(ValidatorsSettings::cSSValidatorUploadUrl());
  m_remoteUi.m_CSSValidatorUploadCB->setCurrentIndex(ValidatorsSettings::cSSValidatorUploadUrlIndex());

#ifdef HAVE_TIDY
  m_internalUi.accessibilityLevel->setCurrentIndex(ValidatorsSettings::accessibilityLevel());
  m_internalUi.runAfterLoading->setChecked(ValidatorsSettings::runAfterLoading());
#endif
}

void ValidatorsDialog::save()
{
  QStringList strList;
  for (int i = 0; i < m_remoteUi.m_WWWValidatorCB->count(); i++) {
    strList.append(m_remoteUi.m_WWWValidatorCB->itemText(i));
  }
  ValidatorsSettings::setWWWValidatorUrl(strList);
  strList.clear();
  for (int i = 0; i < m_remoteUi.m_CSSValidatorCB->count(); i++) {
    strList.append(m_remoteUi.m_CSSValidatorCB->itemText(i));
  }
  ValidatorsSettings::setCSSValidatorUrl(strList);
  strList.clear();
  for (int i = 0; i < m_remoteUi.m_linkValidatorCB->count(); i++) {
    strList.append(m_remoteUi.m_linkValidatorCB->itemText(i));
  }
  ValidatorsSettings::setLinkValidatorUrl(strList);
  strList.clear();
  for (int i = 0; i < m_remoteUi.m_WWWValidatorUploadCB->count(); i++) {
    strList.append(m_remoteUi.m_WWWValidatorUploadCB->itemText(i));
  }
  ValidatorsSettings::setWWWValidatorUploadUrl(strList);
  strList.clear();
  for (int i = 0; i < m_remoteUi.m_CSSValidatorUploadCB->count(); i++) {
    strList.append(m_remoteUi.m_CSSValidatorUploadCB->itemText(i));
  }
  ValidatorsSettings::setCSSValidatorUploadUrl(strList);

  ValidatorsSettings::setWWWValidatorUrlIndex(m_remoteUi.m_WWWValidatorCB->currentIndex());
  ValidatorsSettings::setCSSValidatorUrlIndex(m_remoteUi.m_CSSValidatorCB->currentIndex());
  ValidatorsSettings::setLinkValidatorUrlIndex(m_remoteUi.m_linkValidatorCB->currentIndex());
  ValidatorsSettings::setWWWValidatorUploadUrlIndex(m_remoteUi.m_WWWValidatorUploadCB->currentIndex());
  ValidatorsSettings::setCSSValidatorUploadUrlIndex(m_remoteUi.m_CSSValidatorUploadCB->currentIndex());

#ifdef HAVE_TIDY
  ValidatorsSettings::setAccessibilityLevel(m_internalUi.accessibilityLevel->currentIndex());
  ValidatorsSettings::setRunAfterLoading(m_internalUi.runAfterLoading->isChecked());
#endif

  ValidatorsSettings::self()->writeConfig();

  emit configChanged();
}

void ValidatorsDialog::slotOk()
{
   save();
   hide();
}

void ValidatorsDialog::slotCancel()
{
   load();
   hide();
}

#include "validatorsdialog.moc"
