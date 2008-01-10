/* This file is part of the KDE project
   Copyright (C) 2003 Waldo Bastian <bastian@kde.org>
   Copyright (C) 2003 David Faure <faure@kde.org>
   Copyright (C) 2002 Daniel Molkentin <molkentin@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 or at your option version 3 as published
   by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _KSERVICELISTWIDGET_H
#define _KSERVICELISTWIDGET_H

#include <QGroupBox>
#include <QtGui/QListWidget>
#include <kservice.h>
class MimeTypeData;
class QPushButton;
class KService;

class KServiceListItem : public QListWidgetItem
{
public:
    KServiceListItem( /*TODO const &*/ KService::Ptr pService, int kind );
    bool isImmutable();
    QString desktopPath;
    QString localPath;
};

/**
 * This widget holds a list of services, with 5 buttons to manage it.
 * It's a separate class so that it can be used by both tabs of the
 * module, once for applications and once for services.
 * The "kind" is determined by the argument given to the constructor.
 */
class KServiceListWidget : public QGroupBox
{
  Q_OBJECT
public:
  enum { SERVICELIST_APPLICATIONS, SERVICELIST_SERVICES };
  explicit KServiceListWidget(int kind, QWidget *parent = 0);

  void setMimeTypeData( MimeTypeData * item );

Q_SIGNALS:
  void changed(bool);

protected Q_SLOTS:
  void promoteService();
  void demoteService();
  void addService();
  void editService();
  void removeService();
  void enableMoveButtons();

protected:
  void updatePreferredServices();

private:
  int m_kind;
  QListWidget *servicesLB;
  QPushButton *servUpButton, *servDownButton;
  QPushButton *servNewButton, *servEditButton, *servRemoveButton;
  MimeTypeData *m_mimeTypeData;
};

#endif
