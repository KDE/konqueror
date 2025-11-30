// This file is part of the KDE project
// SPDX-FileCopyrightText: 2025 Stefano Crocco <stefano.crocco@alice.it>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef KONQSPEEDDIAL_H
#define KONQSPEEDDIAL_H

#include "interfaces/speeddial.h"

/**
 * @brief Implementations of the KonqInterfaces::SpeedDial
 */
class KonqSpeedDial : public KonqInterfaces::SpeedDial
{
    Q_OBJECT

public:

    /**
     * @brief Constructor
     *
     * @param parent the parent object
     */
    KonqSpeedDial(QObject *parent);

    ~KonqSpeedDial() override; //!< Destructor

    /**
     * @brief Override of KonqInterfaces::SpeedDial::entries()
     *
     * It reads the entries from the configuration file
     *
     * @return a list of all speed dial entries
     */
    Entries entries() const override;

    /**
     * @brief Override of KonqInterfaces::SpeedDial::localIconUrlForEntry()
     *
     * What this does exactly depends on the entry icon URL:
     * - if it's a local URL or an URL with no scheme and an absolute path, it returns that URL
     * adding the `file` scheme to it if needed
     * - if it's an URL without scheme and a relative path, it treats the path as an icon name
     * and uses `KIconLoader::iconPath()` to attempt to retrieve its path
     * - if it's an empty URL, it treats it as a favicon and uses `KIO::favIconForUrl()` to retrieve
     * the cached version of the favicon
     * - it it's a remote URL it looks in the speed dial cache for a chached version of the icon.
     *
     * @param entry the entry to retrieve the icon for
     * @param size the size of the icon, specified either as a `KIconLoader::Group` or `KIconLoader::StdSizes`.
     * This is only used if the icon should be loaded from a theme
     * @param bool whether to attempt downloading the icon if it's not a local file and it's not in the cache
     * @return the URL of the local icon for @p entry if it's a local file or it's a remote file which is in the cache,
     * and an empty URL otherwise
     */
    QUrl localIconUrlForEntry(const Entry & entry, int size, bool download) override;

    /**
     * @brief Override of KonqInterfaces::SpeedDial::downloadAllIcons()
     */
    void downloadAllIcons() override;

    /**
     * @brief Override of KonqInterfaces::SpeedDial::addEntry()
     *
     * @param entry the new entry
     * @param cause the object which cause the addition of a new entry
     */
    void addEntry(const Entry &entry, QObject *cause = nullptr) override;

    /**
     * @brief Override of KonqInterfaces::SpeedDial::setEntries()
     *
     * @param entries the speed dial entries
     * @param cause the object which caused the changes in the speed dial
     */
    void setEntries(const QList<Entry>& entries, QObject *cause = nullptr) override;

private:

    /**
     * @brief The kind of URL contained in an Entry `iconUrl` field
     */
    enum IconType {
        LocalUrl, //!< A local URL
        LocalFile, //!< An absolute path but without a scheme
        IconName, //!< A relative path without a scheme. It's interpreted as an icon name
        Favicon, //!< A favicon
        RemoteIcon //!< A remote icon with a fully specified URL
    };

    /**
     * @brief The icon type associated with an entry
     * @param entry the entry
     * @return the icon type associated with @p entry
     */
    static IconType iconType(const Entry &entry);

    /**
     * @brief Starts downloading the favicon for @p entry
     *
     * It emits the isIconReady() signal when the favicon has been downloaded
     * @warning This function doesn't check that @p entry actually has an icon of
     * type Favicon: it assumes that it's so and tries to download the favicon for
     * that entry's URL.
     *
     * @param entry the entry whose favicon should be downloaded
     */
    void downloadFavicon(const Entry &entry);

    /**
     * @brief Starts downloading the remote icon for for @p entry
     *
     * It emits the isIconReady() signal when the icon has been downloaded
     * @warning This function doesn't check that @p entry actually has a remote
     * icon: it assumes that it's so and tries to download the it.
     * @param entry the entry whose icon should be downloaded
     */
    void downloadRemoteIcon(const Entry &entry);

    /**
     * @brief Downloads the icon for the given entry
     *
     * This function simply calls downloadFavicon() or downloadRemoteIcon() depending on
     * the icon type for @p entry is Favicon or RemoteIcon. It does nothing for
     * all other icon types.
     * @param entry the entry to download the icon for
     */
    void downloadIcon(const Entry &entry);

    /**
     * @brief The absolute path to cache where the given remote icon should be downloaded to
     *
     * The path will be to a file in #m_cacheDir. The name is created from @p iconUrl
     * host and path components.
     *
     * @note This function doesn't access the filesystem and assumes that @p iconUrl
     * is a remote URL
     * @param iconUrl the URL of the remote icon
     * @return the path where @p iconUrl should be downloaded to
     */
    QString cacheFilePath(const QUrl &iconUrl) const;

    /**
     * @brief The URL for the local icon with the given name and size
     *
     * This uses `KIconLoader::iconPath() to determine the icon path.
     *
     * @param name the name of the icon
     * @param size the size of the icon. It should be a value in either `KIconLoader::Group`
     * or `KIconLoader::StdSizes`
     * @return an URL with the path of the icon file. If no icon with the given name and
     * size is found, the path of the "unknown" icon is returned
     */
    static QUrl fromTheme(const QString &name, int size);

    /**
     * @brief Wether the icon for the given entry is ready to be used
     *
     * @param entry the entry to check the icon status for
     * @return `true` if the icon for the entry is local (either specified by name
     * or by full path) or if it's a remote icon or favicon which has already been
     * downloaded and `false` if it's a remote icon or favicon which hasn't been
     * downloaded yet
     */
    bool isIconReady(const Entry &entry);

    QString m_cacheDir; //!< The directory to look for cached remote icons
};

#endif // KONQSPEEDDIAL_H
