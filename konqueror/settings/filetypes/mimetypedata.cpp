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
#include "mimetypewriter.h"
#include <kdebug.h>
#include <kservice.h>
#include <ksharedconfig.h>
#include <kconfiggroup.h>
#include <kmimetypetrader.h>
#include <kdesktopfile.h>

typedef QMap< QString, QStringList > ChangedServices;
K_GLOBAL_STATIC(ChangedServices, s_changedServices)

MimeTypeData::MimeTypeData(const QString& major)
    : m_askSave(2),
      m_bNewItem(false),
      m_bFullInit(true),
      m_isGroup(true),
      m_major(major)
{
    m_autoEmbed = readAutoEmbed();
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
    setPatterns(m_mimetype->patterns());
    m_autoEmbed = readAutoEmbed();
}

MimeTypeData::AutoEmbed MimeTypeData::readAutoEmbed() const
{
    const KSharedConfig::Ptr config = KSharedConfig::openConfig("filetypesrc", KConfig::NoGlobals);
    const QString key = QString("embed-") + name();
    const KConfigGroup group(config, "EmbedSettings");
    if (m_isGroup) {
        // embedding is false by default except for image/* and inode/* (hardcoded in konq)
        const bool defaultValue = ( m_major == "image" || m_major == "inode" );
        return group.readEntry(key, defaultValue) ? Yes : No;
    } else {
        if (group.hasKey(key))
            return group.readEntry(key, false) ? Yes : No;
        // TODO if ( !mimetype->property( "X-KDE-LocalProtocol" ).toString().isEmpty() )
        // TODO    return MimeTypeData::Yes; // embed by default for zip, tar etc.
        return MimeTypeData::UseGroupSetting;
    }
}

void MimeTypeData::writeAutoEmbed()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig("filetypesrc", KConfig::NoGlobals);
    const QString key = QString("embed-") + name();
    KConfigGroup group(config, "EmbedSettings");
    if (m_isGroup) {
        group.writeEntry(key, m_autoEmbed == Yes);
    } else {
        if (m_autoEmbed == UseGroupSetting)
            group.deleteEntry(key);
        else
            group.writeEntry(key, m_autoEmbed == Yes);
    }
}

bool MimeTypeData::isEssential() const
{
    // Keep in sync with KMimeTYpe::checkEssentialMimeTypes
    const QString n = name();
    if ( n == "application/octet-stream" )
        return true;
    if ( n == "inode/directory" )
        return true;
    if ( n == "inode/directory-locked" ) // doesn't exist anymore, but the check doesn't hurt :)
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

#if 0
void MimeTypeData::setIcon(const QString& icon)
{
    m_icon = icon;
}
#endif

void MimeTypeData::getServiceOffers(QStringList& appServices, QStringList& embedServices) const
{
    const KService::List offerList =
        KMimeTypeTrader::self()->query(m_mimetype->name(), "Application");
    KService::List::const_iterator it(offerList.begin());
    for (; it != offerList.constEnd(); ++it)
        if ((*it)->allowAsDefault())
            appServices.append((*it)->entryPath());

    const KService::List partOfferList =
        KMimeTypeTrader::self()->query(m_mimetype->name(), "KParts/ReadOnlyPart");
    for ( it = partOfferList.begin(); it != partOfferList.constEnd(); ++it)
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

    QStringList storedPatterns = m_mimetype->patterns();
    storedPatterns.sort(); // see ctor
    if ( storedPatterns != m_patterns) {
        kDebug() << "Mimetype Patterns Dirty: old=" << storedPatterns
                 << "m_patterns=" << m_patterns;
        return true;
    }

    if (readAutoEmbed() != m_autoEmbed)
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
        if (areServicesDirty())
            return true;
        if (isMimeTypeDirty())
            return true;
    }
    else // is a group
    {
        if (readAutoEmbed() != m_autoEmbed)
            return true;
    }

    if (m_askSave != 2)
        return true;

    // nothing seems to have changed, it's not dirty.
    return false;
}

bool MimeTypeData::sync()
{
    if (m_isGroup) {
        writeAutoEmbed();
        return false;
    }

    if (m_askSave != 2) {
        const KSharedConfig::Ptr config = KSharedConfig::openConfig("filetypesrc", KConfig::NoGlobals);
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

    writeAutoEmbed();

    bool needUpdateMimeDb = false;
    if (isMimeTypeDirty()) {
        MimeTypeWriter mimeTypeWriter(name());
        mimeTypeWriter.setComment(m_comment);
        mimeTypeWriter.setPatterns(m_patterns);
        if (!mimeTypeWriter.write())
            return false;

        m_bNewItem = false;
        needUpdateMimeDb = true;
    }

    if (areServicesDirty()) {
        syncServices();
    }

    return needUpdateMimeDb;
}

void MimeTypeData::syncServices()
{
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
    QStringList mimeTypeList;
    if (s_changedServices->contains(desktop)) {
        mimeTypeList = s_changedServices->value( desktop );
    } else {
        KService::Ptr s = KService::serviceByDesktopPath(desktop);
        if (!s) return KMimeType::Ptr(); // Hey, where did that one go?
        mimeTypeList = s->serviceTypes();
    }

    for (QStringList::const_iterator it = mimeTypeList.begin();
         it != mimeTypeList.end(); ++it) {
        if ((m_mimetype->name() != *it) && m_mimetype->is(*it)) {
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
        if (!pService) {
            kWarning() << "service with desktop path" << *it << "not found";
            continue; // Where did that one go?
        }

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

void MimeTypeData::setPatterns(const QStringList &p)
{
    m_patterns = p;
    // Sort them, since update-mime-database doesn't respect order (order of globs file != order of xml),
    // and this code says things like if (m_mimetype->patterns() == m_patterns).
    // We could also sort in KMimeType::setPatterns but this would just slow down the
    // normal use case (anything else than this KCM) for no good reason.
    m_patterns.sort();
}

bool MimeTypeData::areServicesDirty() const
{
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
    return false;
}
