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
 * DBus interface for a konqueror view
 */
class KonqViewAdaptor : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Konqueror.View")

public:

    explicit KonqViewAdaptor(KonqView *view);
    ~KonqViewAdaptor() override;

public slots:

    /**
     * Displays another URL, but without changing the view mode
     * (Make sure the part can display this URL)
     */
    void openUrl(const QString &url,
                 const QString &locationBarURL,
                 const QString &nameFilter);

    /**
     * Reload
     */
    void reload();

    /**
     * Change the type of view (i.e. loads a new konqueror view)
     * @param mimeType the mime type we want to show
     * @param serviceName allows to enforce a particular service to be chosen,
     *        @see KonqFactory.
     * @note This assumes mimeType is a real mimetype, not `"Browser/View"`
     */
    bool changeViewMode(const QString &mimeType,
                        const QString &serviceName);

    /**
     * Call this to prevent next openUrl() call from changing history lists
     * Used when the same URL is reloaded (for instance with another view mode)
     */
    void lockHistory();

    /**
     * Stop loading
     */
    void stop();

    /**
     * Retrieve view's URL
     */
    QString url();

    /**
     * Get view's location bar URL, i.e. the one that the view signals
     * It can be different from url(), for instance if we display a index.html
     */
    QString locationBarURL();

    /**
     * @return the servicetype this view is currently displaying
     */
    QString type();

    /**
     * @return the servicetypes this view is capable to display
     */
    QStringList serviceTypes();

    /**
     * @return the part embedded into this view
     */
    QDBusObjectPath part();

    /**
     * Enable/Disable the context popup menu for this view.
     */
    void enablePopupMenu(bool b);

    bool isPopupMenuEnabled() const;

    /*
     * Return length of history
     */
    uint historyLength()const;

    /*
     * Move forward in history "-1"
     */
    void goForward();
    /*
     * Move back in history "+1"
     */
    void goBack();

    bool canGoBack()const;
    bool canGoForward()const;

private:

    KonqView *m_pView;

};

#endif

