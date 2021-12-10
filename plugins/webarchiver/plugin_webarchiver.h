/* This file is part of Webarchiver
    SPDX-FileCopyrightText: 2001 Andreas Schlapbach <schlpbch@iam.unibe.ch>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLUGIN_WEBARCHIVER_H
#define PLUGIN_WEBARCHIVER_H

#include <konq_kpart_plugin.h>

class PluginWebArchiver : public KonqParts::Plugin
{
    Q_OBJECT

public:
    PluginWebArchiver(QObject *parent, const QVariantList &args);
    ~PluginWebArchiver() override = default;

protected slots:
    void slotSaveToArchive();
};

#endif
