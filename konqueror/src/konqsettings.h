/* This file is part of the KDE project
   Copyright (C) 1999-2007 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __konq_settings_h__
#define __konq_settings_h__

#include <QMap>
#include <QString>

class KConfigGroup;

/**
 * Konqueror settings coming from KControl modules.
 *
 * (konquerorrc, group "FMSettings")
 */
class KonqFMSettings
{
public:

    /**
     * The static instance of KonqFMSettings
     */
    static KonqFMSettings * settings();

    /**
     * Reparse the configuration to update the already-created instances
     *
     * Warning : you need to call KGlobal::config()->reparseConfiguration()
     * first (This is not done here so that the caller can avoid too much
     * reparsing if having several classes from the same config file)
     */
    static void reparseConfiguration();

    // Use settings (and mimetype definition files)
    // to find whether to embed a certain service type or not
    // Only makes sense in konqueror.
    bool shouldEmbed( const QString & serviceType ) const;

    QString homeUrl() const;

private:
    /**
     * @internal
     * Constructs a KonqFMSettings instance from a config file.
     */
    KonqFMSettings(const KConfigGroup &config);

    /** Destructor. Don't delete any instance by yourself. */
    virtual ~KonqFMSettings();

private:

    static KonqFMSettings * s_pSettings;

    QMap<QString, QString> m_embedMap;

    QString m_homeURL;

    /** Called by constructor and reparseConfiguration */
    void init(const KConfigGroup &config);

    // There is no default constructor. Use the provided ones.
    KonqFMSettings();
    // No copy constructor either. What for ?
    KonqFMSettings( const KonqFMSettings &);
};

#endif
