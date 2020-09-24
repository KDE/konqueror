/* This file is part of Webarchiver
 *  Copyright (C) 2001 by Andreas Schlapbach <schlpbch@iam.unibe.ch>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 **/

#ifndef PLUGIN_WEBARCHIVER_H
#define PLUGIN_WEBARCHIVER_H

#include <kparts/plugin.h>


class PluginWebArchiver : public KParts::Plugin
{
    Q_OBJECT

public:
    PluginWebArchiver(QObject *parent, const QVariantList &args);
    ~PluginWebArchiver() override = default;

protected slots:
    void slotSaveToArchive();
};

#endif
