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

#ifndef _USERAGENTDLG_H
#define _USERAGENTDLG_H

#include <kcmodule.h>
#include "ui_useragentdlg.h"

class KConfig;
class UserAgentInfo;
class QTreeWidgetItem;

class UserAgentDlg : public KCModule
{
  Q_OBJECT

public:
  UserAgentDlg(QWidget *parent, const QVariantList &args);
  ~UserAgentDlg();

  virtual void load();
  virtual void save();
  virtual void defaults();
  QString quickHelp() const;

private Q_SLOTS:
  void updateButtons();

  void on_newButton_clicked();
  void on_changeButton_clicked();
  void on_deleteButton_clicked();
  void on_deleteAllButton_clicked();

  void on_sendUACheckBox_clicked();
  void on_osNameCheckBox_clicked();
  void on_osVersionCheckBox_clicked();
  void on_platformCheckBox_clicked();
  void on_processorTypeCheckBox_clicked();
  void on_languageCheckBox_clicked();
  void on_sitePolicyTreeWidget_itemSelectionChanged();
  void on_sitePolicyTreeWidget_itemActivated(QTreeWidgetItem*, int);

private:
  void changeDefaultUAModifiers();
  void configChanged(bool enable = true);
  bool handleDuplicate( const QString&, const QString&, const QString& );

  enum
  {
    SHOW_OS = 0,
    SHOW_OS_VERSION,
    SHOW_PLATFORM,
    SHOW_MACHINE,
    SHOW_LANGUAGE
  };

  // Useragent modifiers...
  QString m_ua_keys;

  // Fake user-agent modifiers...
  UserAgentInfo* m_userAgentInfo;
  KConfig *m_config;

  // Interface...
  Ui::UserAgentUI ui;
};

#endif
