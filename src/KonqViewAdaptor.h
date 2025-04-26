/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 2000, 2006 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef __KonqViewAdaptor_h__
#define __KonqViewAdaptor_h__

#include <QStringList>
#include <QDBusObjectPath>

class KonqView;

/**
 * @brief DBus interface for a konqueror view
 */
class KonqViewAdaptor : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Konqueror.View")

public:

    /**
     * @brief Constructor
     *
     * @param view the view represented by the new object
     */
    explicit KonqViewAdaptor(KonqView *view);
    ~KonqViewAdaptor() override; //!< Destructor

public slots:

    /**
     * @brief Displays another URL, but without changing the view mode
     *
     * @note Callers should make sure the current part can display the new URL
     * @param url the URL to display
     * @param locationBarURL the URL to set in the location bar
     * @param nameFilter a filter to apply to the view's contents (e.g. *.cpp). It makes
     * sense only for some parts (e.g. DolphinPart)
     *
     * @see KonqView::openUrl()
     */
    void openUrl(const QString &url, const QString &locationBarURL, const QString &nameFilter);

    /**
     * @brief Reloads the view
     */
    void reload();

    /**
     * @brief Changes the part associated with the view so that it can display a
     * given mimetype
     * @param mimeType the mime type we want to show
     * @param serviceName the plugin id of the part to use. It must be a part able
     * to show @p mimeType. If empty, the part to use is chosen according to user
     * preferences
     */
    bool changeViewMode(const QString &mimeType, const QString &serviceName);

    /**
     * @brief Prevents the next call to openUrl() to change history
     *
     * This should be used when reloading the same URL for any reason (for example,
     * when changing view mode)
     */
    void lockHistory();

    /**
     * @brief Stops loading the current URL
     */
    void stop();

    /**
     * @brief The URL currently shown in the view
     *
     * @return the URL currently shown in the view
     */
    QString url();

    /**
     * @brief The URL in the view as it should be reported to the user
     *
     * @return the URL in the view as it should be reported to the user
     * @see KonqView::locationBarURL()
     */
    QString locationBarURL();

    /**
     * @brief The type of the view
     *
     * @return The type of the view. This can be either the mimetype of the URL
     * displayed in the view or `Browser/View`
     *
     * @see KonqView::type()
     */
    QString type();

    /**
     * @brief The part capabilities of the part shown in the view
     * @return a list with the string representation of the part capabilities of the part shown in the view
     */
    QStringList serviceTypes();

    /**
     * @brief The DBus path of the part associated with the view
     * @return the DBus path of the part embedded into the view
     */
    QDBusObjectPath part();

    /**
     * @brief Enables or disables the context popup menu for the view
     * @param b `true` if the popup menu should be enabled and `false` if it should be disabled
     */
    void enablePopupMenu(bool b);

    /**
     * @brief Whether the popup menu for the view is enabled or not
     * @return b `true` if the popup menu is enabled and `false` if it is disabled
     */
    bool isPopupMenuEnabled() const;

    /**
     * @brief The number of entries in the history of the view
     * @return the number of entries in the view history
     */
    uint historyLength()const;

    /**
     * @brief Moves one step forward in history
     */
    void goForward();

    /**
     * @brief Moves one step backwards in history
     */
    void goBack();

    /**
     * @brief Whether we can go back in history
     *
     * @return `false` if the view is showing the first element in history and `false` if
     * it's showing a later element
     */
    bool canGoBack() const;

    /**
     * @brief Whether we can go forward in history
     *
     * @return `false` if the view is showing the last element in history and `true` if
     * it's showing an earlier element
     */
    bool canGoForward() const;

private:

    KonqView *m_pView; //!< The view this object represents

};

#endif

