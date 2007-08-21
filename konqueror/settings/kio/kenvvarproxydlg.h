/*
   kenvvarproxydlg.h - Base dialog box for proxy configuration

   Copyright (C) 2001- Dawit Alemayehu <adawit@kde.org>

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
#ifndef KENVVARPROXYDLG_H
#define KENVVARPROXYDLG_H

#include <QtCore/QMap>

#include "kproxydlgbase.h"
#include "ui_envvarproxy_ui.h"

class EnvVarProxyDlgUI : public QWidget, public Ui::EnvVarProxyDlgUI
{
public:
  EnvVarProxyDlgUI( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};


class KEnvVarProxyDlg : public KProxyDialogBase
{
    Q_OBJECT

public:
    explicit KEnvVarProxyDlg( QWidget* parent = 0, const char* name = 0 );
    ~KEnvVarProxyDlg();

    virtual const KProxyData data() const;
    virtual void setProxyData( const KProxyData &data );

protected Q_SLOTS:
    virtual void accept();

    void showValue();
    void verifyPressed();
    void autoDetectPressed();

protected:
    void init();
    bool validate();
    
private:
    EnvVarProxyDlgUI* mDlg;

    struct EnvVarPair {
      QString name;
      QString value;
    };

    QMap<QString, EnvVarPair> mEnvVarsMap;
};

#endif // KENVVARPROXYDLG_H
