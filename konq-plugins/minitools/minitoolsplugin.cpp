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

#include <qfile.h>

#include <kdebug.h>
#include <kaction.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kcomponentdata.h>
#include <khtml_part.h>
#include <kgenericfactory.h>
#include <kicon.h>
#include <kstandarddirs.h>

#include <krun.h>
#include <kservice.h>
#include <kactionmenu.h>
#include <kmenu.h>
#include <kbookmarkimporter.h>
#include <kbookmarkmanager.h>
#include <kactioncollection.h>
#include "minitoolsplugin.h"

K_PLUGIN_FACTORY(MinitoolsPluginFactory, registerPlugin<MinitoolsPlugin>();)
K_EXPORT_PLUGIN(MinitoolsPluginFactory("minitoolsplugin"))


MinitoolsPlugin::MinitoolsPlugin(QObject* parent, const QVariantList &)
   : KParts::Plugin(parent) {
  m_part = (parent && parent->inherits( "KHTMLPart" )) ? static_cast<KHTMLPart*>(parent) : 0L;

  m_pMinitoolsMenu = new KActionMenu(KIcon("minitools"),i18n("&Minitools"), actionCollection());
  actionCollection()->addAction("minitools", m_pMinitoolsMenu);

  m_pMinitoolsMenu->setDelayed(false);
  m_pMinitoolsMenu->setEnabled(true);

  connect(m_pMinitoolsMenu->menu(), SIGNAL( aboutToShow() ),
                                   this, SLOT( slotAboutToShow() ));
}

MinitoolsPlugin::~MinitoolsPlugin() {
   ;
}

void MinitoolsPlugin::slotAboutToShow() {

  m_minitoolsList.clear();
  KXBELBookmarkImporterImpl importer;
  connect(&importer, SIGNAL( newBookmark( const QString &, const QString &, const QString &) ),
                     SLOT( newBookmarkCallback( const QString &, const QString &, const QString & ) ));
  connect(&importer, SIGNAL( endFolder() ), 
                     SLOT( endFolderCallback() ));
  QString filename = minitoolsFilename(true);
  if (!filename.isEmpty() && QFile::exists(filename)) {
     importer.setFilename(filename);
     importer.parse();
  }
  filename = minitoolsFilename(false);
  if (!filename.isEmpty() && QFile::exists(filename)) {
     importer.setFilename(filename);
     importer.parse();
  }

  m_pMinitoolsMenu->menu()->clear();

  int count = m_pMinitoolsMenu->menu()->actions().count(); // why not 0???
  bool gotSep = true; // don't start with a sep

  if (m_minitoolsList.count() > 0) {
     MinitoolsList::ConstIterator e = m_minitoolsList.constBegin();
     for( ; e != m_minitoolsList.constEnd(); ++e ) {
        if ( ((*e).first  == "-") 
          && ((*e).second == "-")
        ) {
           if (!gotSep)
              m_pMinitoolsMenu->menu()->addSeparator();
           gotSep = true;
           count++;
        } else {
           QString str = (*e).first;
           // emsquieezzy thingy?
           if (str.length() > 48) {
              str.truncate(48);
              str.append("...");
           }
           QAction* action = m_pMinitoolsMenu->menu()->addAction(
              str, this,
              SLOT(slotItemSelected()));
           action->setData(qVariantFromValue(++count));
           gotSep = false;
        }
     }
  }

  if (!gotSep) {
     // don't have an extra sep
     m_pMinitoolsMenu->menu()->addSeparator();
  }

  m_pMinitoolsMenu->menu()
                       ->addAction(i18n("&Edit Minitools"),
                                    this, SLOT(slotEditBookmarks()));
}

void MinitoolsPlugin::newBookmarkCallback(
   const QString & text, const QString & url, const QString & 
) {
  kDebug(90150) << "MinitoolsPlugin::newBookmarkCallback" << text << url;
  m_minitoolsList.prepend(qMakePair(text,url));
}

void MinitoolsPlugin::endFolderCallback() {
  kDebug(90150) << "MinitoolsPlugin::endFolderCallback";
  m_minitoolsList.prepend(qMakePair(QString("-"),QString("-")));
}

QString MinitoolsPlugin::minitoolsFilename(bool local) {
  return local ? KStandardDirs::locateLocal("data", QLatin1String("konqueror/minitools.xml"))
               : KStandardDirs::locateLocal("data", QLatin1String("konqueror/minitools-global.xml"));
}

void MinitoolsPlugin::slotEditBookmarks() {
   KBookmarkManager *manager = KBookmarkManager::managerForFile(minitoolsFilename(true),"minitools");
   manager->slotEditBookmarks();
}

void MinitoolsPlugin::slotItemSelected() {
  bool ok = false;
  int id = sender() ? qobject_cast<QAction*>(sender())->data().toInt(&ok) : 0;
  if (!ok)
     return;
  if (m_minitoolsList.count() == 0)
     return;
  QString tmp = m_minitoolsList[id-1].second;
  QString script = KUrl::decode_string(tmp.right(tmp.length() - 11)); // sizeof("javascript:")
  connect(this, SIGNAL( executeScript(const QString &) ),
          m_part, SLOT( executeScript(const QString &) ));
  emit executeScript(script);
  disconnect(this, SIGNAL( executeScript(const QString &) ), 0, 0);
}

#include "minitoolsplugin.moc"
