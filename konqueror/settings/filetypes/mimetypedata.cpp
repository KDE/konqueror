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

MimeTypeData::MimeTypeData(const QString& major)
    : m_askSave(AskSaveDefault),
      m_bNewItem(false),
      m_bFullInit(true),
      m_isGroup(true),
      m_major(major)
{
    m_autoEmbed = readAutoEmbed();
}

MimeTypeData::MimeTypeData(const KMimeType::Ptr mime)
    : m_mimetype(mime),
      m_askSave(AskSaveDefault), // TODO: the code for initializing this is missing. FileTypeDetails initializes the checkbox instead...
      m_bNewItem(false),
      m_bFullInit(false),
      m_isGroup(false)
{
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

MimeTypeData::MimeTypeData(const QString& mimeName, bool)
    : m_mimetype(0),
      m_askSave(AskSaveDefault),
      m_bNewItem(true),
      m_bFullInit(false),
      m_isGroup(false)
{
    const int index = mimeName.indexOf('/');
    if (index != -1) {
        m_major = mimeName.left(index);
        m_minor = mimeName.mid(index+1);
    } else {
        m_major = mimeName;
    }
    m_autoEmbed = UseGroupSetting;
    // all the rest is empty by default
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
    // Keep in sync with KMimeType::checkEssentialMimeTypes
    const QString n = name();
    if ( n == "application/octet-stream" )
        return true;
    if ( n == "inode/directory" )
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
        KMimeTypeTrader::self()->query(name(), "Application");
    KService::List::const_iterator it(offerList.begin());
    for (; it != offerList.constEnd(); ++it)
        if ((*it)->allowAsDefault())
            appServices.append((*it)->entryPath());

    const KService::List partOfferList =
        KMimeTypeTrader::self()->query(name(), "KParts/ReadOnlyPart");
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
    if (m_bNewItem)
        return true;

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

    if (m_askSave != AskSaveDefault)
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

    if (m_askSave != AskSaveDefault) {
        const KSharedConfig::Ptr config = KSharedConfig::openConfig("filetypesrc", KConfig::NoGlobals);
        KConfigGroup cg = config->group("Notification Messages");
        if (m_askSave == AskSaveYes) {
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

        needUpdateMimeDb = true;
    }

    if (areServicesDirty()) {
        syncServices();
    }

    return needUpdateMimeDb;
}

void MimeTypeData::syncServices()
{
    KSharedConfig::Ptr profile = KSharedConfig::openConfig("mimeapps.list", KConfig::NoGlobals, "xdgdata-apps");

    // Save preferred services
    KConfigGroup addedApps(profile, "Added Associations");
    saveServices(addedApps, m_appServices);
    KConfigGroup addedParts(profile, "Added KDE Service Associations");
    saveServices(addedParts, m_embedServices);

    // Handle removed services
    KConfigGroup removedApps(profile, "Removed Associations");
    saveRemovedServices(removedApps, m_appServices, "Application");
    KConfigGroup removedParts(profile, "Removed KDE Service Associations");
    saveRemovedServices(removedParts, m_embedServices, "KParts/ReadOnlyPart");
}

static QStringList collectStorageIds(const QStringList& services)
{
    QStringList serviceList;
    QStringList::const_iterator it(services.begin());
    for (int i = services.count(); it != services.end(); ++it, i--) {

        KService::Ptr pService = KService::serviceByDesktopPath(*it);
        if (!pService) {
            kWarning() << "service with desktop path" << *it << "not found";
            continue; // Where did that one go?
        }

        serviceList.append(pService->storageId());
    }
    return serviceList;
}

void MimeTypeData::saveRemovedServices(KConfigGroup & config, const QStringList& services, const QString& genericServiceType)
{
    QStringList removedServiceList;
    const KService::List offerList =
        KMimeTypeTrader::self()->query(name(), genericServiceType);

    KService::List::const_iterator it_srv(offerList.begin());
    for (; it_srv != offerList.end(); ++it_srv) {
        KService::Ptr pService = (*it_srv);
        if (!services.contains(pService->entryPath())) {
            // The service was in m_appServices (or m_embedServices) but has been removed
            removedServiceList.append(pService->storageId());
        }
    }
    if (removedServiceList.isEmpty())
        config.deleteEntry(name());
    else
        config.writeXdgListEntry(name(), removedServiceList);
}

void MimeTypeData::saveServices(KConfigGroup & config, const QStringList& services)
{
    if (services.isEmpty())
        config.deleteEntry(name());
    else
        config.writeXdgListEntry(name(), collectStorageIds(services));
}

void MimeTypeData::refresh()
{
    kDebug() << "MimeTypeData refresh" << name();
    m_mimetype = KMimeType::mimeType( name() );
    if (m_mimetype && m_bNewItem) {
        kDebug() << "OK, created" << name();
        m_bNewItem = false; // if this was a new mimetype, we just created it
        m_icon = m_mimetype->iconName();
    }
}

void MimeTypeData::getAskSave(bool &_askSave)
{
    if (m_askSave == AskSaveYes)
        _askSave = true;
    if (m_askSave == AskSaveNo)
        _askSave = false;
}

void MimeTypeData::setAskSave(bool _askSave)
{
    m_askSave = _askSave ? AskSaveYes : AskSaveNo;
}

bool MimeTypeData::canUseGroupSetting() const
{
    // "Use group settings" isn't available for zip, tar etc.; those have a builtin default...
    if (!m_mimetype) // e.g. new mimetype
        return true;
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
