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

#ifndef __konq_profiledlg_h__
#define __konq_profiledlg_h__

#include <kdialog.h>

#include <QtCore/QMap>
#include <QtGui/QGridLayout>
#include <QtGui/QListWidgetItem>

class KonqViewManager;
class QGridLayout;
class QCheckBox;
class QLineEdit;
class KListWidget;

typedef QMap<QString, QString> KonqProfileMap;

class KonqProfileItem : public QListWidgetItem
{
public:
  KonqProfileItem( KListWidget *, const QString & );
  ~KonqProfileItem() {}

  QString m_profileName;
};

class KonqProfileDlg : public KDialog
{
  Q_OBJECT
public:
  KonqProfileDlg( KonqViewManager *manager, const QString &preselectProfile, QWidget *parent = 0L );
  ~KonqProfileDlg();

  /**
   * Find, read and return all available profiles
   * @return a map with < name, full path >
   */
  static KonqProfileMap readAllProfiles();

protected Q_SLOTS:
  void slotRenameProfile();
  void slotDeleteProfile();
  void slotSave();
  void slotTextChanged( const QString & );
  void slotSelectionChanged();

  void slotItemRenamed( QListWidgetItem * );

private:
  void loadAllProfiles(const QString & = QString());
  KonqViewManager *m_pViewManager;

  KonqProfileMap m_mapEntries;

  QLineEdit *m_pProfileNameLineEdit;

  QCheckBox *m_cbSaveURLs;
  QCheckBox *m_cbSaveSize;

  KListWidget *m_pListView;
};

#endif
