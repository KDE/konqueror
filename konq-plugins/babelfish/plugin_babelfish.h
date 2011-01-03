/* This file is part of the KDE Project
   Copyright (C) 2001 Kurt Granroth <granroth@kde.org>
   Copyright (C) 2003 Rand2342 <rand2342@yahoo.com>

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
#ifndef __plugin_babelfish_h
#define __plugin_babelfish_h

#include <kparts/plugin.h>
#include <klibloader.h>
#include <kactionmenu.h>
#include <QActionGroup>

class PluginBabelFish : public KParts::Plugin
{
  Q_OBJECT
public:
  explicit PluginBabelFish( QObject* parent,
	           const QVariantList & );
  virtual ~PluginBabelFish();

private slots:
  void translateURL(QAction *);
  void slotAboutToShow();
  void slotEnableMenu();

private:
  void addTopLevelAction(const QString& name, const QString& text);

private:
  QActionGroup m_actionGroup;
  KActionMenu* m_menu;
};

#endif
