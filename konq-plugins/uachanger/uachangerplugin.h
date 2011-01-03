/*
    Copyright (c) 2001 Dawit Alemayehu <adawit@kde.org>

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

#ifndef __UACHANGER_PLUGIN_H
#define __UACHANGER_PLUGIN_H

#include <qmap.h>
#include <qstringlist.h>

#include <kurl.h>
#include <klibloader.h>
#include <kparts/plugin.h>

class KActionMenu;
class QAction;
class QActionGroup;
class KConfig;

namespace KIO
{
  class Job;
}

class UAChangerPlugin : public KParts::Plugin
{
  Q_OBJECT

public:
  explicit UAChangerPlugin( QObject* parent,
                   const QVariantList & );
  ~UAChangerPlugin();

protected slots:
  void slotDefault();
  void parseDescFiles();

  void slotConfigure();
  void slotAboutToShow();
  void slotApplyToDomain();
  void slotEnableMenu();
  void slotItemSelected(QAction*);
  void slotReloadDescriptions();

protected:
  QString findTLD (const QString &hostname);
  QString filterHost (const QString &hostname);

private:
  void reloadPage();
  void loadSettings();
  void saveSettings();

  int m_selectedItem;
  bool m_bApplyToDomain;
  bool m_bSettingsLoaded;

  KParts::ReadOnlyPart* m_part;
  KActionMenu* m_pUAMenu;
  KConfig* m_config;
  QAction* m_applyEntireSiteAction;
  QAction* m_defaultAction;
  QActionGroup* m_actionGroup;

  KUrl m_currentURL;
  QString m_currentUserAgent;

  QStringList m_lstAlias;    // menu entry names
  QStringList m_lstIdentity; // UA strings

  // A little wrapper around tag names so that other always goes to the end.
  struct MenuGroupSortKey {
    QString tag;
    bool    isOther;
    MenuGroupSortKey(): isOther(false) {}
    MenuGroupSortKey(const QString& t, bool oth): tag(t), isOther(oth) {}

    bool operator==(const MenuGroupSortKey& o) const {
      return tag == o.tag && isOther == o.isOther;
    }

    bool operator<(const MenuGroupSortKey& o) const {
      return (!isOther && o.isOther) || (tag < o.tag);
    }
  };

  typedef QList<int> BrowserGroup;
  typedef QMap<MenuGroupSortKey,BrowserGroup> AliasMap;
  typedef QMap<MenuGroupSortKey,QString> BrowserMap;

  typedef AliasMap::Iterator AliasIterator;
  typedef AliasMap::ConstIterator AliasConstIterator;

  BrowserMap m_mapBrowser; // tag -> menu name
  AliasMap m_mapAlias;     // tag -> UA string/menu entry name indices.
};

#endif
