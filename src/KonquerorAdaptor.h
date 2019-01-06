/* This file is part of the KDE project
   Copyright 2000 Simon Hausmann <hausmann@kde.org>
   Copyright 2000-2006 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KONQUERORADAPTOR_H
#define KONQUERORADAPTOR_H

#include <QStringList>
#include <QDBusObjectPath>
#include <QDBusMessage>

#define KONQ_MAIN_PATH "/KonqMain"

/**
 * DBus interface of a konqueror process
 */
class KonquerorAdaptor : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Konqueror.Main")

public:

    KonquerorAdaptor();
    ~KonquerorAdaptor() override;

public slots:

    /**
     * Opens a new window for the given @p url (using createSimpleWindow, i.e. a single view)
     * @param url the url to open
     * @param startup_id sets the application startup notification (ASN) property on the window, if not empty.
     * @return the DBUS object path of the window
     */
    QDBusObjectPath openBrowserWindow(const QString &url, const QByteArray &startup_id);

    /**
     * Opens a new window for the given @p url (using createNewWindow)
     * @param url the url to open
     * @param mimetype pass the mimetype of the url, if known, to speed up the process.
     * @param startup_id sets the application startup notification (ASN) property on the window, if not empty.
     * @param tempFile whether to delete the file after use, usually this is false
     * @return the DBUS object path of the window
     */
    QDBusObjectPath createNewWindow(const QString &url, const QString &mimetype, const QByteArray &startup_id, bool tempFile);

    /**
     * Opens a new window like @ref createNewWindow, then selects the given @p filesToSelect
     * @param filesToSelect the files to select in the newly opened file-manager window
     * @param startup_id sets the application startup notification (ASN) property on the window, if not empty.
     * @return the DBUS object path of the window
     */
    QDBusObjectPath createNewWindowWithSelection(const QString &url, const QStringList &filesToSelect, const QByteArray &startup_id);

    /**
     * @return a list of references to all the windows
     */
    QList<QDBusObjectPath> getWindows();

    /**
     * @return a list of all URLs currently opened in this process
     * Convenience function to avoid iterating over windows by hand.
     */
    QStringList urls() const;

    /**
     * Find a window which can be used for a new tab. Called by kfmclient.
     */
    QDBusObjectPath windowForTab();

Q_SIGNALS:
    /**
     * Emitted by kcontrol when the global configuration changes
     */
    void reparseConfiguration();
    /**
     * Used internally by Konqueror to notify all instances when a URL should be added to the combobox.
     */
    void addToCombo(const QString &url, const QDBusMessage &msg);
    /**
     * Used internally by Konqueror to notify all instances when a URL should be removed from the combobox.
     */
    void removeFromCombo(const QString &url, const QDBusMessage &msg);
    /**
     * Used internally by Konqueror to notify all instances when the combobox should be cleared.
     */
    void comboCleared(const QDBusMessage &msg);
};

#endif
