/*
 *  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
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
 */

#ifndef __KCMCSS_H__
#define __KCMCSS_H__


#include <QtCore/QMap>

#define KDE3_SUPPORT
#include <kcmodule.h>
#undef KDE3_SUPPORT
#include "ui_csscustom.h"
#include "ui_cssconfig.h"
#include "ui_preview.h"

class KDialog;

class PreviewDialog : public QDialog, public Ui::PreviewDialog
{
public:
  PreviewDialog( QWidget *parent ) : QDialog( parent ) {
    setModal(true);
    setupUi( this );
  }
};


class CSSConfigDialog : public QWidget, public Ui::CSSConfigDialog
{
public:
  CSSConfigDialog( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};


class CSSCustomDialog : public QWidget, public Ui::CSSCustomDialog
{
public:
  CSSCustomDialog( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};



class CSSConfig : public KCModule
{
  Q_OBJECT

public:
	  
  explicit CSSConfig(QWidget *parent = 0L, const QStringList &list =QStringList() );

  void load();
  void save();
  void defaults();

public Q_SLOTS:
 
  void slotPreview();
  void slotCustomize();

private:

  QMap<QString,QString> cssDict();

  CSSConfigDialog *configDialog;
  CSSCustomDialog *customDialog;
  KDialog *customDialogBase;
};


#endif
