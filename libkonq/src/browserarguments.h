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
    //TODO KF6: when dropping compatibility with KF5, add a suggestedFileName entry
    //to be used for downloads and possibly an id for DownloaderJob
    BrowserArguments();
    BrowserArguments(const BrowserArguments &args);
    BrowserArguments &operator=(const BrowserArguments &args);

    virtual ~BrowserArguments();

    /**
     * This buffer can be used by the part to save and restore its contents.
     * See KHTMLPart for instance.
     */
    QStringList docState;

    /**
     * @p softReload is set when user just hits reload button. It's used
     * currently for two different frameset reload strategies. In case of
     * soft reload individual frames are reloaded instead of reloading whole
     * frameset.
     */
    bool softReload;

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

private:
    BrowserArgumentsPrivate *d;
};

#endif
