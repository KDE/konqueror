/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 2000-2006 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQUERORADAPTOR_H
#define KONQUERORADAPTOR_H

#include <QStringList>
#include <QDBusObjectPath>
#include <QDBusMessage>

#define KONQ_MAIN_PATH "/KonqMain"

/**
 * @brief DBus interface of a Konqueror process
 */
class KonquerorAdaptor : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Konqueror.Main")

public:

    /**
     * @brief Constructor
     */
    KonquerorAdaptor();

    /**
     * @brief Destructor
     */
    ~KonquerorAdaptor() override;

public slots:

    /**
     * @brief Opens a new window for the given URL
     *
     * @param url the url to open. This isn't passed through KUriFilter, so it must be an exact URL
     * (for example, passing `www.kde.org` won't work because it won't be transformed in `https://kde.org`).
     * @param startup_id sets the application startup notification (ASN) property on the window, if not empty.
     * @return the DBus object path of the window
     */
    QDBusObjectPath openBrowserWindow(const QString &url, const QByteArray &startup_id);

    /**
     * @brief Opens a new window for the given URL
     *
     * Unlike openBrowserWindow(), @p url is passed through KonqMisc::konqFilteredURL(), which may change it.
     * This means that it's allowed to pass things like `www.kde.org` instead of `https://kde.org`.
     *
     * @param url the url to open
     * @param mimetype pass the mimetype of the URL, if known, to speed up the process.
     * @param startup_id sets the application startup notification (ASN) property on the window, if not empty.
     * @param tempFile whether to delete the file after use, usually this is false
     * @return the DBus object path of the window
     */
    QDBusObjectPath createNewWindow(const QString &url, const QString &mimetype, const QByteArray &startup_id, bool tempFile);

    /**
     * @brief Opens a new window like @ref openBrowserWindow, then selects the given files
     * @param url the URL to open. This isn't passed through KUriFilter, so it must be an exact URL
     * @param filesToSelect the files to select in the newly opened file-manager window
     * @param startup_id sets the application startup notification (ASN) property on the window, if not empty.
     * @return the DBus object path of the window
     */
    QDBusObjectPath createNewWindowWithSelection(const QString &url, const QStringList &filesToSelect, const QByteArray &startup_id);

    /**
     * @brief A list of references to all the windows
     * @return a list of references to all the windows
     */
    QList<QDBusObjectPath> getWindows();

    /**
     * @brief A list of all URLs currently opened in this process
     *
     * This is a convenience function to avoid iterating over windows by hand.
     * @return a list of all URLs currently opened in this process
     */
    QStringList urls() const;

    /**
     * @brief Finds a window where a new tab can be created
     *
     * See KonqMainWindow::findMostSuitableWindow() for details about how the window
     * is chosen.
     *
     * This is called by `kfmclient`.
     *
     * @return The DBus path of the window or `/` if no suitable window could be found
     */
    QDBusObjectPath windowForTab();

Q_SIGNALS:
    /**
     * @brief Emitted by kcontrol when the global configuration changes
     */
    void reparseConfiguration();
    /**
     * @brief Used internally by Konqueror to notify all instances when a URL should be added to the combobox.
     */
    void addToCombo(const QString &url, const QDBusMessage &msg);
    /**
     * @brief Used internally by Konqueror to notify all instances when a URL should be removed from the combobox.
     */
    void removeFromCombo(const QString &url, const QDBusMessage &msg);
    /**
     * @brief Used internally by Konqueror to notify all instances when the combobox should be cleared.
     */
    void comboCleared(const QDBusMessage &msg);
};

#endif
