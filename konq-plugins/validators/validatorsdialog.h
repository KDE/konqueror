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

#ifndef __validatorsdialog_h
#define __validatorsdialog_h

#include <kpagedialog.h>

#include "ui_remotevalidators.h"

#include <config-konq-validator.h>

#ifdef HAVE_TIDY
#include "ui_internalvalidator.h"
#endif

class ValidatorsDialog : public KPageDialog
{
 Q_OBJECT

 public:
  explicit ValidatorsDialog(QWidget *parent=0 );
  ~ValidatorsDialog();

 signals:
  void configChanged();

 protected slots:
  void slotOk();
  void slotCancel();

 private:
  void load();
  void save();

  Ui::RemoteValidators m_remoteUi;
#ifdef HAVE_TIDY
  Ui::InternalValidator m_internalUi;
#endif
};

#endif
