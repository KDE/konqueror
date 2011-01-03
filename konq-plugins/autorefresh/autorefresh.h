// -*- c++ -*-

/*
  Copyright 2003 by Richard J. Moore, rich@kde.org

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
 */

#ifndef __plugin_autorefresh_h
#define __plugin_autorefresh_h
 
#include <kparts/plugin.h>
 
class QTimer;
class KSelectAction;

/**
 * A plugin is the way to add actions to an existing @ref KParts application,
 * or to a @ref Part.
 *
 * The XML of those plugins looks exactly like of the shell or parts,
 * with one small difference: The document tag should have an additional
 * attribute, named "library", and contain the name of the library implementing
 * the plugin.
 *
 * If you want this plugin to be used by a part, you need to
 * install the rc file under the directory
 * "data" (KDEDIR/share/apps usually)+"/instancename/kpartplugins/"
 * where instancename is the name of the part's instance.
 **/
class AutoRefresh : public KParts::Plugin
{
    Q_OBJECT
public:

    /**
     * Construct a new KParts plugin.
     */
    explicit AutoRefresh( QObject* parent = 0, const QVariantList &args = QVariantList() );

    /**
     * Destructor.
     */
    virtual ~AutoRefresh();

public slots:
    void slotRefresh();
    void slotIntervalChanged();
    
private:
   KSelectAction *refresher;
   QTimer *timer;
};

#endif
