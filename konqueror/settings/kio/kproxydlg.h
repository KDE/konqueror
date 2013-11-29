/*
   kproxydlg.h - Proxy configuration dialog

   Copyright (C) 2001, 2011 Dawit Alemayehu <adawit@kde.org>

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

#ifndef KPROXYDLG_H
#define KPROXYDLG_H

#include <kcmodule.h>
#include "ui_kproxydlg.h"

class KProxyDialog : public KCModule
{
    Q_OBJECT

public:
    enum DisplayUrlFlag {
        HideNone = 0x00,
        HideHttpUrlScheme = 0x01,
        HideHttpsUrlScheme = 0x02,
        HideFtpUrlScheme = 0x04,
        HideSocksUrlScheme = 0x08
    };
    Q_DECLARE_FLAGS(DisplayUrlFlags, DisplayUrlFlag)

    KProxyDialog(QWidget* parent, const QVariantList& args);
    ~KProxyDialog();

    virtual void load();
    virtual void save();
    virtual void defaults();
    QString quickHelp() const;

private Q_SLOTS:
    void on_autoDetectButton_clicked();
    void on_showEnvValueCheckBox_toggled(bool);
    void on_useSameProxyCheckBox_clicked(bool);

    void on_manualProxyHttpEdit_textChanged(const QString&);
    void on_manualNoProxyEdit_textChanged(const QString&);    
    void on_manualProxyHttpEdit_textEdited(const QString&);
    void on_manualProxyHttpSpinBox_valueChanged(int);

    void slotChanged();

private:
    bool autoDetectSystemProxy(QLineEdit* edit, const QString& envVarStr, bool showValue);

    Ui::ProxyDialogUI mUi;
    QStringList mNoProxyForList;
    QMap<QString, QString> mProxyMap;
};

Q_DECLARE_OPERATORS_FOR_FLAGS (KProxyDialog::DisplayUrlFlags)

#endif // KPROXYDLG_H
