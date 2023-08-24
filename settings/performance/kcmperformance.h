/*
    SPDX-FileCopyrightText: 2003 Lubos Lunak <l.lunak@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _KCM_PERFORMANCE_H
#define _KCM_PERFORMANCE_H

#include <kcmodule.h>

namespace KCMPerformance
{

class Konqueror;
class SystemWidget;

class Config
    : public KCModule
{
    Q_OBJECT
public:
    //TODO KF6: when dropping compatibility with KF5, remove QVariantList argument
    Config(QObject *parent_P, const KPluginMetaData &md={}, const QVariantList &args={});
    void load() override;
    void save() override;
    void defaults() override;
private:
    Konqueror *konqueror_widget;
    SystemWidget *system_widget;
};

class KonquerorConfig
    : public KCModule
{
    Q_OBJECT
public:
    //TODO KF6: when dropping compatibility with KF5, remove QVariantList argument
    KonquerorConfig(QObject *parent, const KPluginMetaData &md={}, const QVariantList &args={});
    void load() override;
    void save() override;
    void defaults() override;
private:
    Konqueror *m_widget;
};

} // namespace

#endif
