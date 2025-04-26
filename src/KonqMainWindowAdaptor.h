/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 2000, 2006 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef __KonqMainWindowAdaptor_h__
#define __KonqMainWindowAdaptor_h__

// !!! Don't regenerate this file, I don't want to lose the method documentation
// Use qdbuscpp2xml KonqMainWindowAdaptor.h > org.kde.Konqueror.MainWindow.xml
// if you change the API.

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QDBusConnection>

class KonqMainWindow;

/**
 * DBUS interface for a konqueror main window
 */
class KonqMainWindowAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Konqueror.MainWindow")

public:

    /**
     * @brief Constructor
     *
     * @param mainWindow the main window associated with the new object
     */
    explicit KonqMainWindowAdaptor(KonqMainWindow *mainWindow);
    ~KonqMainWindowAdaptor() override; //!< Destructor

public slots:

    /**
     * @brief Opens a URL in the currente tab of this window
     *
     * @param url the URL to open
     * @param tempFile whether to delete the file after use. Usually this should be `false`
     */
    void openUrl(const QString &url, bool tempFile);

    /**
     * @brief Opens a URL in a new tab in this window
     *
     * @param url the URL to open
     * @param tempFile whether to delete the file after use. Usually this should be `false`
     */
    void newTab(const QString &url, bool tempFile);

    /**
     * @brief Opens and URL in a new tab specifying a startup id
     *
     * @param url the URL to open
     * @param startup_id the startup id
     * @param tempFile whether to delete the file after the view which displays it is closed.
     * This should usually be `false`
     */
    void newTabASN(const QString &url, const QByteArray &startup_id, bool tempFile);

    /**
     * @brief Opens and URL with the givne mimetype in a new tab specifying a startup id
     * @param url the URL to open
     * @param mimetype the mimetype of the URL
     * @param startup_id the startup id
     * @param tempFile whether to delete the file after the view which displays it is closed.
     * This should usually be `false`
     */
    void newTabASNWithMimeType(const QString &url, const QString &mimetype, const QByteArray &startup_id, bool tempFile);

    /**
     * @brief Splits the current view horizontally
     */
    void splitViewHorizontally();

    /**
     * @brief Splits the current view vertically
     */
    void splitViewVertically();

    /**
     * @brief Reloads the current view.
     */
    void reload();

    /**
     * @brief The current view
     *
     * @return the DBus path representing the current KonqView
     */
    QDBusObjectPath currentView();

    /**
     * @brief The current part
     *
     * @return the DBus path representing the current part
     */
    QDBusObjectPath currentPart();

    /**
     * @brief The view corresponding to the given index
     *
     * @param viewNumber the index of the view
     * @return the DBus path of the view corresponding to index @p viewNumber
     */
    QDBusObjectPath view(int viewNumber);

    /**
     * @brief The part corresponding to the given index
     *
     * @param partNumber the index of the part
     * @return the DBus path of the part corresponding to index @p partNumber
     */
    QDBusObjectPath part(int partNumber);

private:

    KonqMainWindow *m_pMainWindow; //!< The main window
};

#endif

