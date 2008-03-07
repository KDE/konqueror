/*
  Copyright (c) 2003 Dirk Mueller <mueller@kde.org>

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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <QRegExp>
#include <QLayout>
#include <kstandarddirs.h>
#include <klocale.h>
#include <ksharedconfig.h>
#include <kiconloader.h>
#include <kfiledialog.h>
#include <kurlrequester.h>
#include <kdebug.h>

#include <kapplication.h>
#include <k3procio.h>
#include <ktoolinvocation.h>

#include "plugin_paths.h"

static QDateTime lastChanged( const QString &dir )
{
    QDateTime t = QFileInfo( dir ).lastModified();
    if( t.isNull())
        return t;
    QStringList subdirs = QDir( dir ).entryList();
    for( QStringList::ConstIterator it = subdirs.begin();
         it != subdirs.end();
         ++it )
    {
        if( *it == "." || *it == ".." )
            continue;
        QDateTime t2 = lastChanged( *it );
        if( !t2.isNull() && t2 > t )
            t = t2;
    }
    return t;
}

static bool checkSearchPathTimestamps( const QStringList &paths, const QStringList &timestamps )
{
    QStringList currentTimestamps;
    bool changed = false;
    QStringList::ConstIterator t = timestamps.begin();
    for( QStringList::ConstIterator it = paths.begin();
         it != paths.end();
         ++it, ++t )
    {
        QDateTime current = lastChanged( *it );
        // store non-existent directory as "N" string rather than empty string, KConfig
        // has a bug with storing a list of empty items
        if( *t == "N" ? !current.isNull() : current != QDateTime::fromString( *t, Qt::ISODate ))
            changed = true;
        currentTimestamps.append( current.isNull() ? "N" : current.toString( Qt::ISODate ));
    }
    if( changed )
    {
        KConfig config("kcmnspluginrc");
	KConfigGroup cg(&config, "Misc");
        cg.writeEntry( "lastSearchPaths", paths );
        cg.writeEntry( "lastSearchTimestamps", currentTimestamps );
        return true;
    }
    return false;
}

extern "C"
{
    KDE_EXPORT void kcminit_nsplugin()
    {
        KConfigGroup config(KSharedConfig::openConfig( "kcmnspluginrc", KConfig::NoGlobals ), "Misc");
        if( true ) // I don't think this needs to be configurable now
        {
            bool update = false;
            QStringList searchPaths = getSearchPaths();
            QStringList lastSearchPaths = config.readEntry( "lastSearchPaths", QStringList());
            QStringList lastTimestamps = config.readEntry ( "lastSearchTimestamps", QStringList());
            if( searchPaths != lastSearchPaths || lastTimestamps.count() != lastSearchPaths.count())
            { // count changed, set empty timestamps, still call checkSearchPathTimestamps()
              // in order to save the current timestamps for the next time
                lastSearchPaths = searchPaths;
                lastTimestamps.clear();
                for( int i = 0;
                     i < searchPaths.count();
                     ++i )
                    lastTimestamps.append( "N" );
                update = true;
            }
            if( checkSearchPathTimestamps( lastSearchPaths, lastTimestamps ))
                update = true;
            if( update )
                KToolInvocation::kdeinitExec("nspluginscan");
        }
    }
}
