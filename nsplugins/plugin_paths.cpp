/*

  This application scans for Netscape plugins and create a cache and
  the necessary mimelnk and service files.


  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
                     Stefan Schimanski <1Stein@gmx.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include "plugin_paths.h"

#include <ksharedconfig.h>
#include <kconfiggroup.h>
#include <stdlib.h>

QStringList getSearchPaths()
{
    QStringList searchPaths;
    KConfigGroup config(KSharedConfig::openConfig( "kcmnspluginrc", KConfig::NoGlobals ), "Misc");

    // setup default paths
    if ( !config.hasKey("scanPaths") ) {
        QStringList paths;
        // keep sync with kdebase/apps/konqueror/settings/konqhtml
        paths.append("$HOME/.mozilla/plugins");
        paths.append("$HOME/.netscape/plugins");
        paths.append("/usr/lib/firefox/plugins");
        paths.append("/usr/lib64/browser-plugins");
        paths.append("/usr/lib/browser-plugins");
        paths.append("/usr/local/netscape/plugins");
        paths.append("/opt/mozilla/plugins");
	paths.append("/opt/mozilla/lib/plugins");
        paths.append("/opt/netscape/plugins");
        paths.append("/opt/netscape/communicator/plugins");
        paths.append("/usr/lib/netscape/plugins");
        paths.append("/usr/lib/netscape/plugins-libc5");
        paths.append("/usr/lib/netscape/plugins-libc6");
        paths.append("/usr/lib/mozilla/plugins");
	paths.append("/usr/lib64/netscape/plugins");
	paths.append("/usr/lib64/mozilla/plugins");
        paths.append("$MOZILLA_HOME/plugins");
        config.writeEntry( "scanPaths", paths );
    }

    // read paths
    searchPaths = config.readPathListEntry( "scanPaths" );

    // append environment variable NPX_PLUGIN_PATH
    QStringList envs = QString( getenv("NPX_PLUGIN_PATH") ).split(':');
    QStringList::Iterator it;
    for (it = envs.begin(); it != envs.end(); ++it)
        searchPaths.append(*it);

    return searchPaths;
}
