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

#include <QSet>
#include <QPointer>
#include <QStringList>

#include <kurl.h>
#include <kparts/plugin.h>
#include <kparts/listingextension.h>

class KUrl;
class KDirLister;
class KActionMenu;
class KFileItemList;

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
  void loadSettings ();
  void saveSettings ();

private:
  bool m_bSettingsLoaded;
  QMap<QString,QStringList> m_filters;
};


class DirFilterPlugin : public KParts::Plugin
{
  Q_OBJECT

public:

  DirFilterPlugin (QObject* parent, const QVariantList &);
  ~DirFilterPlugin ();

private Q_SLOTS:
  void slotReset();
  void slotOpenURL();
  void slotOpenURLCompleted();
  void slotShowPopup();
  void slotShowCount();
  void slotMultipleFilters();
  void slotItemSelected(QAction*);
  void slotListingEvent(KParts::ListingNotificationExtension::NotificationEventType, const KFileItemList&);

private:
  void itemsRemoved(const KFileItemList&);
  void itemsAdded(const KFileItemList&);

private:

  struct MimeInfo
  {
    MimeInfo() : action(0), useAsFilter(false) {}

    QAction *action;
    bool useAsFilter;

    QString iconName;
    QString mimeComment;

    QSet<QString> filenames;
  };

  QPointer<KParts::ReadOnlyPart> m_part;
  QPointer<KParts::ListingFilterExtension> m_listingExt;
  KActionMenu* m_pFilterMenu;

  typedef QMap<QString,MimeInfo> MimeInfoMap;
  MimeInfoMap m_pMimeInfo;
};
#endif
