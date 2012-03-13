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

#ifndef KCMCSS_H
#define KCMCSS_H


#include <QtCore/QMap>

#include <KParts/ReadOnlyPart>

#include "ui_csscustom.h"

class KDialog;
class CSSConfigWidget;
class KHTMLPart;

class CSSCustomDialog: public QWidget, public Ui::CSSCustomDialog
{
    Q_OBJECT
public:
  CSSCustomDialog( QWidget *parent );
  QMap<QString,QString> cssDict();

public Q_SLOTS:
  void slotPreview();

Q_SIGNALS:
  void changed();

private:
  KParts::ReadOnlyPart* part;
};



class CSSConfig : public QWidget
{
  Q_OBJECT

public:
	  
  explicit CSSConfig(QWidget *parent = 0L, const QVariantList &list =QVariantList() );

  void load();
  void save();
  void defaults();

public Q_SLOTS:
 
  void slotCustomize();

Q_SIGNALS:
  void changed(bool);//connected to KCModule signal
  void changed();//connected to KCModule slot

private:

  CSSConfigWidget *configWidget;
  KDialog *customDialogBase;
  CSSCustomDialog *customDialog;
};


#endif
