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
 * @brief Class which provides access to mimetype embedding settings
 *
 * These settings are managed by the filetypes KCM and are stored in the `filetypesrc`
 * configuration file under the `EmbedSettings` group
 *
 * No new instances of this class can be created. The only existing instance can
 * be accessed using the settings() static function.
 */
class LIBKONQ_EXPORT KonqFMSettings
{
public:

    /**
     * @brief The only instance of this class
     *
     * @return the singleton instance of this class
     */
    static KonqFMSettings *settings();

    /**
     * @brief Reads again the settings from the configuration file
     *
     * @warning Before calling this function, you need to call
     * @code
     * KSharedConfig::openConfig()->reparseConfiguration()
     * @endcode
     * This is not done here so that the caller can avoid too much
     * reparsing if having several classes from the same config file
     */
    static void reparseConfiguration();

    /**
     * @brief The configuration object containing the embedding settings
     *
     * @return the configuration object containing the embedding settings
     */
    KSharedConfig::Ptr fileTypesConfig();

    // Use settings (and mimetype definition files)
    // to find whether to embed a certain service type or not
    // Only makes sense in konqueror.
    /**
     * @brief Whether the given mimetype should be embedded or not
     *
     * It first checks whether there's an explicit setting for that mimetype. If
     * there's no such setting, it looks for settings for the whole group the mimetype
     * belongs to. If still no setting is found, it returns a default value depending
     * on the mimetype.
     *
     * @param serviceType the service type or mimetype
     * @return `true` if the given service type or mime type should be embedded and
     * `false` otherwise
     */
    bool shouldEmbed(const QString &serviceType) const;

private:
    /**
     * @brief Destructor
     *
     * This is private because you shouldn't delete any instance by yourself
     */
    virtual ~KonqFMSettings();

    /**
     * @brief Reads the data from the configuration object
     *
     * This function is called both by the constructor and by reparseConfiguration
     * and fills #m_embedMap.
     *
     * @param reparse whether or not to read again the configuration file before
     * reading the data from the configuration object
     */
    void init(bool reparse);

    /**
     * @brief The default constructor
     *
     * It's private because you should never create instances of this class but only
     * use the instance returned by settings()
     */
    KonqFMSettings();

    /**
     * @brief The copy constructor
     *
     * It's private because it should never be used
     */
    KonqFMSettings(const KonqFMSettings &);


private:

    /**
     * @brief The embedding settings read from the configuration file
     *
     * This is a map where the keys have the form `embed-`_mimetype and the value can be
     * `true` if the mimetype should be embedded or `false` if it shouldn't
     */
    QMap<QString, QString> m_embedMap;

    KSharedConfig::Ptr m_fileTypesConfig; //!< The configuration object containing the embedding settings

    friend class KonqEmbedSettingsSingleton;
};

#endif //KONQEMBEDSETTINGS
