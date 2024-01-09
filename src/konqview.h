/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 1998-2005 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef __konq_view_h__
#define __konq_view_h__

#include "konqmainwindow.h" // hmm, please move PageSecurity out of konq_mainwindow...
#include "konqfactory.h"
#include "konqframe.h"
#include "konqutils.h"
#include "kf5compat.h" //For NavigationExtension
#include "browserarguments.h"

#include <kservice.h>
#include <QMimeType>

#include <QList>

#include <QObject>
#include <QStringList>
#include <QPointer>
#include <QEvent>

#include <config-konqueror.h>

class UrlLoader;
class KonqFrame;
namespace KParts
{
//TODO KF6: when removing compatibility with KF5, uncomment the line below
//class NavigationExtension;
class StatusBarExtension;
}

#if QT_VERSION_MAJOR < 6
#ifdef KActivities_FOUND
namespace KActivities
{
class ResourceInstance;
}
#endif
#endif

namespace Konq {
    class PlaceholderPart;
}

// TODO: make the history-handling code reuseable (e.g. in kparts) for people who want to use a
// khtml-based browser in some apps. Back/forward support is all in here currently.
struct HistoryEntry {
    void loadItem(const KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options);
    void saveConfig(KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options);
    static HistoryEntry* fromDelayedLoadingData(const KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options);

    QUrl url;
    QString locationBarURL; // can be different from url when showing a index.html
    QString title;
    QByteArray buffer;
    QString strServiceType;
    QString strServiceName;
    QByteArray postData;
    QString postContentType;
    bool doPost;
    QString pageReferrer;
    KonqMainWindow::PageSecurity pageSecurity;
    bool reload; // This is used when History entry is restored from a config file
};

/* This class represents a child of the main view. The main view maintains
 * the list of children. A KonqView contains a Browser::View and
 * handles it. It's more or less the backend structure for the views.
 * The widget handling stuff is done by the KonqFrame.
 */
class KONQ_TESTS_EXPORT KonqView : public QObject
{
    Q_OBJECT
public:

    /**
     * @brief Constructor which creates a view with a PlaceholderPart
     * @param viewFrame the frame which will contain the view
     * @param mainWindow the main window the view will belong to
     */
    KonqView(KonqFrame *viewFrame, KonqMainWindow* mainWindow);

    /**
     * Create a konqueror view with an appropriate part
     * @param viewFactory the factory to be used to create the part
     * @param viewFrame the frame where to create the view
     * @param mainWindow is the main window :-)
     * @param service the service implementing the part
     * @param partServiceOffers list of part offers found by the factory
     * @param appServiceOffers list of app offers found by the factory
     * @param serviceType the serviceType implemented by the part
     * @param passiveMode whether to initially make the view passive
     */
    KonqView(KonqViewFactory &viewFactory,
             KonqFrame *viewFrame,
             KonqMainWindow *mainWindow,
             const KPluginMetaData &service,
             const QVector<KPluginMetaData> &partServiceOffers,
             const KService::List &appServiceOffers,
             const QString &serviceType,
             bool passiveMode);

    ~KonqView() override;

    bool isWebEngineView() const;

    /**
     * Displays another URL, but without changing the view mode (caller has to
     * ensure that the call makes sense)
     * @param url the URL to open
     * @param locationBarURL the URL to set in the location bar (see @ref setLocationBarURL)
     * @param nameFilter e.g. *.cpp
     * @param tempFile whether to delete the file after use
     */
    void openUrl(const QUrl &url,
                 const QString &locationBarURL,
                 const QString &nameFilter = QString(),
                 bool tempFile = false,
                 const QUrl &requestedUrl={});

    void duplicateView(KonqView *otherView);

    /**
     * Change the part inside this view if necessary.
     * Contract: the caller should call stop() first.
     *
     * @param mimeType the mime type we want to show
     * @param serviceName allows to enforce a particular service to be chosen,
     *        @see KonqFactory.
     * @param forceAutoEmbed
     * @param dontDeleteTemporaryFile
     */
    bool changePart(const QString &mimeType,
                    const QString &serviceName = QString(),
                    bool forceAutoEmbed = false);

    /**
     * Ensures that this view's part supports the given @p mimeType,
     * otherwise calls changePart.
     */
    bool ensureViewSupports(const QString &mimeType,
                            bool forceAutoEmbed);

    /**
     * Call this to prevent next openUrl() call from changing history lists
     * Used when the same URL is reloaded (for instance with another view mode)
     *
     * Calling with lock=false is a hack reserved to the "find" feature.
     */
    void lockHistory(bool lock = true)
    {
        m_bLockHistory = lock;
    }

    /**
     * @return true if view can go back
     */
    bool canGoBack() const
    {
        return m_lstHistoryIndex > 0;
    }

    /**
     * @return true if view can go forward
     */
    bool canGoForward() const
    {
        return m_lstHistoryIndex != m_lstHistory.count() - 1;
    }

    /**
     * @return the position in the history
     */
    int historyIndex() const
    {
        return m_lstHistoryIndex;
    }

    int historyLength()
    {
        return m_lstHistory.count();
    }

    /**
     * Move in history. +1 is "forward", -1 is "back", you can guess the rest.
     */
    void go(int steps);

    /**
     * Helper function for go() and KonqViewManager
     */
    void restoreHistory();

    void setHistoryIndex(int index)
    {
        m_lstHistoryIndex = index;
    }

    /**
     * @return the history of this view
     */
    const QList<HistoryEntry *> &history()
    {
        return m_lstHistory;
    }

    /**
     * @return the HistoryEntry at postion @p pos
     */
    const HistoryEntry *historyAt(int pos);

    /**
     *
     */
    HistoryEntry *currentHistoryEntry() const
    {
        return m_lstHistory.value(m_lstHistoryIndex);
    }

    /**
     * Creates a deep copy of the @p other view's history buffers.
     */
    void copyHistory(KonqView *other);

    /**
     * Set the UrlLoader instance that is running something for this view
     * The main window uses this to store the UrlLoader for each child view.
     */
    void setUrlLoader(UrlLoader *loader);

    UrlLoader* urlLoader() const
    {
        return m_loader;
    }

    /**
     * Stop loading
     */
    void stop(bool keepTemporaryFile = false);

    /**
     * @brief The URL the view was requested to display
     *
     * This will be the same as the part's URL except when the URL was downloaded using
     * a part's \link KonqInterfaces::DownloaderExtension DownloaderExtension\endlink. In this case, this will be the URL the user originally
     * requested, while realUrl() is the URL of the downloaded file.
     *
     * @return the URL the view was requested to display
     * @see realUrl()
     * @see KonqInterfaces::DownloaderExtension
     */
    QUrl url() const {return m_requestedUrl;};

    /**
     * @brief The real URL displayed in the view
     *
     * If the URL represents a file which has been downloaded using a part's
     * \link KonqInterfaces::DownloaderExtension DownloaderExtension\endlink before being displayed, then the real URL is the local
     * file the URL was downloaded to. In all other cases, this is the same as url().
     * @return the URL of the local file where a file was downloaded by a part's \link KonqInterfaces::DownloaderExtension DownloaderExtension\endlink
     * or url() if the URL wasn't downloaded using \link KonqInterfaces::DownloaderExtension DownloaderExtension\endlink
     * @see KonqInterfaces::DownloaderExtension
     */
    QUrl realUrl() const;

    QUrl upUrl() const;

    /**
     * Get view's location bar URL, i.e. the one that the view signals
     * It can be different from url(), for instance if we display a index.html
     */
    QString locationBarURL() const
    {
        return m_sLocationBarURL;
    }

    /**
     * Get the URL that was typed to get the current URL.
     */
    QString typedUrl() const
    {
        return m_sTypedURL;
    }
    /**
     * Set the URL that was typed to get the current URL.
     */
    void setTypedURL(const QString &u)
    {
        m_sTypedURL = u;
    }

    /**
     * Returns the name filter, if any, like *.txt
     */
    QString nameFilter() const;

    /**
     * Returns true if the part was modified by the user (unsaved data).
     */
    bool isModified() const;

    /**
     * Return the security state of page in view
     */
    KonqMainWindow::PageSecurity pageSecurity() const
    {
        return m_pageSecurity;
    }

    /**
     * @return the part embedded into this view
     */
    KParts::ReadOnlyPart *part() const
    {
        return m_pPart;
    }

    /**
     * see KonqViewManager::removePart
     */
    void partDeleted()
    {
        m_pPart = nullptr;
    }

    /**
     * Return true if the loading in the view was aborted due to an error
     * or to user cancellation
     */
    bool aborted() const
    {
        return m_bAborted;
    }

    KParts::NavigationExtension *browserExtension() const;

    KParts::StatusBarExtension *statusBarExtension() const;

    /**
     * @return a pointer to the KonqFrame which the view lives in
     */
    KonqFrame *frame() const
    {
        return m_pKonqFrame;
    }

    /**
     * @return the servicetype this view is currently displaying
     * This is usually a mimetype, but it can be "Browser/View"
     * for toggle views like the sidebar or the embedded konsole.
     */
    QString serviceType() const
    {
        return m_serviceType;
    }

    /**
     * The mimetype this view is currently displaying.
     * Can be invalid, if serviceType() is not a mimetype.
     */
    QMimeType mimeType() const;

    /**
     * @return the servicetypes this view is capable to display
     */
    QStringList serviceTypes() const
    {
        return Konq::serviceTypes(m_service);
    }

    /**
     * Returns true if the part used in this view would be able to display
     * @p mimeType too. WARNING: this often leads to showing HTML in katepart...
     */
    bool supportsMimeType(const QString &mimeType) const;

    //TODO: it seems to be unused. Check and remove it
    /**
     * Whether the view in this part is suitable for web browsing.
     *
     * @return @b true if the part is suitable for browsing and @b false otherwise
     *
     * @warning Unfortunately, as far as I know, there's no way to detect whether a part is
     * suitable for browsing. This function works by hardcoding the component names of such
     * parts (currently only khtml, kwebkitpart and webenginepart). If other browsing parts
     * will be added, the list should be changed or another way must be found.
     **/
    bool isWebBrowsingPart() const;

    // True if showing a directory
    bool showsDirectory() const;

    // True if currently loading
    bool isLoading() const
    {
        return m_bLoading;
    }
    void setLoading(bool loading, bool hasPending = false);

    // True if "locked to current location" (and their view mode, in fact)
    bool isLockedLocation() const
    {
        return m_bLockedLocation;
    }
    void setLockedLocation(bool b);

    // True if can't be made active (e.g. dirtree).
    bool isPassiveMode() const
    {
        return m_bPassiveMode;
    }
    void setPassiveMode(bool mode);

    // True if 'link' symbol set
    bool isLinkedView() const
    {
        return m_bLinkedView;
    }
    void setLinkedView(bool mode);

    // True if toggle view
    void setToggleView(bool b)
    {
        m_bToggleView = b;
    }
    bool isToggleView() const
    {
        return m_bToggleView;
    }

    // True if it always follows the active view
    void setFollowActive(bool b)
    {
        m_bFollowActive = b;
    }
    bool isFollowActive()
    {
        return m_bFollowActive;
    }

    // True if locked to current view mode
    // Toggle views and passive views are locked to their view mode.
    bool isLockedViewMode() const
    {
        return m_bToggleView || m_bPassiveMode;
    }

    /**
     * @brief Whether the view can navigate to the given URL
     *
     * The view can always navigate to @p newUrl unless it's locked and @p newUrl is different
     * from the current URL. If the view is following the current view, then the it can navigate to
     * @p newUrl only if it's the same url as that of the current view
     *
     * @param newUrl the URL the view wants to navigate to
     * @return `true` if the view can navigate to @p newUrl and `false` otherwise
     */
    bool canNavigateTo(const QUrl &newUrl) const;

    // True if "builtin" (see X-KDE-BrowserView-Built-Into)
    //bool isBuiltinView() const { return m_bBuiltinView; }

    /**
     * The current viewmode used by this view -- only meaningful
     * when the part implements several view modes internally, like DolphinPart.
     */
    QString internalViewMode() const;
    /**
     * Switch the internal view mode in this view -- only meaningful
     * when the part implements several view modes internally, like DolphinPart.
     */
    void setInternalViewMode(const QString &viewMode);

    KPluginMetaData service() const
    {
        return m_service;
    }

    QString caption() const
    {
        return m_caption;
    }

    QVector<KPluginMetaData> partServiceOffers()
    {
        return m_partServiceOffers;
    }
    KService::List appServiceOffers()
    {
        return m_appServiceOffers;
    }

    KonqMainWindow *mainWindow() const
    {
        return m_pMainWindow;
    }

    // return true if the method was found, false if the execution failed
    bool callExtensionMethod(const char *methodName);
    bool callExtensionBoolMethod(const char *methodName, bool value);
    bool callExtensionURLMethod(const char *methodName, const QUrl &value);

    void setViewName(const QString &name);
    QString viewName() const;

    // True to enable the context popup menu
    void enablePopupMenu(bool b);
    bool isPopupMenuEnabled() const
    {
        return m_bPopupMenuEnabled;
    }

    KFileItemList selectedItems() const
    {
        return m_selectedItems;
    }

    void reparseConfiguration();

    void disableScrolling();

    QString dbusObjectPath();
    QString partObjectPath();

    // Set the KGlobal active componentData(the one used by KBugReport)
    void setActiveComponent();

    // Called before reloading this view. Sets args.reload to true, and offers to repost form data.
    // Returns false in case the reload must be canceled.
    bool prepareReload(KParts::OpenUrlArguments &args, BrowserArguments &browserArgs, bool softReload);

    // overload for the QString version
    void setLocationBarURL(const QUrl &locationBarURL);

    /**
     * Gives focus to the part's widget, after we just opened a URL in this part.
     * Does nothing on error:/ urls, so that the user can fix the wrong URL more easily.
     */
    void setFocus();

    /**
     * Returns true if this page is showing an error page, from an error: URL.
     * url() will still return the original URL, so it can't be used for this purpose.
     */
    bool isErrorUrl() const;

    /**
     * Saves config in a KConfigGroup
     */
    void saveConfig(KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options);
    void loadHistoryConfig(const KConfigGroup &config, const QString &prefix);

    /**
     * @brief Creates a view and load an URL according to the delayed loading data and the history content
     *
     * It does nothing if there's no delayed loading data
     */
    void loadDelayed();

    /**
     * @brief Changes the part used to display the current url
     * @param newPluginId the plugin id of the new part to use
     * @param newInternalViewMode the new view mode for the part, if any
     *
     * If the current URL is a temporary file, according to #m_tempFile, it __won't be deleted__. This is because
     * we want to display the same URL, not navigate to another one. Deleting the temporary file would mean
     * the new part doesn't actually have a file to display.
     */
    void switchEmbeddingPart(const QString &newPluginId, const QString &newInternalViewMode = {});

    /**
     * @brief Stores the information needed to restore the state of the view
     *
     * The history-related information is directly read in #m_lstHistory and #m_lstHistoryIndex,
     * while the part-specific information is stored in #m_delayedLoadingData.
     *
     * This function allows to load the content of the view only when it actually becomes visible
     * @param mimeType the mimetype of the URL
     * @param serviceName the id of the part to use
     * @param openUrl whether an URL should be loaded when the part will be created
     * @param url the URL to load when the part will be created
     * @param lockedLocation whether to block the URL when the view will be created
     * @param grp the object to read configuration options from
     * @param prexfix the prefix to add to configuration options names when reading from @p grp
     */
    void storeDelayedLoadingData(const QString &mimeType, const QString &serviceName, bool openUrl, const QUrl &url,
                                 bool lockedLocation, const KConfigGroup &grp, const QString &prefix);

    /**
     * @brief Whether or not loading this view has been delayed
     *
     * When this function returns `true` it means that loadDelayed() should be called next time
     * the tab containing this view is activated.
     *
     * @return `true` if the loading has been delayed and `false` otherwise
     */
    bool isDelayed() const;

Q_SIGNALS:

    /**
     * Signal the main window that the embedded part changed (e.g. because of changePart)
     */
    void sigPartChanged(KonqView *childView, KParts::ReadOnlyPart *oldPart, KParts::ReadOnlyPart *newPart);

    /**
     * Emitted in slotCompleted
     */
    void viewCompleted(KonqView *view);

public Q_SLOTS:
    /**
     * Store location-bar URL in the child view
     * and updates the main view if this view is the current one
     * May be different from url.
     */
    void setLocationBarURL(const QString &locationBarURL);
    /**
     * get an icon for the URL from the NavigationExtension
     */
    void setIconURL(const QUrl &iconURL);

    void setTabIcon(const QUrl &url);

    void setCaption(const QString &caption);

    void setPageSecurity(int);

    // connected to the KROP's KIO::Job
    // but also to UrlLoader's job
    void slotInfoMessage(KJob *, const QString &msg);

private Q_SLOTS:
    // connected to the KROP's KIO::Job
    void slotStarted(KIO::Job *job);
    void slotCompleted();
    void slotCompleted(bool);
    void slotCanceled(const QString &errMsg);
    void slotPercent(KJob *, unsigned long percent);
    void slotSpeed(KJob *, unsigned long bytesPerSecond);

    //The four slots below are called in response to the several variants of the NavigationExtension::popupMenu signal.
    //There are four variants of this signal: two come from KParts::NavigationExtension and two from BrowserExtension,
    //which (as of KF6) has been moved to Konqueror. For each of them, there are two variants, one taking a KFileItemList
    //and one taing a QUrl.
    //
    //Since these slots need to be disconnected, we can't just use a lambda
#if QT_VERSION_MAJOR > 5
    void slotBrowserPopupMenuFiles(const QPoint &global,
        const KFileItemList &items, const KParts::OpenUrlArguments &args, const BrowserArguments &bargs,
        KParts::NavigationExtension::PopupFlags flags, const KParts::NavigationExtension::ActionGroupMap &actionGroups);
    void slotBrowserPopupMenuUrl(const QPoint &global,
        const QUrl &url, mode_t mode, const KParts::OpenUrlArguments &arguments, const BrowserArguments &bargs,
        KParts::NavigationExtension::PopupFlags flags, const KParts::NavigationExtension::ActionGroupMap &actionGroups);
#endif
    void slotPopupMenuFiles(const QPoint &global,
        const KFileItemList &items, const KParts::OpenUrlArguments &args, KParts::NavigationExtension::PopupFlags flags,
        const KParts::NavigationExtension::ActionGroupMap &actionGroups);
    void slotPopupMenuUrl(const QPoint &global,
        const QUrl &url, mode_t mode, const KParts::OpenUrlArguments &arguments, KParts::NavigationExtension::PopupFlags flags,
        const KParts::NavigationExtension::ActionGroupMap &actionGroups);

    /**
     * Connected to the NavigationExtension
     */
    void slotSelectionInfo(const KFileItemList &items);
    void slotMouseOverInfo(const KFileItem &item);
    void slotOpenURLNotify();
    void slotEnableAction(const char *name, bool enabled);
    void slotSetActionText(const char *name, const QString &text);
    void slotMoveTopLevelWidget(int x, int y);
    void slotResizeTopLevelWidget(int w, int h);
    void slotRequestFocus(KParts::ReadOnlyPart *);

private:
    /**
     * Replace the current part with a new part, created by @p viewFactory.
     *
     * If the factory isn't valid and @p allowPlaceholder is `true`, a PlaceholderPart
     * will be created.
     *
     * @param viewFactory the factory to use to create the part
     * @param allowPlaceholder whether a PlaceholderPart should be used if @p viewFactory isn't valid.
     *  If this is `false` and @p viewFactory is invalid, nothing will be done
     */
    void switchView(KonqViewFactory &viewFactory, bool allowPlaceholder = false);

    /**
     * Connects the internal part to the main window.
     * Do this after creating it and before inserting it.
     */
    void connectPart();

    /**
     * Creates a new entry in the history.
     */
    void createHistoryEntry();

    /**
     * Appends a entry in the history.
     */
    void appendHistoryEntry(HistoryEntry *historyEntry);

    /**
     * Updates the current entry in the history.
     * @param needsReload whether page is fully loaded
     * (not done in openUrl, to be able to revert if aborting)
     */
    void updateHistoryEntry(bool needsReload);

    void aboutToOpenURL(const QUrl &url, const KParts::OpenUrlArguments &args = KParts::OpenUrlArguments());

    void setPartMimeType();

    void finishedWithCurrentURL();

    bool eventFilter(QObject *obj, QEvent *e) override;

    /**
     * @brief Casts the part associated with the view to a Konq::PlaceholderPart
     * @return the part associated with the view cast to a Konq::PlaceholderPart or `nullptr` if it's not a Konq::PlaceholderPart
     */
    Konq::PlaceholderPart *placeholderPart() const;

////////////////// private members ///////////////

    KParts::ReadOnlyPart *m_pPart;

    QString m_sLocationBarURL;
    QString m_sTypedURL;
    KonqMainWindow::PageSecurity m_pageSecurity;
    KFileItemList m_selectedItems;

    /**
     * The full history (back + current + forward)
     */
    QList<HistoryEntry *> m_lstHistory;
    /**
     * The current position in the history
     */
    int m_lstHistoryIndex;

    /**
     * The post data that _resulted_ in this page.
     * e.g. when submitting a form, and the result is an image, this data will be
     * set (and saved/restored) when the image is being viewed. Necessary for reload.
     */
    QByteArray m_postData;
    QString m_postContentType;
    bool m_doPost;

    /**
     * The referrer that was used to obtain this page.
     */
    QString m_pageReferrer;

    KonqMainWindow *m_pMainWindow;
    UrlLoader *m_loader = nullptr;
    KonqFrame *m_pKonqFrame;

    uint m_bLoading: 1;
    uint m_bLockedLocation: 1;
    uint m_bPassiveMode: 1;
    uint m_bLinkedView: 1;
    uint m_bToggleView: 1;
    uint m_bLockHistory: 1;
    uint m_bAborted: 1;
    uint m_bGotIconURL: 1;
    uint m_bPopupMenuEnabled: 1;
    uint m_bFollowActive: 1;
    uint m_bPendingRedirection: 1;
    uint m_bBuiltinView: 1;
    uint m_bURLDropHandling: 1;
    uint m_bDisableScrolling: 1;
    uint m_bErrorURL: 1;
    QVector<KPluginMetaData> m_partServiceOffers;
    KService::List m_appServiceOffers;
    KPluginMetaData m_service;
    QString m_serviceType;
    QString m_caption;
    QString m_tempFile;
    QString m_dbusObjectPath;
    QUrl m_requestedUrl;

#if QT_VERSION_MAJOR < 6
#ifdef KActivities_FOUND
    KActivities::ResourceInstance *m_activityResourceInstance;
#endif
#endif
};

#endif
