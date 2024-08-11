/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 1999 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 1999 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef BROWSERARGUMENTS_H
#define BROWSERARGUMENTS_H

#include <libkonq_export.h>

#include <QByteArray>
#include <QStringList>

class QDebug;

struct BrowserArgumentsPrivate;

/**
 * @struct BrowserArguments browserarguments.h
 *
 * @short BrowserArguments is a set of web-browsing-specific arguments,
 * which allow specifying how a URL should be opened by openUrl()
 * (as a complement to KParts::OpenUrlArguments which are the non-web-specific arguments)
 *
 * The arguments remain stored in the browser extension after that,
 * and can be used for instance to jump to the xOffset/yOffset position
 * once the url has finished loading.
 *
 * The parts (with a browser extension) who care about urlargs will
 * use those arguments, others will ignore them.
 *
 * This can also be used the other way round, when a part asks
 * for a URL to be opened (with openUrlRequest or createNewWindow).
 */
struct LIBKONQ_EXPORT BrowserArguments {
    //TODO add fields and methods for information currently reported using OpenUrlArguments::metaData

    BrowserArguments();
    BrowserArguments(const BrowserArguments &args);
    BrowserArguments &operator=(const BrowserArguments &args);

    virtual ~BrowserArguments();

    // ----- POST-related -----
    //TODO: These methods and fields are related to POST requests. Currently,
    //POST requests are handled by QtWebEngine without interaction with the rest
    //of Konqueror. See whether it is possible to improve this and, if not,
    //remove everything here
    /**
     * KHTML-specific field, contents of the HTTP POST data.
     */
    QByteArray postData;

    /**
     * KHTML-specific field, header defining the type of the POST data.
     */
    void setContentType(const QString &contentType);
    /**
     * KHTML-specific field, header defining the type of the POST data.
     */
    QString contentType() const;
    /**
     * KHTML-specific field, whether to do a POST instead of a GET,
     * for the next openURL.
     */
    void setDoPost(bool enable);

    /**
     * KHTML-specific field, whether to do a POST instead of a GET,
     * for the next openURL.
     */
    bool doPost() const;

    // ----- END POST related -----

    /**
     * Whether to lock the history when opening the next URL.
     * This is used during e.g. a redirection, to avoid a new entry
     * in the history.
     */
    void setLockHistory(bool lock);
    bool lockHistory() const;

    /**
     * Whether the URL should be opened in a new tab instead in a new window.
     */
    void setNewTab(bool newTab);
    bool newTab() const;

    //TODO: still needed, given that WebEnginePart doesn't have access to frame names anymore?
    /**
     * The frame in which to open the URL. KHTML/Konqueror-specific.
     */
    QString frameName;

    /**
     * If true, the part who asks for a URL to be opened can be 'trusted'
     * to execute applications. For instance, the directory views can be
     * 'trusted' whereas HTML pages are not trusted in that respect.
     */
    bool trustedSource;

    //TODO see whether it is possible to integrate these with WebEnginePart, otherwise remvoe them
    /**
     * @return true if the request was a result of a META refresh/redirect request or
     * HTTP redirect.
     */
    bool redirectedRequest() const;

    /**
     * Set the redirect flag to indicate URL is a result of either a META redirect
     * or HTTP redirect.
     *
     * @param redirected
     */
    void setRedirectedRequest(bool redirected);

    /**
     * Set whether the URL specifies to be opened in a new window.
     *
     * When openUrlRequest is emitted:
     * <ul>
     *  <li>normally the url would be opened in the current view.</li>
     *  <li>setForcesNewWindow(true) specifies that a new window or tab should be used:
     *  setNewTab(true) requests a tab specifically, otherwise the user-preference is followed.
     *  This is typically used for target="_blank" in web browsers.</li>
     * </ul>
     *
     * When createNewWindow is emitted:
     * <ul>
     *  <li>if setNewTab(true) was called, a tab is created.</li>
     *  <li>otherwise, if setForcesNewWindow(true) was called, a window is created.</li>
     *  <li>otherwise the user preference is followed.</li>
     * </ul>
     */
    void setForcesNewWindow(bool forcesNewWindow);

    /**
     * Whether the URL specifies to be opened in a new window
     */
    bool forcesNewWindow() const;

    /**
     * @brief The suggested name to use when downloading the URL
     * @return The suggested name
     */
    QString suggestedDownloadName() const;

    /**
     * @brief Changes the suggested name to use when downloading the URL
     * @param name the suggested name
     */
    void setSuggestedDownloadName(const QString &name);

    using MaybeInt = std::optional<int>;

    /**
     * @brief The download id, if any
     * @return The identifier for a download to be performed by the requesting part or `std::nullopt`
     * if the request doesn't represent a download or the download should be performed by the
     * part which will display the URL
     */
    MaybeInt downloadId() const;
    /**
     * @brief Sets the download id
     *
     * By default, the download id is `std::nullopt`.
     * @param id the new download id. It can be an `int` or `std::nullopt`
     */
    void setDownloadId(MaybeInt id);

    /**
     * @brief The part to use when embedding the requested URL
     * @return The plugin id of the the part to use when embedding the requested URL or
     * an empty string if no specific part has been requested
     */
    QString embedWith() const;
    /**
     * @brief Tells Konqueror to use a specific part when embedding the requested URL
     *
     * By default, the part to use will be determined according to the mimetype of the URL
     * and to the user's preferences
     * @param partId the plugin id of the part to use. If empty, the default behavior will
     * be restored
     */
    void setEmbedWith(const QString &partId);

    /**
     * @brief The application to use when opening the requested URL externally
     * @return The application to use when opening the requested URL externally or
     * an empty string if no specific application has been requested
     */
    QString openWith() const;
    /**
     * @brief Tells Konqueror to use a specific application when opening the requested URL externally
     *
     * By default, the application to use will be determined according to the mimetype of the URL
     * and to the user's preferences
     * @param app the application to use. If empty, the default behavior will be restored
     */
    void setOpenWith(const QString &app);

    //TODO: is it possible to provide more information about the reasons for ignoring the default part,
    //so that Konqueror can better choose a different approach
    /**
     * @brief Tells Konqueror to ignore the default part for html files when deciding what part to use when
     * embedding the URL
     *
     * There are situation in which Konqueror may think that an URL should be displayed in the default part
     * for HTML files but it is already known that this isn't actually true. This informs Konqueror of this
     * situation, so that it can choose another part. This is, for example, the case when downloading a file
     * supported by the html part but with a response containing the `Content-Disposition: attachment` header.
     *
     * @return `true` if Konqueror shouldn't use the default part for html files to embed the URL or `false` if
     * it can use such part
     */
    bool ignoreDefaultHtmlPart() const;
    /**
     * @brief Sets whether or not the default part for html files should be ignored when deciding what part to use
     * to embed the URL
     *
     * The default behavior is _not_ to ignore the default part for html files.
     * @warning It's important to always call this when the default part for html files should be ignored: not doing
     * so could lead to endless loops
     * @param ignore whether the default part for html files should be ignored or not
     */
    void setIgnoreDefaultHtmlPart(bool ignore);

    /**
     * @brief Enum describing actions which can be performed with an URL
     */
    enum class Action {
        UnknownAction, //!< A default value to use, for example, when no action has been determined
        DoNothing, //!< Take no action
        Save, //!< Save the URL to disk
        Embed, //!< Embed the URL in Konqueror
        Open, //!< Open the URL in an external application
        Execute //!< Execute the URL
    };

    /**
     * @brief Which action should be performed on the URL
     * @return The action to be performed on the URL. A value of Action::Unknown
     * means that Konqueror is free to decide what to do
     */
    Action forcedAction() const;
    /**
     * @brief Sets the action to perform on the URL
     *
     * If this is not called, Konqueror forcedAction() will return Action::Unknown
     * @param act the action to perform on the URL. Pass Action::Unknown if
     * Konqueror should be free to decide what to do
     */
    void setForcedAction(Action act);

private:
    /**
     * @brief Helper function which returns #d, creating it if it doesn't exist
     *
     * @return #d
     */
    BrowserArgumentsPrivate* ensureD();
    BrowserArgumentsPrivate *d;
};

QDebug LIBKONQ_EXPORT operator<<(QDebug dbg, BrowserArguments::Action action);

#endif
