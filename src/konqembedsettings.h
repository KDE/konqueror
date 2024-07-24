/* This file is part of the KDE project
    SPDX-FileCopyrightText: 1999-2007 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KONQEMBEDSETTINGS
#define KONQEMBEDSETTINGS

#include <ksharedconfig.h>
#include <QMap>
#include <QString>
#include <kconfig.h>

#include <libkonq_export.h>

/**
 * Konqueror settings regarding embedding mimetypes coming from KCM modules.
 *
 * (konquerorrc, group "FMSettings")
 */
class LIBKONQ_EXPORT KonqFMSettings
{
public:

    /**
     * The static instance of KonqFMSettings
     */
    static KonqFMSettings *settings();

    /**
     * Reparse the configuration to update the already-created instances
     *
     * Warning : you need to call KSharedConfig::openConfig()->reparseConfiguration()
     * first (This is not done here so that the caller can avoid too much
     * reparsing if having several classes from the same config file)
     */
    static void reparseConfiguration();

    KSharedConfig::Ptr fileTypesConfig();

    // Use settings (and mimetype definition files)
    // to find whether to embed a certain service type or not
    // Only makes sense in konqueror.
    bool shouldEmbed(const QString &serviceType) const;

private:
    /** Destructor. Don't delete any instance by yourself. */
    virtual ~KonqFMSettings();

private:
    QMap<QString, QString> m_embedMap;

    KSharedConfig::Ptr m_fileTypesConfig;

    /** Called by constructor and reparseConfiguration */
    void init(bool reparse);

    // There is no default constructor. Use the provided ones.
    KonqFMSettings();
    // No copy constructor either. What for ?
    KonqFMSettings(const KonqFMSettings &);

    friend class KonqEmbedSettingsSingleton;
};

#endif //KONQEMBEDSETTINGS
