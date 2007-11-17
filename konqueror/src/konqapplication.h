/* This file is part of the KDE project
   Copyright (C) 2006 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KONQ_APPLICATION_H
#define KONQ_APPLICATION_H

#include "konqprivate_export.h"
#include <kapplication.h>

class QDBusMessage;

// This is used to know if we are being closed by session management
// or by the user. See KonqMainWindow::~KonqMainWindow.
// Credits to Matthias Ettrich for the idea.
class KONQ_TESTS_EXPORT KonquerorApplication : public KApplication
{
  Q_OBJECT
public:
  KonquerorApplication();

  bool closedByUser() const { return !closed_by_sm; }
  void commitData(QSessionManager& sm) {
    closed_by_sm = true;
    KApplication::commitData( sm );
    closed_by_sm = false;
  }

public slots:
  void slotReparseConfiguration();
  void slotUpdateProfileList();

private slots:
  void slotAddToCombo( const QString& url, const QDBusMessage& msg );
  void slotRemoveFromCombo( const QString& url, const QDBusMessage& msg );
  void slotComboCleared( const QDBusMessage& msg );

private:
  bool closed_by_sm;

};

#endif
