/*
 * main.cpp for lisa and kio_lan kcm module
 *
 *  Copyright (C) 2000, 2006 Alexander Neundorf <neundorf@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef MYMAIN_H
#define MYMAIN_H

// Qt
#include <QtGui/QTabWidget>
#include <QtGui/QVBoxLayout>

// KDE
#include <kcmodule.h>
#include <kglobal.h>

class QTabWidget;

class LanBrowser:public KCModule
{
   Q_OBJECT
   public:
      LanBrowser(QWidget *parent, const QVariantList &args);
      virtual void load();
      virtual void save();

   private:
      QVBoxLayout layout;
      QTabWidget tabs;
      KCModule *smbPage;
      KCModule *lisaPage;
      KCModule *kioLanPage;
};
#endif

