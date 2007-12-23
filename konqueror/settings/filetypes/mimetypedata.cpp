/* This file is part of the KDE project
   Copyright (C) 2003 Waldo Bastian <bastian@kde.org>
   Copyright (C) 2003, 2007 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 or at your option version 3 as published by
   the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "mimetypedata.h"
#include <QFile>
#include <kstandarddirs.h>
#include <QXmlStreamWriter>
#include <kdebug.h>
#include <kservice.h>
#include <ksharedconfig.h>
#include <kconfiggroup.h>
#include <kmimetypetrader.h>
#include <kdesktopfile.h>

typedef QMap< QString, QStringList > ChangedServices;
K_GLOBAL_STATIC(ChangedServices, s_changedServices)

static MimeTypeData::AutoEmbed readAutoEmbed( KMimeType::Ptr mimetype )
{
    // TODO store this somewhere else!
    const QVariant v = mimetype->property( "X-KDE-AutoEmbed" );
    if ( v.isValid() )
        return (v.toBool() ? MimeTypeData::Yes : MimeTypeData::No);
    else if ( !mimetype->property( "X-KDE-LocalProtocol" ).toString().isEmpty() )
        return MimeTypeData::Yes; // embed by default for zip, tar etc.
    else
        return MimeTypeData::UseGroupSetting;
}

MimeTypeData::MimeTypeData(const QString& major)
    : m_askSave(2),
      m_bNewItem(false),
      m_bFullInit(true),
      m_isGroup(true),
      m_major(major)
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig("konquerorrc", KConfig::NoGlobals);
    bool defaultValue = defaultEmbeddingSetting( major );
    m_autoEmbed = config->group("EmbedSettings").readEntry( QLatin1String("embed-")+m_major, defaultValue ) ? Yes : No;
}

MimeTypeData::MimeTypeData(const KMimeType::Ptr mime, bool newItem)
    : m_mimetype(mime),
      m_askSave(2), // TODO: the code for initializing this is missing. FileTypeDetails initializes the checkbox instead...
      m_bNewItem(newItem),
      m_isGroup(false)
{
    m_bFullInit = false;
    const QString mimeName = m_mimetype->name();
    const int index = mimeName.indexOf('/');
    if (index != -1) {
        m_major = mimeName.left(index);
        m_minor = mimeName.mid(index+1);
    } else {
        m_major = mimeName;
    }
    m_comment = m_mimetype->comment();
    m_icon = m_mimetype->iconName();
    m_patterns = m_mimetype->patterns();
    m_autoEmbed = readAutoEmbed( m_mimetype );
}

bool MimeTypeData::defaultEmbeddingSetting( const QString& major )
{
    // embedding is false by default except for image/*
    return ( major=="image" );
}

bool MimeTypeData::isEssential() const
{
    // TODO resync with kmimetype.cpp
    const QString n = name();
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

void MimeTypeData::setIcon(const QString& icon)
{
    m_icon = icon;
}

void MimeTypeData::getServiceOffers(QStringList& appServices, QStringList& embedServices) const
{
    KService::List offerList =
        KMimeTypeTrader::self()->query(m_mimetype->name(), "Application");
    KService::List::const_iterator it(offerList.begin());
    for (; it != offerList.constEnd(); ++it)
        if ((*it)->allowAsDefault())
            appServices.append((*it)->entryPath());

    offerList = KMimeTypeTrader::self()->query(m_mimetype->name(), "KParts/ReadOnlyPart");
    for ( it = offerList.begin(); it != offerList.constEnd(); ++it)
        embedServices.append((*it)->entryPath());
}

void MimeTypeData::getMyServiceOffers() const
{
    getServiceOffers(m_appServices, m_embedServices);
    m_bFullInit = true;
}

QStringList MimeTypeData::appServices() const
{
    if (!m_bFullInit) {
        getMyServiceOffers();
    }
    return m_appServices;
}

QStringList MimeTypeData::embedServices() const
{
    if (!m_bFullInit) {
        getMyServiceOffers();
    }
    return m_embedServices;
}

bool MimeTypeData::isMimeTypeDirty() const
{
    Q_ASSERT(!m_isGroup);
    if ( m_bNewItem )
        return true;
    if ((m_mimetype->name() != name()) &&
        (name() != "application/octet-stream")) {
        kDebug() << "Mimetype Name Dirty: old=" << m_mimetype->name() << "name()=" << name();
        return true;
    }
    if (m_mimetype->comment() != m_comment) {
        kDebug() << "Mimetype Comment Dirty: old=" << m_mimetype->comment() << "m_comment=" << m_comment;
        return true;
    }
    if (m_mimetype->iconName() != m_icon) {
        kDebug() << "Mimetype Icon Dirty: old=" << m_mimetype->iconName() << "m_icon=" << m_icon;
        return true;
    }

    if (m_mimetype->patterns() != m_patterns) {
        //kDebug() << "Mimetype Patterns Dirty: old=" << m_mimetype->patterns()
        //         << "m_patterns=" << m_patterns;
        return true;
    }

    if ( readAutoEmbed( m_mimetype ) != m_autoEmbed )
        return true;
    return false;
}

bool MimeTypeData::isDirty() const
{
    if ( m_bNewItem ) {
        kDebug() << "New item, need to save it";
        return true;
    }

    if ( !m_isGroup ) {
        if (m_bFullInit) {
            QStringList oldAppServices;
            QStringList oldEmbedServices;
            getServiceOffers( oldAppServices, oldEmbedServices );

            if (oldAppServices != m_appServices) {
                kDebug() << "App Services Dirty: old=" << oldAppServices
                         << " m_appServices=" << m_appServices;
                return true;
            }
            if (oldEmbedServices != m_embedServices) {
                kDebug() << "Embed Services Dirty: old=" << oldEmbedServices
                         << " m_embedServices=" << m_embedServices;
                return true;
            }
        }
        if (isMimeTypeDirty())
            return true;
    }
    else // is a group
    {
        KSharedConfig::Ptr config = KSharedConfig::openConfig("konquerorrc", KConfig::NoGlobals);
        bool defaultValue = defaultEmbeddingSetting(m_major);
        AutoEmbed oldAutoEmbed = config->group("EmbedSettings").readEntry( QLatin1String("embed-")+m_major, defaultValue ) ? Yes : No;
        if ( m_autoEmbed != oldAutoEmbed )
            return true;
    }

    if (m_askSave != 2)
        return true;

    // nothing seems to have changed, it's not dirty.
    return false;
}

void MimeTypeData::sync()
{
    if (m_isGroup) {
        KSharedConfig::Ptr config = KSharedConfig::openConfig("konquerorrc", KConfig::NoGlobals);
        config->group("EmbedSettings").writeEntry( QLatin1String("embed-")+m_major, m_autoEmbed == Yes );
        return;
    }

    if (m_askSave != 2) {
        KSharedConfig::Ptr config = KSharedConfig::openConfig("konquerorrc", KConfig::NoGlobals);
        KConfigGroup cg = config->group("Notification Messages");
        if (m_askSave == 0) {
            // Ask
            cg.deleteEntry("askSave"+name());
            cg.deleteEntry("askEmbedOrSave"+name());
        } else {
            // Do not ask, open
            cg.writeEntry("askSave"+name(), "no" );
            cg.writeEntry("askEmbedOrSave"+name(), "no" );
        }
    }

    if (isMimeTypeDirty()) {
        // XDG shared mime: we must write into a <kdehome>/share/mime/packages/ file...
        // To simplify our job, let's use one "input" file per mimetype, in the user's dir.
        // TODO this writes into $HOME/.local/share/mime which makes the unit test mess up the user's configuration... can we avoid that? is $KDEHOME/share/mime available too?
        const QString packageFileName = KStandardDirs::locateLocal( "xdgdata-mime", "packages/" + m_major + '-' + m_minor + ".xml" );
        kDebug() << "writing" << packageFileName;
        QFile packageFile(packageFileName);
        if (!packageFile.open(QIODevice::WriteOnly)) {
            kError() << "Couldn't open" << packageFileName << "for writing";
            return;
        }
        QXmlStreamWriter writer(&packageFile);
        writer.setAutoFormatting(true);
        writer.writeStartDocument();
        const QString nsUri = "http://www.freedesktop.org/standards/shared-mime-info";
        writer.writeDefaultNamespace(nsUri);
        writer.writeStartElement("mime-info");
        writer.writeStartElement(nsUri, "mime-type");
        writer.writeAttribute("type", name());

        writer.writeStartElement(nsUri, "comment");
        writer.writeCharacters(m_comment);
        writer.writeEndElement(); // comment

        // TODO: we cannot write out the icon -> remove GUI for modifying the icon!
        //cg.writeEntry("Icon", m_icon);

        foreach(const QString& pattern, m_patterns) {
            writer.writeStartElement(nsUri, "glob");
            writer.writeAttribute("pattern", pattern);
            writer.writeEndElement(); // glob
        }

        // TODO store this somewhere else!
        //if ( m_autoEmbed == UseGroupSetting )
        //    cg.deleteEntry( QLatin1String("X-KDE-AutoEmbed"), false );
        //else
        //    cg.writeEntry( QLatin1String("X-KDE-AutoEmbed"), m_autoEmbed == Yes );

        writer.writeEndElement(); // mime-info
        writer.writeEndElement(); // mime-type
        writer.writeEndDocument();

        m_bNewItem = false;
    }

    if (!m_bFullInit)
        return;

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

    KService::List::const_iterator it_srv(offerList.begin());

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
                               ? (*s_changedServices)[ pService->entryPath() ] : group.readXdgListEntry("MimeType");

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

                group.writeXdgListEntry("MimeType", mimeTypeList);

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
    for(QStringList::const_iterator it = mimeTypeList.begin();
        it != mimeTypeList.end(); ++it) {
        if (m->is(*it))
            return true;
    }

    return false;
}

KMimeType::Ptr MimeTypeData::findImplicitAssociation(const QString &desktop)
{
    KService::Ptr s = KService::serviceByDesktopPath(desktop);
    if (!s) return KMimeType::Ptr(); // Hey, where did that one go?

    QStringList mimeTypeList = s_changedServices->contains( s->entryPath())
                               ? (*s_changedServices)[ s->entryPath() ] : s->serviceTypes();

    for(QStringList::const_iterator it = mimeTypeList.begin();
        it != mimeTypeList.end(); ++it)
    {
        if ((m_mimetype->name() != *it) && m_mimetype->is(*it))
        {
            return KMimeType::mimeType(*it);
        }
    }
    return KMimeType::Ptr();
}

void MimeTypeData::saveServices( KConfig & profile, const QStringList& services, const QString & genericServiceType )
{
    QStringList::const_iterator it(services.begin());
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
                           ? (*s_changedServices)[ pService->entryPath() ] : group.readXdgListEntry("MimeType");
            mimeTypeList.append(name());

            group.writeXdgListEntry("MimeType", mimeTypeList);
            desktop->sync();
            delete desktop;

            // if two or more types have been modified, and they use the same service,
            // accumulate the changes
            (*s_changedServices)[ pService->entryPath() ] = mimeTypeList;
        }
    }
}

void MimeTypeData::refresh()
{
    kDebug() << "MimeTypeData refresh" << name();
    m_mimetype = KMimeType::mimeType( name() );
}

void MimeTypeData::reset()
{
    s_changedServices->clear();
}

void MimeTypeData::getAskSave(bool &_askSave)
{
    if (m_askSave == 0)
        _askSave = true;
    if (m_askSave == 1)
        _askSave = false;
}

void MimeTypeData::setAskSave(bool _askSave)
{
    if (_askSave)
        m_askSave = 0;
    else
        m_askSave = 1;
}

bool MimeTypeData::canUseGroupSetting() const
{
    // "Use group settings" isn't available for zip, tar etc.; those have a builtin default...
    const bool hasLocalProtocolRedirect = !m_mimetype->property( "X-KDE-LocalProtocol" ).toString().isEmpty();
    return !hasLocalProtocolRedirect;
}
