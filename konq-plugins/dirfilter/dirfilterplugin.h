/*
   Copyright (C) 2000-2011 Dawit Alemayehu <adawit@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef DIR_FILTER_PLUGIN_H
#define DIR_FILTER_PLUGIN_H

#include <QtCore/QSet>
#include <QtCore/QStringList>

#include <kparts/plugin.h>

class KDirLister;
class KActionMenu;
class KUrl;
class KFileItem;

namespace KParts
{
    class ReadOnlyPart;
}

class SessionManager
{
public:
  SessionManager ();
  ~SessionManager ();
  QStringList restore (const KUrl& url);
  void save (const KUrl& url, const QStringList& filters);

  bool showCount;
  bool useMultipleFilters;

protected:
  QString generateKey (const KUrl& url);
  void loadSettings ();
  void saveSettings ();

private:
  int m_pid;
  bool m_bSettingsLoaded;
  QMap<QString,QStringList> m_filters;
};


class DirFilterPlugin : public KParts::Plugin
{
  Q_OBJECT

public:

  DirFilterPlugin (QObject* parent, const QVariantList &);
  ~DirFilterPlugin ();

protected:

  struct MimeInfo
  {
    MimeInfo() : action(0), useAsFilter(false) {}

    QAction *action;
    bool useAsFilter;

    QString mimeType;
    QString iconName;
    QString mimeComment;

    QSet<QString> filenames;
  };

  void loadSettings();
  void saveSettings();

private slots:
  void slotReset();
  void slotTimeout();
  void slotOpenURL();
  void slotShowPopup();
  void slotShowCount();
  void slotMultipleFilters();
  void slotItemSelected(QAction*);
  void slotItemRemoved(const KFileItem &);
  void slotItemsAdded(const KFileItemList &);

private:
  KUrl m_pURL;
  KParts::ReadOnlyPart* m_part;
  KActionMenu* m_pFilterMenu;
  KDirLister* m_dirLister;

  typedef QMap<QString,MimeInfo> MimeInfoMap;
  MimeInfoMap m_pMimeInfo;
};
#endif
