/* This file is part of the KDE project
   Copyright (C) 2003 Waldo Bastian <bastian@kde.org>
   Copyright (C) 2003 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

// Own
#include "typeslistitem.h"

// KDE
#include <ksharedconfig.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kstandarddirs.h>
#include <kdesktopfile.h>
#include <kmimetypetrader.h>
#include <k3staticdeleter.h>


QMap< QString, QStringList >* TypesListItem::s_changedServices;
static K3StaticDeleter< QMap< QString, QStringList > > deleter;

TypesListItem::TypesListItem(Q3ListView *parent, const QString & major)
  : Q3ListViewItem(parent), metaType(true), m_bNewItem(false), m_askSave(2)
{
  initMeta(major);
  setText(0, majorType());
}

TypesListItem::TypesListItem(TypesListItem *parent, KMimeType::Ptr mimetype, bool newItem)
  : Q3ListViewItem(parent), metaType(false), m_bNewItem(newItem), m_askSave(2)
{
  init(mimetype);
  setText(0, minorType());
}


TypesListItem::TypesListItem(Q3ListView *parent, KMimeType::Ptr mimetype, bool newItem)
  : Q3ListViewItem(parent), metaType(false), m_bNewItem(newItem), m_askSave(2)
{
  init(mimetype);
  setText(0, majorType());
}

TypesListItem::~TypesListItem()
{
}

void TypesListItem::initMeta( const QString & major )
{
  m_bFullInit = true;
  m_mimetype = 0L;
  m_major = major;
  KSharedConfig::Ptr config = KSharedConfig::openConfig("konquerorrc", KConfig::NoGlobals);
  bool defaultValue = defaultEmbeddingSetting( major );
  m_autoEmbed = config->group("EmbedSettings").readEntry( QLatin1String("embed-")+m_major, defaultValue ) ? 0 : 1;
}

bool TypesListItem::defaultEmbeddingSetting( const QString& major )
{
  // embedding is false by default except for image/*
  return ( major=="image" );
}

void TypesListItem::setup()
{
  if (m_mimetype)
  {
     setPixmap(0, KIconLoader::global()->loadMimeTypeIcon(m_mimetype->iconName(),K3Icon::Small, IconSize(K3Icon::Small)));
  }
  Q3ListViewItem::setup();
}

void TypesListItem::init(KMimeType::Ptr mimetype)
{
  m_bFullInit = false;
  m_mimetype = mimetype;

  int index = mimetype->name().indexOf("/");
  if (index != -1) {
    m_major = mimetype->name().left(index);
    m_minor = mimetype->name().right(mimetype->name().length() -
                                     (index+1));
  } else {
    m_major = mimetype->name();
    m_minor = "";
  }
  m_comment = mimetype->comment(QString());
  m_icon = mimetype->iconName(QString());
  m_patterns = mimetype->patterns();
  m_autoEmbed = readAutoEmbed( mimetype );
}

int TypesListItem::readAutoEmbed( KMimeType::Ptr mimetype )
{
  QVariant v = mimetype->property( "X-KDE-AutoEmbed" );
  if ( v.isValid() )
      return (v.toBool() ? 0 : 1);
  else if ( !mimetype->property( "X-KDE-LocalProtocol" ).toString().isEmpty() )
      return 0; // embed by default for zip, tar etc.
  else
      return 2;
}

QStringList TypesListItem::appServices() const
{
  if (!m_bFullInit)
  {
     TypesListItem *that = const_cast<TypesListItem *>(this);
     that->getServiceOffers(that->m_appServices, that->m_embedServices);
     that->m_bFullInit = true;
  }
  return m_appServices;
}

QStringList TypesListItem::embedServices() const
{
  if (!m_bFullInit)
  {
     TypesListItem *that = const_cast<TypesListItem *>(this);
     that->getServiceOffers(that->m_appServices, that->m_embedServices);
     that->m_bFullInit = true;
  }
  return m_embedServices;
}

void TypesListItem::getServiceOffers( QStringList & appServices, QStringList & embedServices ) const
{
  KService::List offerList =
    KMimeTypeTrader::self()->query(m_mimetype->name(), "Application");
  KService::List::ConstIterator it(offerList.begin());
  for (; it != offerList.constEnd(); ++it)
    if ((*it)->allowAsDefault())
      appServices.append((*it)->entryPath());

  offerList = KMimeTypeTrader::self()->query(m_mimetype->name(), "KParts/ReadOnlyPart");
  for ( it = offerList.begin(); it != offerList.constEnd(); ++it)
    embedServices.append((*it)->entryPath());
}

bool TypesListItem::isMimeTypeDirty() const
{
  if ( m_bNewItem )
    return true;
  if ((m_mimetype->name() != name()) &&
      (name() != "application/octet-stream"))
  {
    kDebug() << "Mimetype Name Dirty: old=" << m_mimetype->name() << " name()=" << name();
    return true;
  }
  if (m_mimetype->comment(QString()) != m_comment)
  {
    kDebug() << "Mimetype Comment Dirty: old=" << m_mimetype->comment(QString()) << " m_comment=" << m_comment;
    return true;
  }
  if (m_mimetype->iconName(QString()) != m_icon)
  {
    kDebug() << "Mimetype Icon Dirty: old=" << m_mimetype->iconName(QString()) << " m_icon=" << m_icon;
    return true;
  }

  if (m_mimetype->patterns() != m_patterns)
  {
    kDebug() << "Mimetype Patterns Dirty: old=" << m_mimetype->patterns().join(";")
              << " m_patterns=" << m_patterns.join(";") << endl;
    return true;
  }

  if ( readAutoEmbed( m_mimetype ) != (int)m_autoEmbed )
    return true;
  return false;
}

bool TypesListItem::isDirty() const
{
  if ( !m_bFullInit)
  {
    return false;
  }

  if ( m_bNewItem )
  {
    kDebug() << "New item, need to save it";
    return true;
  }

  if ( !isMeta() )
  {
    QStringList oldAppServices;
    QStringList oldEmbedServices;
    getServiceOffers( oldAppServices, oldEmbedServices );

    if (oldAppServices != m_appServices)
    {
      kDebug() << "App Services Dirty: old=" << oldAppServices.join(";")
                << " m_appServices=" << m_appServices.join(";") << endl;
      return true;
    }
    if (oldEmbedServices != m_embedServices)
    {
      kDebug() << "Embed Services Dirty: old=" << oldEmbedServices.join(";")
                << " m_embedServices=" << m_embedServices.join(";") << endl;
      return true;
    }
    if (isMimeTypeDirty())
      return true;
  }
  else
  {
    KSharedConfig::Ptr config = KSharedConfig::openConfig("konquerorrc", KConfig::NoGlobals);
    bool defaultValue = defaultEmbeddingSetting(m_major);
    unsigned int oldAutoEmbed = config->group("EmbedSettings").readEntry( QLatin1String("embed-")+m_major, defaultValue ) ? 0 : 1;
    if ( m_autoEmbed != oldAutoEmbed )
      return true;
  }

  if (m_askSave != 2)
    return true;

  // nothing seems to have changed, it's not dirty.
  return false;
}

void TypesListItem::sync()
{
  Q_ASSERT(m_bFullInit);
  if ( isMeta() )
  {
    KSharedConfig::Ptr config = KSharedConfig::openConfig("konquerorrc", KConfig::NoGlobals);
    config->group("EmbedSettings").writeEntry( QLatin1String("embed-")+m_major, m_autoEmbed == 0 );
    return;
  }

  if (m_askSave != 2)
  {
    KSharedConfig::Ptr config = KSharedConfig::openConfig("konquerorrc", KConfig::NoGlobals);
    KConfigGroup cg = config->group("Notification Messages");
    if (m_askSave == 0)
    {
       // Ask
       cg.deleteEntry("askSave"+name());
       cg.deleteEntry("askEmbedOrSave"+name());
    }
    else
    {
       // Do not ask, open
       cg.writeEntry("askSave"+name(), "no" );
       cg.writeEntry("askEmbedOrSave"+name(), "no" );
    }
  }

  if (isMimeTypeDirty())
  {
    // We must use KConfig otherwise config.deleteEntry doesn't
    // properly cancel out settings already present in system files.
    KDesktopFile config( "mime", m_mimetype->entryPath() );
    KConfigGroup cg = config.desktopGroup();

    cg.writeEntry("Type", "MimeType");
    cg.writeEntry("MimeType", name());
    cg.writeEntry("Icon", m_icon);
    cg.writeEntry("Patterns", m_patterns, ';');
    cg.writeEntry("Comment", m_comment);
    cg.writeEntry("Hidden", false);

    if ( m_autoEmbed == 2 )
      cg.deleteEntry( QLatin1String("X-KDE-AutoEmbed"), false );
    else
      cg.writeEntry( QLatin1String("X-KDE-AutoEmbed"), m_autoEmbed == 0 );

    m_bNewItem = false;
  }

  KConfig profile("profilerc", KConfig::NoGlobals);

  // Deleting current contents in profilerc relating to
  // this service type
  //
  const QStringList groups = profile.groupList();
  QStringList::const_iterator it;
  for (it = groups.begin(); it != groups.end(); it++ )
  {
      KConfigGroup group = profile.group( *it );

    // Entries with Preference <= 0 or AllowAsDefault == false
    // are not in m_services
    if ( group.readEntry( "ServiceType" ) == name()
         && group.readEntry( "Preference" ) > 0
         && group.readEntry( "AllowAsDefault",false ) )
    {
      group.deleteGroup();
    }
  }

  // Save preferred services
  //

  groupCount = 1;

  saveServices( profile, m_appServices, "Application" );
  saveServices( profile, m_embedServices, "KParts/ReadOnlyPart" );

  // Handle removed services

  KService::List offerList =
    KMimeTypeTrader::self()->query(m_mimetype->name(), "Application");
  offerList += KMimeTypeTrader::self()->query(m_mimetype->name(), "KParts/ReadOnlyPart");

  KService::List::ConstIterator it_srv(offerList.begin());

  for (; it_srv != offerList.end(); ++it_srv) {
      KService::Ptr pService = (*it_srv);

      bool isApplication = pService->isApplication();
      if (isApplication && !pService->allowAsDefault())
          continue; // Only those which were added in init()

      // Look in the correct list...
      if ( (isApplication && ! m_appServices.contains( pService->entryPath() ))
           || (!isApplication && !m_embedServices.contains( pService->entryPath() ))
          ) {
        // The service was in m_appServices but has been removed
        // create a new .desktop file without this mimetype

        if( s_changedServices == NULL )
            deleter.setObject( s_changedServices, new QMap< QString, QStringList > );
        QStringList mimeTypeList = s_changedServices->contains( pService->entryPath())
            ? (*s_changedServices)[ pService->entryPath() ] : pService->serviceTypes();

        if ( mimeTypeList.contains( name() ) ) {
          // The mimetype is listed explicitly in the .desktop files, so
          // just remove it and we're done
          KDesktopFile *desktop;
          if ( !isApplication )
          {
            desktop = new KDesktopFile("services", pService->entryPath());
          }
          else
          {
            QString path = pService->locateLocal();
            KDesktopFile orig("apps", pService->entryPath());
            desktop = orig.copyTo(path);
          }

          KConfigGroup group = desktop->desktopGroup();
          mimeTypeList = s_changedServices->contains( pService->entryPath())
            ? (*s_changedServices)[ pService->entryPath() ] : group.readEntry("MimeType",QStringList(), ';');

          // Remove entry and the number that might follow.
          for(int i=0;(i = mimeTypeList.indexOf(name())) != -1;)
          {
             QStringList::iterator it = mimeTypeList.begin()+i;
             it = mimeTypeList.erase(it);
             if (it != mimeTypeList.end())
             {
               // Check next item
               bool numeric;
               (*it).toInt(&numeric);
               if (numeric)
                  mimeTypeList.erase(it);
             }
          }

          group.writeEntry("MimeType", mimeTypeList, ';');

          // if two or more types have been modified, and they use the same service,
          // accumulate the changes
          (*s_changedServices)[ pService->entryPath() ] = mimeTypeList;

          desktop->sync();
          delete desktop;
        }
        else {
          // The mimetype is not listed explicitly so it can't
          // be removed. Preference = 0 handles this.

          // Find a group header. The headers are just dummy names as far as
          // KUserProfile is concerned, but using the mimetype makes it a
          // bit more structured for "manual" reading
          while ( profile.hasGroup(
                  name() + " - " + QString::number(groupCount) ) )
              groupCount++;

          KConfigGroup cg = profile.group( name() + " - " + QString::number(groupCount) );

          cg.writeEntry("Application", pService->storageId());
          cg.writeEntry("ServiceType", name());
          cg.writeEntry("AllowAsDefault", true);
          cg.writeEntry("Preference", 0);
        }
      }
  }
}

static bool inheritsMimetype(KMimeType::Ptr m, const QStringList &mimeTypeList)
{
  for(QStringList::ConstIterator it = mimeTypeList.begin();
      it != mimeTypeList.end(); ++it)
  {
    if (m->is(*it))
       return true;
  }

  return false;
}

KMimeType::Ptr TypesListItem::findImplicitAssociation(const QString &desktop)
{
    KService::Ptr s = KService::serviceByDesktopPath(desktop);
    if (!s) return KMimeType::Ptr(); // Hey, where did that one go?

    if( s_changedServices == NULL )
       deleter.setObject( s_changedServices, new QMap< QString, QStringList > );
    QStringList mimeTypeList = s_changedServices->contains( s->entryPath())
       ? (*s_changedServices)[ s->entryPath() ] : s->serviceTypes();

    for(QStringList::ConstIterator it = mimeTypeList.begin();
       it != mimeTypeList.end(); ++it)
    {
       if ((m_mimetype->name() != *it) && m_mimetype->is(*it))
       {
          return KMimeType::mimeType(*it);
       }
    }
    return KMimeType::Ptr();
}

void TypesListItem::saveServices( KConfig & profile, QStringList services, const QString & genericServiceType )
{
  QStringList::Iterator it(services.begin());
  for (int i = services.count(); it != services.end(); ++it, i--) {

    KService::Ptr pService = KService::serviceByDesktopPath(*it);
    if (!pService) continue; // Where did that one go?

    // Find a group header. The headers are just dummy names as far as
    // KUserProfile is concerned, but using the mimetype makes it a
    // bit more structured for "manual" reading
    while ( profile.hasGroup( name() + " - " + QString::number(groupCount) ) )
        groupCount++;

    KConfigGroup group = profile.group( name() + " - " + QString::number(groupCount) );

    group.writeEntry("ServiceType", name());
    group.writeEntry("GenericServiceType", genericServiceType);
    group.writeEntry("Application", pService->storageId());
    group.writeEntry("AllowAsDefault", true);
    group.writeEntry("Preference", i);

    // merge new mimetype
    if( s_changedServices == NULL )
       deleter.setObject( s_changedServices, new QMap< QString, QStringList > );
    QStringList mimeTypeList = s_changedServices->contains( pService->entryPath())
       ? (*s_changedServices)[ pService->entryPath() ] : pService->serviceTypes();

    if (!mimeTypeList.contains(name()) && !inheritsMimetype(m_mimetype, mimeTypeList))
    {
      KDesktopFile *desktop;
      if ( !pService->isApplication() )
      {
        desktop = new KDesktopFile("services", pService->entryPath());
      }
      else
      {
        QString path = pService->locateLocal();
        KDesktopFile orig("apps", pService->entryPath());
        desktop = orig.copyTo(path);
      }

      KConfigGroup group = desktop->desktopGroup();
      mimeTypeList = s_changedServices->contains( pService->entryPath())
            ? (*s_changedServices)[ pService->entryPath() ] : group.readEntry("MimeType", QStringList(),';');
      mimeTypeList.append(name());

      group.writeEntry("MimeType", mimeTypeList, ';');
      desktop->sync();
      delete desktop;

      // if two or more types have been modified, and they use the same service,
      // accumulate the changes
      (*s_changedServices)[ pService->entryPath() ] = mimeTypeList;
    }
  }
}

void TypesListItem::setIcon( const QString& icon )
{
  m_icon = icon;
  setPixmap( 0, SmallIcon( icon ) );
}

bool TypesListItem::isEssential() const
{
    QString n = name();
    if ( n == "application/octet-stream" )
        return true;
    if ( n == "inode/directory" )
        return true;
    if ( n == "inode/directory-locked" )
        return true;
    if ( n == "inode/blockdevice" )
        return true;
    if ( n == "inode/chardevice" )
        return true;
    if ( n == "inode/socket" )
        return true;
    if ( n == "inode/fifo" )
        return true;
    if ( n == "application/x-shellscript" )
        return true;
    if ( n == "application/x-executable" )
        return true;
    if ( n == "application/x-desktop" )
        return true;
    return false;
}

void TypesListItem::refresh()
{
    kDebug() << "TypesListItem refresh " << name();
    m_mimetype = KMimeType::mimeType( name() );
}

void TypesListItem::reset()
{
    if( s_changedServices )
        s_changedServices->clear();
}

void TypesListItem::getAskSave(bool &_askSave)
{
    if (m_askSave == 0)
       _askSave = true;
    if (m_askSave == 1)
       _askSave = false;
}

void TypesListItem::setAskSave(bool _askSave)
{
   if (_askSave)
      m_askSave = 0;
   else
      m_askSave = 1;
}

bool TypesListItem::canUseGroupSetting() const
{
  // "Use group settings" isn't available for zip, tar etc.; those have a builtin default...
    bool hasLocalProtocolRedirect = !m_mimetype->property( "X-KDE-LocalProtocol" ).toString().isEmpty();
    return !hasLocalProtocolRedirect;
}

