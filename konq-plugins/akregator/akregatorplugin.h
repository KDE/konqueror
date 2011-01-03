/* This file is part of the KDE project

   Copyright (C) 2004 Gary Cramblitt <garycramblitt@comcast.net>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of 
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _AKREGATORPLUGIN_H_
#define _AKREGATORPLUGIN_H_

#include <konq_popupmenu.h>
#include <kfileitem.h>
#include <kconfig.h>

#include "pluginbase.h"

class KHTMLPart;

namespace Akregator
{

class AkregatorMenu : public KonqPopupMenuPlugin, PluginBase
{
  Q_OBJECT
public:
    AkregatorMenu( KonqPopupMenu *, const QStringList &list );
    virtual ~AkregatorMenu();

public slots:
    void slotAddFeed();

protected:
    bool isFeedUrl(const QString &s);
    bool isFeedUrl(const KFileItem &item);

private:
    QStringList m_feedMimeTypes;
//    KConfig *m_conf;
    KHTMLPart *m_part;
    QString m_feedURL;
};

}

#endif

