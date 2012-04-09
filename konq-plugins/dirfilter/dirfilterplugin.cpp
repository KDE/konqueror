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

#include <unistd.h>
#include <sys/types.h>

#include <qtimer.h>
#include <qapplication.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <khbox.h>
#include <kicon.h>

#include <kdebug.h>
#include <klocale.h>
#include <kcomponentdata.h>
#include <kactionmenu.h>

#include <kaction.h>
#include <kmenu.h>
#include <kmimetype.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <kdirsortfilterproxymodel.h>

#include <kdirlister.h>
#include <kpluginfactory.h>
#include <kparts/browserextension.h>
#include <kactioncollection.h>
#include "dirfilterplugin.h"
#include <kdirmodel.h>
#include <kconfiggroup.h>

K_GLOBAL_STATIC(SessionManager, globalSessionManager)

SessionManager::SessionManager()
{
  m_bSettingsLoaded = false;
  loadSettings ();
}

SessionManager::~SessionManager()
{
  saveSettings();
}

QString SessionManager::generateKey (const KUrl& url)
{
  QString key;

  key = url.protocol ();
  key += ':';

  if (!url.host ().isEmpty ())
  {
    key += url.host ();
    key += ':';
  }

  key += url.path ();
  key += ':';
  key += QString::number (m_pid);

  return key;
}

QStringList SessionManager::restore (const KUrl& url)
{
  QString key = generateKey (url);

  if (m_filters.contains(key))
    return m_filters[key];
  else
    return QStringList ();
}

void SessionManager::save (const KUrl& url, const QStringList& filters)
{
  QString key = generateKey(url);
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
  m_pid = getpid ();
  m_bSettingsLoaded = true;
}



DirFilterPlugin::DirFilterPlugin (QObject* parent,
                                  const QVariantList &)
                :KParts::Plugin (parent), m_pFilterMenu(0)
{
  m_part = qobject_cast<KParts::ReadOnlyPart*>(parent);

  if ( !m_part )
    return;

  m_dirLister = qFindChild<KDirLister*>(m_part);
  if (!m_dirLister && m_part->widget()) {
    m_dirLister = qFindChild<KDirLister*>(m_part->widget());
    if ( !m_dirLister )
      return;
  }

  m_pFilterMenu = new KActionMenu (KIcon("view-filter"),i18n("View F&ilter"),
                                   actionCollection());
  actionCollection()->addAction("filterdir", m_pFilterMenu);
  m_pFilterMenu->setDelayed (false);
  m_pFilterMenu->setWhatsThis(i18n("Allow to filter the currently displayed items by filetype."));

  connect (m_pFilterMenu->menu(), SIGNAL (aboutToShow()),
           SLOT (slotShowPopup()));
  connect(  m_pFilterMenu->menu(), SIGNAL(triggered(QAction*)),
           SLOT(slotItemSelected(QAction*)) );

  connect (m_dirLister, SIGNAL (deleteItem(KFileItem)),
           SLOT(slotItemRemoved(KFileItem)));
  connect (m_dirLister, SIGNAL (newItems(KFileItemList)),
           SLOT (slotItemsAdded(KFileItemList)));
  connect (m_dirLister, SIGNAL (itemsFilteredByMime(KFileItemList)),
           SLOT (slotItemsAdded(KFileItemList)));
  connect (m_part, SIGNAL(aboutToOpenURL()), SLOT(slotOpenURL()));
}

DirFilterPlugin::~DirFilterPlugin()
{
  delete m_pFilterMenu;
}

void DirFilterPlugin::slotOpenURL ()
{
  const KUrl url (m_part->url());

  //kDebug(90190) << "DirFilterPlugin: Current URL: " << m_pURL.url();

  if (m_pURL != url)
  {
    m_pURL = url;
    m_pMimeInfo.clear();
    m_dirLister->setMimeFilter (globalSessionManager->restore(url));
  }
}

void DirFilterPlugin::slotShowPopup()
{
  if (!m_part)
  {
    m_pFilterMenu->setEnabled (false);
    return;
  }

  uint enableReset = 0;

  QString label;
  QStringList inodes;

  m_pFilterMenu->menu()->clear();
  m_pFilterMenu->menu()->addTitle (i18n("Only Show Items of Type"));

  const MimeInfoMap::const_iterator itEnd = m_pMimeInfo.constEnd();
  for (MimeInfoMap::const_iterator it = m_pMimeInfo.constBegin(); it != itEnd ; ++it)
  {
    if (it.key().startsWith("inode"))
    {
      inodes << it.key();
      continue;
    }

    if (!globalSessionManager->showCount)
      label = it.value().mimeComment;
    else
    {
      label = it.value().mimeComment;
      label += "  (";
      label += QString::number (it.value().filenames.size ());
      label += ')';
    }

    QAction *action = m_pFilterMenu->menu()->addAction (
                               KIcon(it.value().iconName), label);
    action->setCheckable( true );
    if (it.value().useAsFilter)
    {
        action->setChecked( true );
        enableReset++;
    }
    m_pMimeInfo[it.key()].action = action;
  }

  // Add all the items that have mime-type of "inode/*" here...
  if (!inodes.isEmpty())
  {
    m_pFilterMenu->menu()->addSeparator ();

    Q_FOREACH(const QString& inode, inodes)
    {
      if (!globalSessionManager->showCount)
        label = m_pMimeInfo[inode].mimeComment;
      else
      {
        label = m_pMimeInfo[inode].mimeComment;
        label += "  (";
        label += QString::number (m_pMimeInfo[inode].filenames.size ());
        label += ')';
      }

      QAction *action = m_pFilterMenu->menu()->addAction (
                              KIcon(m_pMimeInfo[inode].iconName), label);
      action->setCheckable( true );
      if (m_pMimeInfo[inode].useAsFilter)
      {
        action->setChecked( true );
        enableReset ++;
      }
      m_pMimeInfo[inode].action = action;
    }
  }
  m_pFilterMenu->menu()->addSeparator ();
  QAction *action = m_pFilterMenu->menu()->addAction (i18n("Use Multiple Filters"),
                                               this, SLOT(slotMultipleFilters()));
  action->setEnabled( enableReset <= 1);
  action->setCheckable( true );
  action->setChecked( globalSessionManager->useMultipleFilters);

  action = m_pFilterMenu->menu()->addAction (i18n("Show Count"), this,
                                               SLOT(slotShowCount()));
  action->setCheckable( true );
  action->setChecked( globalSessionManager->showCount);

  action = m_pFilterMenu->menu()->addAction (i18n("Reset"), this,
                                               SLOT(slotReset()));
  action->setEnabled( enableReset);
}

void DirFilterPlugin::slotItemSelected (QAction *action)
{
  if (!m_part || !m_dirLister || !action)
    return;

  MimeInfoMap::iterator it = m_pMimeInfo.begin();
  const MimeInfoMap::iterator itEnd = m_pMimeInfo.end();
  while (it != itEnd && action != it.value().action)
    it++;

  if (it != itEnd)
  {
    MimeInfo& mimeInfo = it.value();
    QStringList filters;

    if (mimeInfo.useAsFilter)
    {
      mimeInfo.useAsFilter = false;
      filters = m_dirLister->mimeFilters();
      if (filters.removeAll(it.key()))
        m_dirLister->setMimeFilter (filters);
    }
    else
    {
      m_pMimeInfo[it.key()].useAsFilter = true;

      if (globalSessionManager->useMultipleFilters)
      {
        filters = m_dirLister->mimeFilters();
        filters << it.key();
      }
      else
      {
        filters << it.key();

        MimeInfoMap::iterator item = m_pMimeInfo.begin();
        const MimeInfoMap::iterator itemEnd = m_pMimeInfo.end();
        while ( item != itemEnd )
        {
          if ( item != it )
            item.value().useAsFilter = false;
          item++;
        }
      }

      m_dirLister->setMimeFilter (filters);
    }

    // We'd maybe benefit from an extra  Q_PROPERTY in the DolphinPart
    // for setting the mime filter, here. For now, just refresh
    // the model - refreshing the DolphinPart is more complex.
    const KUrl url (m_part->url());
    m_dirLister->openUrl (url);
    globalSessionManager->save (url, filters);
  }
}

void DirFilterPlugin::slotItemsAdded (const KFileItemList& list)
{
 if (list.count() == 0 || !m_dirLister || !m_dirLister->nameFilter().isEmpty())
  {
    if (m_dirLister) m_pFilterMenu->setEnabled (m_dirLister->nameFilter().isEmpty());
    return;
  }

  KUrl url = m_part->url();

  // Make sure the filter menu is enabled once a named
  // filter is removed.
  if (!m_pFilterMenu->isEnabled())
    m_pFilterMenu->setEnabled (true);

  const KFileItemList::const_iterator kend = list.end();
  for (KFileItemList::const_iterator kit = list.begin(); kit != kend; ++kit )
  {
    QString name = (*kit).name();
    KMimeType::Ptr mime = (*kit).mimeTypePtr(); // don't resolve mimetype if unknown
    if (!mime)
      continue;
    QString mimeType = mime->name();

    if (!m_pMimeInfo.contains (mimeType))
    {
      MimeInfo& mimeInfo = m_pMimeInfo[mimeType];
      QStringList filters = m_dirLister->mimeFilters();
      mimeInfo.useAsFilter = (!filters.isEmpty () &&
                              filters.contains (mimeType));
      mimeInfo.mimeComment = (*kit).mimeComment();
      mimeInfo.iconName = mime->iconName();
      mimeInfo.filenames.insert(name);
    }
    else
    {
      m_pMimeInfo[mimeType].filenames.insert(name);
    }
  }
}

void DirFilterPlugin::slotItemRemoved (const KFileItem& item)
{
  if (!m_dirLister)
    return;

  QString mimeType = item.mimetype().trimmed();
  MimeInfoMap::iterator it = m_pMimeInfo.find(mimeType);

  if (it != m_pMimeInfo.end())
  {
    MimeInfo& info = it.value();

    if (info.filenames.size () > 1) {
      info.filenames.remove(item.name ());
    } else {
      if (info.useAsFilter)
      {
        QStringList filters = m_dirLister->mimeFilters();
        filters.removeAll(mimeType);
        m_dirLister->setMimeFilter(filters);
        globalSessionManager->save (m_part->url(), filters);
        QTimer::singleShot( 0, this, SLOT(slotTimeout()) );
      }
      m_pMimeInfo.erase(it);
    }
  }
}

void DirFilterPlugin::slotReset()
{
  if (!m_part || !m_dirLister)
    return;

  const MimeInfoMap::iterator itEnd = m_pMimeInfo.end();
  for (MimeInfoMap::iterator it = m_pMimeInfo.begin(); it != itEnd; ++it)
    it.value().useAsFilter = false;

  const QStringList filters;
  const KUrl url (m_part->url());
  m_dirLister->setMimeFilter (filters);
  m_dirLister->openUrl (url);
  globalSessionManager->save (url, filters);
}

void DirFilterPlugin::slotShowCount()
{
  if (globalSessionManager->showCount)
    globalSessionManager->showCount = false;
  else
    globalSessionManager->showCount = true;
}

void DirFilterPlugin::slotMultipleFilters()
{
  globalSessionManager->useMultipleFilters = !globalSessionManager->useMultipleFilters;
}

void DirFilterPlugin::slotTimeout()
{
  if (m_dirLister)
    m_dirLister->openUrl (m_part->url());
}

K_PLUGIN_FACTORY(DirFilterFactory, registerPlugin<DirFilterPlugin>();)
K_EXPORT_PLUGIN(DirFilterFactory("dirfilterplugin"))

#include "dirfilterplugin.moc"

