/*
    SPDX-FileCopyrightText: 2001 David Faure <david@mandrakesoft.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BEHAVIOUR_H
#define BEHAVIOUR_H

#include <kcmodule.h>
#include <kconfig.h>
#include <ksharedconfig.h>
#include <QStringList>

class QCheckBox;
class QLabel;

class KBehaviourOptions : public KCModule
{
    Q_OBJECT
public:
    explicit KBehaviourOptions(QObject *parent, const KPluginMetaData &md={}, const QVariantList &args={});
    ~KBehaviourOptions() override;
    void load() override;
    void save() override;
    void defaults() override;

protected Q_SLOTS:
    void updateWinPixmap(bool);

private:
    KSharedConfig::Ptr g_pConfig;
    QString groupname;

    QCheckBox *cbNewWin;

    QLabel *winPixmap;

    QCheckBox *cbShowDeleteCommand;
};

#endif // BEHAVIOUR_H
