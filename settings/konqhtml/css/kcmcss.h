/*
    SPDX-FileCopyrightText: 2000 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KCMCSS_H
#define KCMCSS_H

#include <QMap>

#include <KParts/ReadOnlyPart>

#include "ui_csscustom.h"

class QDialog;
class CSSConfigWidget;
class QButtonGroup;
class QAbstractButton;

class CSSCustomDialog: public QWidget, public Ui::CSSCustomDialog
{
    Q_OBJECT
public:
    CSSCustomDialog(QWidget *parent);
    QMap<QString, QString> cssDict();

public Q_SLOTS:
    void slotPreview();

Q_SIGNALS:
    void changed();

private:
    KParts::ReadOnlyPart *part;
};

class CSSConfig : public QWidget
{
    Q_OBJECT

public:

    explicit CSSConfig(QWidget *parent = nullptr, const QVariantList &list = QVariantList());

    void load();
    void save();
    void defaults();

public Q_SLOTS:

    void slotCustomize();

private Q_SLOTS:
    void stylesheetChoiceChanged(QAbstractButton *btn, bool checked);
    void useCustomBackgroundToggled(bool on);

Q_SIGNALS:
    void changed();//connected to KCModule slot

private:

    CSSConfigWidget *configWidget;
    QButtonGroup *stylesheetChoicesGroup;
    QDialog *customDialogBase;
    CSSCustomDialog *customDialog;
};

#endif
