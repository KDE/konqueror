/*
    main.h

    SPDX-FileCopyrightText: 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
    SPDX-FileCopyrightText: 2000 Daniel Molkentin <molkentin@kde.org>

    Requires the Qt widget libraries, available at no cost at
    http://www.troll.no/

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef __MAIN_H__
#define __MAIN_H__

#include <kcmodule.h>
#include <ksharedconfig.h>

class KJavaOptions;
class KJavaScriptOptions;

class QTabWidget;

class KJSParts : public KCModule
{
    Q_OBJECT

public:

    KJSParts(QWidget *parent, const QVariantList &);

    void load() override;
    void save() override;
    void defaults() override;
    QString quickHelp() const override;

private:
    QTabWidget   *tab;

    KJavaScriptOptions *javascript;
    KJavaOptions       *java;

    KSharedConfig::Ptr mConfig;
};

#endif
