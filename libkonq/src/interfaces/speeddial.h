//  This file is part of the KDE project
//  SPDX-FileCopyrightText: 2025 Stefano Crocco <stefano.crocco@alice.it>
// 
//  SPDX-License-Identifier: LGPL-2.0-or-later

#ifndef KONQINTERFACES_SPEEDDIAL_H
#define KONQINTERFACES_SPEEDDIAL_H

#include "libkonq_export.h"

#include "konqsettings.h"

#include <QObject>
#include <QUrl>

namespace KonqInterfaces {

/**
 * Interface to interact with the speed dial
 *
 * This interface allows to retrieve a list of all the speed dial entries stored
 * in the configuration file, to add a new speed dial entry and to set a new list
 * of speed dial entries.
 *
 * Entry icons must be handled carefully, as they often are remote files. When a
 * new speed dial entry is added (or an existing one is modified) the corresponding
 * icons may need to be downloaded. This isn't done automatically: you can start
 * downloading the icons for all entries using downloadAllIcons() or use localIconUrlForEntry()
 * for the icon of a single entry. Both functions download the icon asynchronously.
 *
 * @note Usually icons will be downloaded by the configuration dialog which the
 * user uses to create a new speed dial entry.
 *
 * @note Currently, there aren't methods to remove entries or to change them. To
 * do so, use entries() to retrieve the current entries, change the list and then
 * pass the updated list to setEntries().
 */
class LIBKONQ_EXPORT  SpeedDial : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Constructor
     *
     * @param parent the parent object
     */
    SpeedDial(QObject* parent);

    ~SpeedDial() override; //!< Destructor

    using Entry = Konq::Settings::SpeedDialEntry; //!< An entry in the speed dial
    using Entries = QList<Konq::SettingsBase::SpeedDialEntry>; //!< A list of entries in the speed dial

    /**
     * @brief A list of all speed dial entries
     * @return a list of all speed dial entries
     */
    virtual Entries entries() const = 0;

    /**
     * @brief The local URL of the icon to use as icon for the given entry
     *
     * If the icon for @p entry is not a local file and it's not in the cache, if @p download
     * is `true`, this function attempts to download the icon file and emits the iconReady()
     * signal when the download has finished.
     *
     * @param entry the entry to retrieve the icon for
     * @param size the size of the icon, specified either as a `KIconLoader::Group` or `KIconLoader::StdSizes`.
     * This is only used if the icon should be loaded from a theme
     * @param bool whether to attempt downloading the icon if it's not a local file and it's not in the cache
     * @return the URL of the local icon for @p entry if it's a local file or it's a remote file which is in the cache,
     * and an empty URL otherwise
     */
    virtual QUrl localIconUrlForEntry(const Entry &entry, int size, bool download = true) = 0;

    /**
     * @brief Starts downloading all remote icons and favicons which haven't been downloaded yet
     */
    virtual void downloadAllIcons() = 0;

    /**
     * @brief Adds a new speed dial entry
     *
     * Implementations of this method should save the new entry in the configuration
     * files and emit the speedDialChanged() signal, passing @p cause as argument.
     *
     * @param entry the new entry
     * @param cause the object which cause the addition of a new entry. It should
     * be passed as argument to speedDialChanged(). A value of `nullptr` means
     * that no specific object is responsible for the change
     */
    virtual void addEntry(const Entry &entry, QObject *cause= nullptr) = 0;

    /**
     * @brief Sets the entries of the speed dial
     *
     * This stores @p entries in the configuration file replacing any existing entries
     * and emits the speedDialChanged() signal, passing @p cause as argument.
     *
     * @param entries the speed dial entries
     * @param cause the object which caused the changes in the speed dial. It should
     * be passed as argument to speedDialChanged(). A value of `nullptr` means
     * that no specific object is responsible for the change
     */
    virtual void setEntries(const QList<Entry> &entries, QObject *cause = nullptr) = 0;

    /**
     * @brief Returns an instance of the SpeedDial which is child of the given object
     *
     * This assumes that there's only a single instance of a SpeedDial-derived class
     * among the children of the given object.
     *
     * @param parent the object among whose children to look for the instance of SpeedDial
     * @return @p parent or one of its children as a SpeedDial or `nullptr` if neither @p parent
     * nor its children is derived from SpeedDial. Children are searched recursively.
     */
    static SpeedDial* speedDial(QObject *parent = qApp);

    /**
     * @brief The URL of the speed dial page
     * @return the URL of the speed dial page
     */
    static QUrl url() {return QUrl(QStringLiteral("konq:speeddial"));}

Q_SIGNALS:
    /**
     * @brief Signal emitted when the icon for an entry has been downloaded
     *
     * @param entry the entry for which the icon has been downloaded
     * @param pixmap the downloaded pixmap
     */
    void iconReady(const Entry &entry, const QUrl &pixmap);

    /**
     * @brief Signal emitted when the speed dial configuration has changed
     */
    void speedDialChanged(QObject *cause = nullptr);
};

}

Q_DECLARE_INTERFACE(KonqInterfaces::SpeedDial, "SpeedDial")

#endif // KONQINTERFACES_SPEEDDIAL_H
