/*
   cache.h - Proxy configuration dialog

   Copyright (C) 2001,02,03 Dawit Alemayehu <adawit@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License (GPL) version 2 as published by the Free Software
   Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef CACHECONFIGMODULE_H
#define CACHECONFIGMODULE_H

// KDE
#include <kcmodule.h>

// Local
#include "ui_cache.h"

class CacheConfigModule : public KCModule
{
  Q_OBJECT

public:
  CacheConfigModule(QWidget *parent, const QVariantList &args);
  ~CacheConfigModule();

  virtual void load();
  virtual void save();
  virtual void defaults();
  QString quickHelp() const;

private Q_SLOTS:
  void configChanged();
  void on_clearCacheButton_clicked();

private:
  Ui::CacheConfigUI ui;
};

#endif // CACHE_H
