// /* This file is part of the KDE project
//     SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>
// 
//     SPDX-License-Identifier: LGPL-2.0-or-later
// */

#ifndef KONQBROWSER_H
#define KONQBROWSER_H

#include "interfaces/browser.h"

#include <QPointer>

namespace KonqInterfaces
{
    class CookieJar;
}

class KonquerorApplication;

/**
 * @brief Implementation of KonqInterfaces::Browser
 */
class KonqBrowser : public KonqInterfaces::Browser
{
    Q_OBJECT

public:
    /**
     * @brief Default constructor
     *
     * @param parent the parent object
     */
    KonqBrowser(QObject* parent = nullptr);

    ~KonqBrowser();  ///< Destructor

    KonqInterfaces::CookieJar* cookieJar() const override; ///< Implementation of KonqInterfaces::Browser::cookieJar()

    void setCookieJar(KonqInterfaces::CookieJar* jar) override; ///< Implementation of KonqInterfaces::Browser::setCookieJar()

    QString konqUserAgent() const override; ///< Implementation of Browser::konqUserAgent()
    QString defaultUserAgent() const override; ///< Implementation of Browser::defaultUserAgent()
    QString userAgent() const override; ///< Implementation of Browser::currentUserAgent()
    void setTemporaryUserAgent(const QString & newUA) override; ///< Implementation of Browser::setTemporaryUserAgent()
    void clearTemporaryUserAgent() override; ///< Implementation of Browser::clearTemporaryUserAgent()

    /**
     * @brief Reads the configuration files and applies the necessary changes
     *
     * This should be called whenever the configuration changes, usually from KonquerorApplication::reparseConfiguration()
     */
    void applyConfiguration();

    static QString konquerorUserAgent(); ///< The standard Konqueror user agent string

    /**
     * @brief Implementation of KonqInterfaces::Browser::canNavigateTo
     * @return the value returned by KonqView::canNavigateTo() for the view associated with the part
     * or if it isn't possible to find such a view (it should never happen)
     * @see KonqView::canNavigateTo()
     */
    bool canNavigateTo(KParts::ReadOnlyPart *part, const QUrl &url) const override;

    /**
     * @brief Implementation of KonqInterfaces::Browser::openUrl()
     * @see KonqInterfaces::Browser::openUrl()
     */
    bool openUrl(const QUrl & url, KParts::OpenUrlArguments & args, const BrowserArguments & bargs, QWidget * window = nullptr) override;

    KonqInterfaces::Window* window(QWidget* widget) override;

    /**
     * @brief Implementation of KonqInterfaces::Browser::setSaveDirForWindow()
     * @see KonqInterfaces::Browser::setSaveDirForWindow()
     */
    void setSaveDirForWindow(const QString &saveDir, QWidget *window) override;

    QString partForLocalFile(const QString & path, const QString & mimeType) override;

private:

    /**
     * @brief Struct used to store information about the user agent
     */
    struct UserAgentData {
        QString defaultUA; ///< The default user agent
        QString temporaryUA; ///< The temporary UA, if it has been set
        bool usingDefaultUA = true; ///< Whether the default user agent is currently in use
        /**
         * @brief The user agent currently in use
         *
         * Depending on the value of #usingDefaultUA, this will either be #defaulUA or #temporaryUA
         */
        QString currentUserAgent() const {return usingDefaultUA ? defaultUA : temporaryUA;}
    };

    /**
     * @brief Reads the default user agent from the configuration file
     *
     * This method also emits the userAgentChanged() signal, if needed
     */
    void readDefaultUserAgent();

    /**
     * @brief Helper function returning `qApp` cast to a KonquerorApplication
     */
    KonquerorApplication* app() const;

    UserAgentData m_userAgent; ///< The user agent data

    QPointer<KonqInterfaces::CookieJar> m_cookieJar; ///< Holds the cookie jar
};

#endif // KONQBROWSER_H
