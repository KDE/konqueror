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

#include "dirfilterplugin.h"

#include <kicon.h>
#include <kdebug.h>
#include <kmenu.h>
#include <kaction.h>
#include <klocale.h>
#include <kfileitem.h>
#include <kmimetype.h>
#include <kactionmenu.h>
#include <kconfiggroup.h>
#include <kcomponentdata.h>
#include <kpluginfactory.h>
#include <kactioncollection.h>

#include <kparts/browserextension.h>
#include <kparts/fileinfoextension.h>


K_GLOBAL_STATIC(SessionManager, globalSessionManager)

static QString generateKey(const KUrl& url)
{
  QString key;

  if (url.isValid()) {
    key = url.protocol();
    key += QLatin1Char(':');

    if (url.hasHost()) {
      key += url.host();
      key += QLatin1Char(':');
    }

    if (url.hasPath()) {
        key += url.path();
    }
  }

  return key;
}

SessionManager::SessionManager()
{
  m_bSettingsLoaded = false;
  loadSettings();
}

SessionManager::~SessionManager()
{
  saveSettings();
}

QStringList SessionManager::restore(const KUrl& url)
{
  const QString key(generateKey(url));

  if (m_filters.contains(key))
    return m_filters[key];

    return QStringList();
}

void SessionManager::save(const KUrl& url, const QStringList& filters)
{
  const QString key(generateKey(url));
  m_filters[key] = filters;
}

void SessionManager::saveSettings()
{
  KConfig cfg ("dirfilterrc", KConfig::NoGlobals);
  KConfigGroup group = cfg.group ("General");

  group.writeEntry ("ShowCount", showCount);
  group.writeEntry ("UseMultipleFilters", useMultipleFilters);
  cfg.sync();
}

void SessionManager::loadSettings()
{
  if (m_bSettingsLoaded)
    return;

  KConfig cfg ("dirfilterrc", KConfig::NoGlobals);
  KConfigGroup group = cfg.group ("General");

  showCount = group.readEntry ("ShowCount", false);
  useMultipleFilters = group.readEntry ("UseMultipleFilters", true);
  m_bSettingsLoaded = true;
}


DirFilterPlugin::DirFilterPlugin (QObject* parent, const QVariantList &)
    :KParts::Plugin (parent)
{
  m_part = qobject_cast<KParts::ReadOnlyPart*>(parent);
  if (m_part) {
      connect(m_part, SIGNAL(aboutToOpenURL()), this, SLOT(slotOpenURL()));
      connect(m_part, SIGNAL(completed()), this, SLOT(slotOpenURLCompleted()));
      connect(m_part, SIGNAL(completed(bool)), this, SLOT(slotOpenURLCompleted()));
  }

  KParts::ListingNotificationExtension* notifyExt = KParts::ListingNotificationExtension::childObject(m_part);
  if (notifyExt && notifyExt->supportedNotificationEventTypes() != KParts::ListingNotificationExtension::None) {
      m_listingExt = KParts::ListingFilterExtension::childObject(m_part);
      connect(notifyExt, SIGNAL(listingEvent(KParts::ListingNotificationExtension::NotificationEventType,KFileItemList)),
              this, SLOT(slotListingEvent(KParts::ListingNotificationExtension::NotificationEventType,KFileItemList)));

      m_pFilterMenu = new KActionMenu (KIcon("view-filter"), i18n("View F&ilter"), actionCollection());
      actionCollection()->addAction("filterdir", m_pFilterMenu);
      m_pFilterMenu->setDelayed(false);
      m_pFilterMenu->setDisabled(true);
      m_pFilterMenu->setWhatsThis(i18n("Allow to filter the currently displayed items by filetype."));
      connect(m_pFilterMenu->menu(), SIGNAL(aboutToShow()),
              this, SLOT(slotShowPopup()));
      connect(m_pFilterMenu->menu(), SIGNAL(triggered(QAction*)),
              this, SLOT(slotItemSelected(QAction*)));
  }
}

DirFilterPlugin::~DirFilterPlugin()
{
  delete m_pFilterMenu;
}

void DirFilterPlugin::slotOpenURL ()
{
  Q_ASSERT(m_part);
  m_pFilterMenu->setDisabled(true); // will be enabled after directory filtering is done.
  if (m_part && !m_part->arguments().reload()) {
    m_pMimeInfo.clear();
  }
}

void DirFilterPlugin::slotOpenURLCompleted()
{
  Q_ASSERT(m_part);
  if (m_listingExt && m_part && !m_part->arguments().reload()) {
    // Disable the mime-type filter if the name filter is set.
    m_pFilterMenu->setEnabled(m_listingExt->filter(KParts::ListingFilterExtension::SubString).toString().isEmpty());
    if (m_pFilterMenu->isEnabled()) {
      const QStringList filters = globalSessionManager->restore(m_part->url());
      m_listingExt->setFilter(KParts::ListingFilterExtension::MimeType, filters);
      Q_FOREACH(const QString& mimeType, filters) {
        if (m_pMimeInfo.contains(mimeType)) {
          m_pMimeInfo[mimeType].useAsFilter = true;
        }
      }
    }
  }
}

void DirFilterPlugin::slotShowPopup()
{
  // Not enabled return...
  if (!m_pFilterMenu->isEnabled()) {
      return;
  }

  quint64 enableReset = 0;

  m_pFilterMenu->menu()->clear();
  m_pFilterMenu->menu()->addTitle (i18n("Only Show Items of Type"));

  QString label;
  QStringList inodes;
  QMapIterator<QString, MimeInfo> it (m_pMimeInfo);

  while (it.hasNext()) {
    it.next();

    if (it.key().startsWith("inode")) {
      inodes << it.key();
      continue;
    }

    if (!globalSessionManager->showCount) {
      label = it.value().mimeComment;
    } else {
      label = it.value().mimeComment;
      label += "  (";
      label += QString::number (it.value().filenames.size ());
      label += ')';
    }

    QAction *action = m_pFilterMenu->menu()->addAction (
                               KIcon(it.value().iconName), label);
    action->setCheckable( true );
    if (it.value().useAsFilter) {
        action->setChecked( true );
        enableReset++;
    }
    m_pMimeInfo[it.key()].action = action;
  }

  // Add all the items that have mime-type of "inode/*" here...
  if (!inodes.isEmpty())
  {
    m_pFilterMenu->menu()->addSeparator ();

    Q_FOREACH(const QString& inode, inodes) {
      if (!globalSessionManager->showCount)
        label = m_pMimeInfo[inode].mimeComment;
      else {
        label = m_pMimeInfo[inode].mimeComment;
        label += "  (";
        label += QString::number (m_pMimeInfo[inode].filenames.size ());
        label += ')';
      }

      QAction *action = m_pFilterMenu->menu()->addAction (
                              KIcon(m_pMimeInfo[inode].iconName), label);
      action->setCheckable(true);
      if (m_pMimeInfo[inode].useAsFilter) {
        action->setChecked(true);
        enableReset ++;
      }
      m_pMimeInfo[inode].action = action;
    }
  }
  m_pFilterMenu->menu()->addSeparator ();
  QAction *action = m_pFilterMenu->menu()->addAction(i18n("Use Multiple Filters"),
                                                     this, SLOT(slotMultipleFilters()));
  action->setEnabled(enableReset <= 1);
  action->setCheckable(true);
  action->setChecked(globalSessionManager->useMultipleFilters);

  action = m_pFilterMenu->menu()->addAction(i18n("Show Count"), this,
                                            SLOT(slotShowCount()));
  action->setCheckable(true);
  action->setChecked(globalSessionManager->showCount);

  action = m_pFilterMenu->menu()->addAction(i18n("Reset"), this,
                                            SLOT(slotReset()));
  action->setEnabled(enableReset);
}

void DirFilterPlugin::slotItemSelected (QAction *action)
{
  if (!m_listingExt || !action || !m_part)
    return;

  MimeInfoMap::iterator it = m_pMimeInfo.begin();
  MimeInfoMap::const_iterator itEnd = m_pMimeInfo.end();
  while (it != itEnd && action != it.value().action)
    it++;

  if (it != itEnd) {
    MimeInfo& mimeInfo = it.value();
    QStringList filters;

    if (mimeInfo.useAsFilter){
      mimeInfo.useAsFilter = false;
      filters = m_listingExt->filter(KParts::ListingFilterExtension::MimeType).toStringList();
      if (filters.removeAll(it.key()))
        m_listingExt->setFilter (KParts::ListingFilterExtension::MimeType, filters);
    } else {
      m_pMimeInfo[it.key()].useAsFilter = true;

      if (globalSessionManager->useMultipleFilters) {
        filters = m_listingExt->filter(KParts::ListingFilterExtension::MimeType).toStringList();
        filters << it.key();
      } else {
        filters << it.key();

        MimeInfoMap::iterator item = m_pMimeInfo.begin();
        MimeInfoMap::const_iterator itemEnd = m_pMimeInfo.end();
        while ( item != itemEnd )
        {
          if ( item != it )
            item.value().useAsFilter = false;
          item++;
        }
      }
      m_listingExt->setFilter(KParts::ListingFilterExtension::MimeType, filters);
    }

    globalSessionManager->save (m_part->url(), filters);
  }
}

void DirFilterPlugin::slotListingEvent(KParts::ListingNotificationExtension::NotificationEventType type, const KFileItemList& items)
{
    switch (type) {
      case KParts::ListingNotificationExtension::ItemsAdded:
          itemsAdded(items);
          break;
      case KParts::ListingNotificationExtension::ItemsDeleted:
          itemsRemoved(items);
          break;
      default:
          break;
    }

}

void DirFilterPlugin::itemsAdded (const KFileItemList& list)
{
  if (list.count() == 0 || !m_listingExt) {
    if (m_listingExt) {
        m_pFilterMenu->setEnabled(m_listingExt->filter(KParts::ListingFilterExtension::SubString).toString().isEmpty());
    }
    return;
  }

  // Make sure the filter menu is enabled once a named
  // filter is removed.
  if (!m_pFilterMenu->isEnabled())
    m_pFilterMenu->setEnabled (true);

  Q_FOREACH (const KFileItem& item, list) {
    const QString mimeType (item.mimetype());

    if (!m_pMimeInfo.contains (mimeType)) {
      MimeInfo& mimeInfo = m_pMimeInfo[mimeType];
      const QStringList filters = m_listingExt->filter(KParts::ListingFilterExtension::MimeType).toStringList();
      mimeInfo.useAsFilter = (!filters.isEmpty () &&
                              filters.contains (mimeType));
      mimeInfo.mimeComment = item.mimeComment();
      mimeInfo.iconName = item.iconName();
      mimeInfo.filenames.insert(item.name());
    } else {
      m_pMimeInfo[mimeType].filenames.insert(item.name());
    }
  }
}

void DirFilterPlugin::itemsRemoved (const KFileItemList& list)
{
  if (!m_listingExt || !m_part)
    return;

  Q_FOREACH(const KFileItem& item, list) {
    const QString mimeType (item.mimetype());
    MimeInfoMap::iterator it = m_pMimeInfo.find(mimeType);
    if (it != m_pMimeInfo.end()) {
      MimeInfo& info = it.value();

      if (info.filenames.size () > 1) {
        info.filenames.remove(item.name ());
      } else {
        if (info.useAsFilter) {
            QStringList filters = m_listingExt->filter(KParts::ListingFilterExtension::MimeType).toStringList();
            filters.removeAll(mimeType);
            m_listingExt->setFilter(KParts::ListingFilterExtension::MimeType, filters);
            globalSessionManager->save (m_part->url(), filters);
        }
        m_pMimeInfo.erase(it);
      }
    }
  }
}

void DirFilterPlugin::slotReset()
{
  if (!m_part || !m_listingExt)
    return;

  MimeInfoMap::const_iterator itEnd = m_pMimeInfo.end();
  for (MimeInfoMap::iterator it = m_pMimeInfo.begin(); it != itEnd; ++it)
    it.value().useAsFilter = false;

  const QStringList filters;
  m_listingExt->setFilter(KParts::ListingFilterExtension::MimeType, filters);
  globalSessionManager->save (m_part->url(), filters);
}

void DirFilterPlugin::slotShowCount()
{
  globalSessionManager->showCount = !globalSessionManager->showCount;
}

void DirFilterPlugin::slotMultipleFilters()
{
  globalSessionManager->useMultipleFilters = !globalSessionManager->useMultipleFilters;
}

K_PLUGIN_FACTORY(DirFilterFactory, registerPlugin<DirFilterPlugin>();)
K_EXPORT_PLUGIN(DirFilterFactory("dirfilterplugin"))

#include "dirfilterplugin.moc"
