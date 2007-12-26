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

#ifndef MIMETYPEDATA_H
#define MIMETYPEDATA_H

#include <kmimetype.h>

/**
 * This is a non-gui (data) class, that represents a mimetype.
 * It is a KMimeType::Ptr plus the changes we made to it.
 */
class MimeTypeData
{
public:
    // Constructor used for groups
    MimeTypeData(const QString& major);
    // Real constructor, used for a mimetype.
    MimeTypeData(const KMimeType::Ptr mime, bool newItem = false);

    QString name() const { return m_major + '/' + m_minor; }
    QString majorType() const { return m_major; }
    QString minorType() const { return m_minor; }
    void setMinor(const QString& m) { m_minor = m; }
    QString comment() const { return m_comment; }
    void setComment(const QString& c) { m_comment = c; }

    /**
     * Returns true if "this" is a group
     */
    bool isMeta() const { return m_isGroup; }

    /**
     * Returns true if the type is essential, i.e. can't be deleted
     * (see KMimeType::checkEssentialMimeTypes)
     */
    bool isEssential() const;
    QString icon() const { return m_icon; }
    //void setIcon(const QString& icon);
    QStringList patterns() const { return m_patterns; }
    void setPatterns(const QStringList &p);
    QStringList appServices() const;
    void setAppServices(const QStringList &dsl) { m_appServices = dsl; }
    QStringList embedServices() const;
    void setEmbedServices(const QStringList &dsl) { m_embedServices = dsl; }

    enum AutoEmbed { Yes = 0, No = 1, UseGroupSetting = 2 };
    AutoEmbed autoEmbed() const { return m_autoEmbed; }
    void setAutoEmbed( AutoEmbed a ) { m_autoEmbed = a; }

    const KMimeType::Ptr& mimeType() const { return m_mimetype; }
    bool canUseGroupSetting() const;

    void getAskSave(bool &);
    void setAskSave(bool);

    // Whether the given service lists this mimetype explicitly
    KMimeType::Ptr findImplicitAssociation(const QString &desktop);

    /**
     * Returns true if the mimetype data has any unsaved changes.
     */
    bool isDirty() const;
    /**
     * Save changes to disk.
     * Does not check isDirty(), so the common idiom is if (data.isDirty()) { needUpdate = data.sync(); }
     * Returns true if update-mime-database needs to be run afterwards
     */
    bool sync();
    /**
     * Update m_mimetype from ksycoca when Apply is pressed
     */
    void refresh();

    static void runUpdateMimeDatabase();

    static bool defaultEmbeddingSetting( const QString& major );
    static void reset();

private:
    bool isMimeTypeDirty() const; // whether the mimetype .desktop file needs saving
    void getServiceOffers(QStringList& appServices, QStringList& embedServices) const;
    void getMyServiceOffers() const;
    void syncServices();
    void saveServices( KConfig & profile, const QStringList& services, const QString & servicetype2 );

    KMimeType::Ptr m_mimetype; // 0 if this is data for a mimetype group (m_isGroup==true)
    unsigned int groupCount:16; // shared between saveServices and sync
    unsigned int m_askSave:3; // 0 yes, 1 no, 2 default       -- TODO enum
    AutoEmbed m_autoEmbed:3;
    bool m_bNewItem:1;
    mutable bool m_bFullInit:1; // lazy init of m_appServices and m_embedServices
    bool m_isGroup:1;
    QString m_major, m_minor, m_comment, m_icon;
    QStringList m_patterns;
    mutable QStringList m_appServices;
    mutable QStringList m_embedServices;
};

#endif /* MIMETYPEDATA_H */
