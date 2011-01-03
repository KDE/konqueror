/*
    Copyright (c) 2003 Alexander Kellett <lypanov@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License (LGPL) as published by the Free Software Foundation;
    either version 2 of the License, or (at your option) any later
    version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef __MINITOOLS_PLUGIN_H
#define __MINITOOLS_PLUGIN_H

#include <qmap.h>
#include <QList>
#include <qstringlist.h>

#include <kurl.h>
#include <klibloader.h>
#include <kparts/plugin.h>

class KHTMLPart;
class KActionMenu;

class MinitoolsPlugin : public KParts::Plugin {
   Q_OBJECT

public:
   MinitoolsPlugin( QObject* parent, const QVariantList & );
   ~MinitoolsPlugin();

protected slots:
   void slotAboutToShow();
   void slotEditBookmarks();
   void slotItemSelected();
   void newBookmarkCallback( const QString &, const QString &, const QString & );
   void endFolderCallback( );

signals:
   void executeScript( const QString &script );

private:
   QString minitoolsFilename(bool local);

   int m_selectedItem;
  
   KHTMLPart* m_part;
   KActionMenu* m_pMinitoolsMenu;
  
   typedef QPair<QString,QString> Minitool;
   typedef QList<Minitool> MinitoolsList;
  
   MinitoolsList m_minitoolsList;
};

#endif
