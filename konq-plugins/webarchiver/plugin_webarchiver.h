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
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 **/

/* $Id$ */

#ifndef plugin_webarchiver_h
#define plugin_webarchiver_h

#include <kparts/plugin.h>
#include <klibloader.h>

class PluginWebArchiver : public KParts::Plugin
{
  Q_OBJECT

 public:
  PluginWebArchiver( QObject* parent,
                     const QStringList & );
  virtual ~PluginWebArchiver();

 public slots:
   void slotSaveToArchive();
};

#endif
