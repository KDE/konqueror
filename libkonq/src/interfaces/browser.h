/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KONQINTERFACES_BROWSER_H
#define KONQINTERFACES_BROWSER_H

#include "libkonq_export.h"

#include <QObject>

namespace KParts {
    class ReadOnlyPart;
    class OpenUrlArguments;
};

class BrowserArguments;

namespace KonqInterfaces {

class CookieJar;
class Window;
class SpeedDial;

/**
 * @brief Abstract class representing the Konqueror browser
 */
class LIBKONQ_EXPORT Browser : public QObject
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     *
     * @param parent the parent object
     */
    Browser(QObject* parent = nullptr);
    virtual ~Browser(); ///< Destructor

    /**
     * @brief Sets the object to use for cookies management
     *
     * @note this object *wont't* take ownership of @p jar.
     *
     * @param jar the object to use for cookie management
     */
    virtual void setCookieJar(CookieJar *jar) = 0;

    /**
     * @brief The object to use for cookie management
     * @return the object to use for cookie management
     */
    virtual CookieJar* cookieJar() const = 0;

    /**
     * @brief The standard user agent used by Konqueror
     * @return The standard user agent used by Konqueror
     */
    virtual QString konqUserAgent() const = 0;

    /**
     * @brief The default user agent
     * @return The default user agent
     */
    virtual QString defaultUserAgent() const = 0;

    /**
     * @brief The user agent currently in use
     *
     * This can be either defaultUserAgent() or a temporary user agent set with setTemporaryUserAgent()
     * @return The user agent currently in use
     * @see setTemporaryUserAgent()
     */
    virtual QString userAgent() const = 0;

    /**
     * @brief Sets a temporary user agent to be used instead of the default one
     *
     * The temporary user agent remains in use until the application is closed or this is called again.
     *
     * Implementations should emit the userAgentChanged() signal, unless the new user
     * agent is effectively equal to to old. If switching from the default user agent to a
     * temporary user agent but the two are equal (or vice versa) the signal shouldn't be emitted
     * @param newUA the new user agent string.
     */
    virtual void setTemporaryUserAgent(const QString &newUA) = 0;

    /**
     * @brief Stops using a temporary user agent and returns to the default one
     */
    virtual void clearTemporaryUserAgent() = 0;

    /**
     * @brief Casts the given object or one of its children to a Browser
     *
     * This is similar to
     * @code
     * obj->findChild<Browser*>();
     * @endcode
     * except that if @p obj derives from Browser, it will be returned, regardless of whether any
     * of its children also derive from it.
     * @param obj the object to cast to a Browser
     * @return @p obj or one of its children as a Browser* or `nullptr` if neither @p obj nor its children derive from Browser
     */
    static Browser* browser(QObject* obj);

    /**
     * @brief Whether a part has permission to navigate to the given URL
     * @param part the part
     * @param url the URL the part wants to navigate to
     * @return `true` if @p part can navigate to @p url and `false` otherwise.
     */
    virtual bool canNavigateTo(KParts::ReadOnlyPart *part, const QUrl &url) const = 0;

    /**
     * @brief Asks Konqueror to open the given URL
     *
     * This is similar to using the BrowserExtension::browserOpenUrlRequest() except that:
     * - it doesn't specify the part requesting to open the URL
     * - it doesn't use a signal
     * - it synchronous
     *
     * This function should mostly be used when wanting to open an URL without having a part to use for emitting
     * the BrowserExtension::browserOpenUrlRequest() signal.
     * @param url the URL to open
     * @param args arguments describing how to open the URL
     * @param bargs Konqueror-specific arguments describing how to open the URL
     * @param window the window to open the URL in. Even if this is declared as a `QWidget`, it is assumed to be either
     * `nullptr` or an instance of Konqueror main window. If this is `nullptr` or not an instance of Konqueror main window,
     * Konqueror itself will decide in which window to open the URL, creating a new window if needed
     * @return `true` if a suitable window was found or created and `false` otherwise (this should usually never happen)
     */
    virtual bool openUrl(const QUrl &url, KParts::OpenUrlArguments &args, const BrowserArguments &bargs, QWidget *window = nullptr) = 0;

    virtual Window* window(QWidget *widget) = 0;
    /**
     * @brief Sets the starting directory for the "Save As" dialogs used to download files from the given window
     * @param saveDir the new starting directory
     * @param window the window to change the directory for
     */
    virtual void setSaveDirForWindow(const QString &saveDir, QWidget *window) = 0;

    /**
     * @brief The part to use to open a given local file
     *
     * This function will try to determine the mimetype of the file using `QMimeDatabase`, unless a mimetype is explicitly given.
     * @param path the path of the file
     * @param mimetype if this is not empty, the mimetype of the file won't be determined automatically using `QMimeDatabase`.
     *  @p mimetype will be used as mimetype, instead
     */
    virtual QString partForLocalFile(const QString &path, const QString &mimeType = {}) = 0;

    /**
     * @brief The global speed dial object
     * @return the global speed dial object
     */
    virtual SpeedDial* speedDial() = 0;

signals:
    void configurationChanged(); ///< Signal emitted after the configuration has changed

    /**
     * @brief Signal emitted when the user agent changes
     *
     * This signal can be emitted in several situations:
     * - the default user agent is in use and the user changes it
     * - the user switched from the default user agent to a temporary one (or vice versa) and the two are different
     * - the user switched from a temporary user agent to a different temporary user agent
     *
     * @param currentUA the new current user agent
     * @see setTemporaryUserAgent()
     */
    void userAgentChanged(const QString &currentUA);
};

}

#endif // KONQINTERFACES_BROWSER_H
