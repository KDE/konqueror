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
#include "browserarguments.h"

#include <kservice.h>
#include <KParts/NavigationExtension>
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
class StatusBarExtension;
}

namespace Konq {
    class PlaceholderPart;
    class ViewType;
}

/**
 * @brief Struct representing an entry in history
 *
 * An entry contains information about a visited URL, including the details on the URL itself,
 * on how it was reached and on how it was visualized.
 *
 * This information allows to restore the contents of a view after it has been closed (up to a degree
 * which depends on the part displaying the page)
 */
struct HistoryEntry {

    /**
     * @brief Saves the history entry to a configuration group
     *
     * To distinguish between different history entries, each key in the group is
     * prefixed with a string which should be unique for each configuration entry
     * (typically it's a progressive number).
     *
     * @param config the configuration group to save the entry to
     * @param prefix the unique prefix to add to each key
     * @param options what to save
     */
    void saveConfig(KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options);

    /**
     * @brief Creates a history entry from data in the given configuration group
     *
     * @param config the configuration group to read the data from
     * @param prefix the prefix used in the configuration object
     * @param options the options to use when reading data. It describes what to read from the configuration object
     * @return a new HistoryEntry with the information read from the configuration group
     */
    static HistoryEntry* fromDelayedLoadingData(const KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options);

    QUrl url; //!< The URL of the entry
    /**
     * @brief The URL entered in the location bar
     *
     * It can be different from #url when showing a `index.html` URL
     */
    QString locationBarURL;
    QString title; //!< The title of the page
    QByteArray buffer; //!< Data describing the state of the page
    /**
     * @brief A string describing the view type
     *
     * It is one of the values returned by Konq::ViewType::toString()
     */
    QString strViewType;
    QString strServiceName; //!< The plugin id of the part used to show the entry

    /**
     * @brief The data passed to the POST request which produced the URL
     *
     * It will be empty if the URL wasn't the result of a POST data.
     */
    QByteArray postData;

    /**
     * @brief The content type of a POST request
     *
     * @todo According to the comments in BrowserArguments, this is KHTML-specific,
     * so it shouldn't be needed anymore
     */
    QString postContentType;
    bool doPost; //!< Whether or not the URL is the result of a POST request
    QString pageReferrer; //!< The referrer of the URL
    KonqMainWindow::PageSecurity pageSecurity; //!< The encryption status of the URL

    /**
     * @brief Whether to call `ReadOnly::openUrl()` on the page when it's restored from a configuration file
     *
     * If this is `false`, `openUrl()` won't be used and `KParts::NavigationExtension::restoreState()` will
     * be used instead.
     */
    bool reload;
};

/**
 * @brief Class wrapping a part
 *
 * It implements the interactions between a part and the rest of Konqueror.
 *
 * Note that this class is not a `QWidget` and doesn't contain the widget
 * associated with the part. Each view is instead associated with a KonqFrame,
 * and it's the frame which contains the part widget.
 *
 * Each view also contains a history with all the URLs which have been shown in it
 *
 * A view can be in several special states:
 * - locked: a locked view can't move away from the current URL or change its view mode.
 * Attempts to change the URL or view mode will fail
 * - linked: the view is linked with other views and it should always display the
 * same URL as them (unless locked)
 * - passive: a passive view can never become the active view. Currently, only the
 * sidebar part is passive
 * - toggle view: toggle views are views which can be hidden or shown using a button
 * (for example, the side bar or the terminal emulator)
 */
class KONQ_TESTS_EXPORT KonqView : public QObject
{
    Q_OBJECT
public:

    /**
     * @brief Constructor which creates a view containing a PlaceholderPart
     *
     * @param viewFrame the frame which will contain the view
     * @param mainWindow the main window the view will belong to
     */
    KonqView(KonqFrame *viewFrame, KonqMainWindow* mainWindow);

    /**
     * @brief Constructor which creates a view containing a part corresponding to
     * the given plugin
     *
     * @param viewFactory the factory which will create the part
     * @param viewFrame the frame where to create the view
     * @param mainWindow the main window where the view will be
     * @param service the metadata of the plugin providing the part to create
     * @param partServiceOffers a list of part offers found by the factory
     * @param appServiceOffers a list of app offers found by the factory
     * @param type the part capability or mimetype
     * @param passiveMode whether to initially make the view passive
     */
    KonqView(KonqViewFactory &viewFactory,
             KonqFrame *viewFrame,
             KonqMainWindow *mainWindow,
             const KPluginMetaData &service,
             const QVector<KPluginMetaData> &partServiceOffers,
             const KService::List &appServiceOffers,
             const Konq::ViewType &type,
             bool passiveMode);

    ~KonqView() override; //!< Destructor

    /**
     * @brief Whether devtools (inspect element, etc.) is available for this view.
     *
     * This is determined by checking whether the part for this view
     * has a method `setInspectedPart(KParts::ReadOnlyPart *)`.
     *
     * @return @b true if devtools is available for this view, @b false otherwise.
     */
    bool isDevtoolsAvailable() const;

    /**
     * @brief Opens the given URL in the current part.
     *
     * This assumes that the current part is actually able to display the URL:
     * it's up to the caller to ensure this is true.
     *
     * @param url the URL to open
     * @param locationBarURL the URL to set in the location bar (see @ref setLocationBarURL)
     * @param nameFilter a filter to apply to the view's contents (e.g. *.cpp). It makes
     * sense only for some parts (e.g. DolphinPart)
     * @param tempFile whether to delete the URL shown in the part after use
     * @param requestedUrl the URL which the user requested, if it's different from @p url.
     * For example, when the user wants to download and display a remote file, the file
     * will be first downloaded to a local temporary location and then displayed: @p url will
     * be the URL of the local file, while @p requestedUrl will be the remote URL
     */
    void openUrl(const QUrl &url,
                 const QString &locationBarURL,
                 const QString &nameFilter = QString(),
                 bool tempFile = false,
                 const QUrl &requestedUrl={});

    /**
     * @brief Makes this view a duplicate of another view
     *
     * This function calls openUrl() with arguments determined from @p otherView,
     * so that the two views behave in the same way.
     *
     * @note If @p otherView shows a downloaded file, this function won't make this
     * view display the same file but will create a copy of it to avoid problems
     * when deleting the file as one of the views is closed.
     *
     * @param otherView the view to duplicate
     */
    void duplicateView(KonqView *otherView);

    /**
     * @brief Changes the part associated with the view
     *
     * If creates a new part which can display the given mimetype or has the given `PartCapability`.
     * If @p type represents a mimetype, the preferred part for that mimetype is used,
     * unless @p serviceName is non-empty, in which case the part corresponding to that name
     * is used if possible.
     *
     * If this would create a new part of the same type as the current one (either because
     * it has the same plugin id as @p serviceName or because it's already the preferred part
     * for @p type), nothing is done. Instead, if the current part supports @p type but isn't
     * the preferred part for it and it doesn't have the same plugin id as @p serviceName, a new
     * part is created.
     *
     * @note If the view is locked (according to isLockedViewMode()), nothing is done and `false` is returned.
     * @warning The caller should call stop() before calling this method.
     *
     * @param type the mimetype the part should support or the view capability the part should have
     * @param serviceName the pluginId of a specific part to use
     * @param forceAutoEmbed whether to force embedding even if the user chose to display
     * the mimetype in a separate application
     * @return `true` if the part associated with the view supports @p type and `false` if it wasnt't possible
     * to create such a part.
     */
    bool changePart(const Konq::ViewType &type,
                    const QString &serviceName = QString(),
                    bool forceAutoEmbed = false);

    /**
     * @brief Ensures that the part associated with the view supports the given mimetype
     *
     * If the part currently associated with the view doesn't support @p mimetype,
     * changePart() is called to replace it with a new part which supports it. If the
     * current part already supports @p mimetype, nothing is done.
     *
     * @param mimeType the mimetype which should be supported by the part in the view
     * @param forceAutoEmbed whether to force embedding even if the user chose to display
     * the mimetype in a separate application
     */
    bool ensureViewSupports(const QString &mimeType,
                            bool forceAutoEmbed);

    /**
     * @brief Prevents then next openUrl() call to modify history
     *
     * Usually, when openUrl() is called, the newly opened URL is added to the history.
     * After calling this function, the next time openUrl() is called, it won't alter
     * history.
     *
     * @note This only prevents openUrl() to change history once: openUrl() itself will
     * remove the lock on history the next time it's called.
     *
     * @param lock it should always be `true` (passing `false` is a hack reserved to the "find" feature)
     */
    void lockHistory(bool lock = true)
    {
        m_bLockHistory = lock;
    }

    /**
     * @brief Whether or not it is possible to move backwards in history from the current URL
     *
     * It's possible to move backwards in history if the current URL doesn't correspond to
     * the first entry in history.
     *
     * @return `true` if it's possible to move backwards in history from the current URL and
     * `false` if it's impossible.
     */
    bool canGoBack() const
    {
        return m_lstHistoryIndex > 0;
    }

    /**
     * @brief Whether or not it is possible to move forwards in history from the current URL
     *
     * It's possible to move forwards in history if the current URL doesn't correspond to
     * the last entry in history.
     *
     * @return `true` if it's possible to move forwards in history from the current URL and
     * `false` if it's impossible.
     */
    bool canGoForward() const
    {
        return m_lstHistoryIndex != m_lstHistory.count() - 1;
    }

    /**
     * @brief The position of the current URL in history
     *
     * @return the position of the current URL in history
     */
    int historyIndex() const
    {
        return m_lstHistoryIndex;
    }

    /**
     * @brief The number of entries in history
     * @return the number of entries in history
     */
    int historyLength()
    {
        return m_lstHistory.count();
    }

    /**
     * @brief Moves in history
     *
     * @param steps the number of steps to move in history. A positive number means
     * moving forwards while a negative number means moving backwards
     *
     * Move in history. +1 is "forward", -1 is "back", you can guess the rest.
     */
    void go(int steps);

    /**
     * @brief Changes the state of the view so that it matches that described in
     * the current history entry
     *
     * The way in which the views tries to restore its contents depends on whether
     * the part provide a `KParts::NavigationExtension` and on whether the history
     * entry buffer is empty or not:
     * - if the part has a navigation extension and the buffer is not empty,
     * `NavigationExtension::restoreState()` is called with the buffer as argument
     * - if the part doesn't have a navigation extension, its `openUrl()` method is
     * used
     * - if the part has a navigation extension but the buffer is empty, one of the
     * two behaviors above is used, depending on the value of @p openUrlIfNoBuffer.
     *
     * @note Ensure to call setHistoryEntry() with the current index before calling
     * this function
     *
     * @param openUrlIfNoBuffer what to do if the history entry's buffer is empty:
     * if `true` call the part's `openUrl()` method, if `false` use `restoreState()`,
     * even if most likely it will do nothing as there's no state to restore.
     * This parameter is ignored if the part doesnt' have a `NavigationExtension`
     */
    void restoreHistory(bool openUrlIfNoBuffer = false);

    /**
     * @brief Tells the view which position in history is being shown
     *
     * @param index the new position in history. It must go from 0 (the first element in history)
     * to one less than the number of elements in history()
     */
    void setHistoryIndex(int index)
    {
        m_lstHistoryIndex = index;
    }

    /**
     * @brief The history associated with the view
     *
     * The history is a list of pointers to HistoryEntry objects, from the oldest to the newest
     *
     * @return the history associated with the view
     */
    const QList<HistoryEntry *> &history()
    {
        return m_lstHistory;
    }

    /**
     * @brief The history element at the given position
     *
     * @param pos the position of the history element to return. It must be between 0
     * and one less than the number of elements in history().
     *
     * @return the history element at postion @p pos or `nullptr` if @p pos is outside
     * the allowed range
     */
    const HistoryEntry *historyAt(int pos);

    /**
     * @brief The history entry corresponding to the URL currently shown in the view
     * @return the history entry corresponding to the URL currently shown in the view
     */
    HistoryEntry *currentHistoryEntry() const
    {
        return m_lstHistory.value(m_lstHistoryIndex);
    }

    /**
     * @brief Makes this view's history become equal to another view's history
     *
     * This method creates a deep copy of the other view's history, then replaces
     * this view's history with that copy. The result is that the two views have the
     * same history but represented by different objects, so that they can be changed
     * independently.
     *
     * @param other the view to copy history from. If this is `nullptr`, nothing is done
     */
    void copyHistory(KonqView *other);

    /**
     * @brief Sets the UrlLoader which is being used to load an URL in this view
     *
     * The main window uses this to store the UrlLoader for each child view.
     *
     * @param loader the UrlLoader which is being used
     */
    void setUrlLoader(UrlLoader *loader);

    /**
     * @brief The UrlLoader being used to load an Url in the view
     * @return the UrlLoader being used to load an Url in the view or `nullptr` if there's no such object
     */
    UrlLoader* urlLoader() const
    {
        return m_loader;
    }

    /**
     * @brief Stops the view from loading or displaying the current URL
     *
     * After a call to this method, the view is ready to open another URL and to
     * change the part used to display the current URL.
     *
     * Unless @p keepTemporaryFile is `true`, this will delete the file being displayed,
     * if it's temporary.
     *
     * @warning Always call this function before calling changePart()
     *
     * @param keepTemporaryFile whether or not to delete the temporary file being displayed.
     * This should be `true` when this method is called in preparation to changing the part
     * used to display the current URL.
     */
    void stop(bool keepTemporaryFile = false);

    /**
     * @brief The URL the view was requested to display
     *
     * This will be the same as the part's URL except when the URL was downloaded using
     * a KonqInterfaces::DownloadJob. In this case, this will be the URL the user originally
     * requested, while realUrl() is the URL of the downloaded file.
     *
     * @return the URL the view was requested to display
     * @see realUrl()
     * @see KonqInterfaces::DownloadJob
     */
    QUrl url() const;

    /**
     * @brief The real URL displayed in the view
     *
     * If the URL represents a file which has been downloaded using a
     * KonqInterfaces::DownloadJob before being displayed, then the real URL is the local
     * file the URL was downloaded to. In all other cases, this is the same as url().
     * @return the URL of the local file where a file was downloaded by a KonqInterfaces::DownloadJob
     * and url() otherwise
     * @see KonqInterfaces::DownloadJob
     */
    QUrl realUrl() const;

    /**
     * @brief The parent URL for the current URL
     *
     * This uses `KIO::upUrl()` to determine the parent URL.
     *
     * @return the parent URL for the current URL
     */
    QUrl upUrl() const;

    /**
     * @brief The URL in the view to be shown to the user
     *
     * It can be different from url(), for example when displaying a index.html.
     *
     * @return The URL to be shown to the user
     */
    QString locationBarURL() const
    {
        return m_sLocationBarURL;
    }

    /**
     * @brief The URL the user typed in the location bar to get the current URL
     * @return the URL the user typed in the location bar to get the current URL
     */
    QString typedUrl() const
    {
        return m_sTypedURL;
    }

    /**
     * @brief Sets the URL the user typed in the location bar to get the current URL
     * @param u the URL the user typed in the location bar to get the current URL
     */
    void setTypedURL(const QString &u)
    {
        m_sTypedURL = u;
    }

    /**
     * @brief The name filter applied to the view
     *
     * A name filter is a shell glob (e.g. `*.txt`) which can be used to display only some elements in the view.
     * This is stored as a property of the part, in case the part knows what to do with it (currently, only DolphinPart does).
     *
     * @return the name filter applied to the view
     */
    QString nameFilter() const;

    /**
     * @brief Whether the part was modified by the user and has unsaved data
     * @return true if the part was modified by the user and has unsaved data and false if it wasn't modified or
     * it was modified but the data has already been saved
     */
    bool isModified() const;

    /**
     * @brief The security state of the URL in the view
     * @return the security state of the URL in the view
     */
    KonqMainWindow::PageSecurity pageSecurity() const
    {
        return m_pageSecurity;
    }

    /**
     * @brief The part associated with this view
     * @return the part associated with this view or `nullptr` if no part is associated with the view
     */
    KParts::ReadOnlyPart *part() const
    {
        return m_pPart;
    }

    /**
     * @brief Informs the view that the associated part has been deleted
     *
     * This has the effect of setting #m_pPart to `nullptr`
     */
    void partDeleted()
    {
        m_pPart = nullptr;
    }

    /**
     * @brief Whether loading the URL in the view was aborted
     *
     * Loading can be aborted because of an error or because of the user canceling it
     * @return `true` if loading was aborted and `false` otherwise
     */
    bool aborted() const
    {
        return m_bAborted;
    }

    /**
     * @brief The navigation extension provided by the part in the view
     *
     * @return the navigation extension provided by the part in the view or `nullptr`
     * if the part doesn't provide a navigation extension
     */
    KParts::NavigationExtension *navigationExtension() const;

    /**
     * @brief The statusbar extension provided by the part in the view
     *
     * @return the statusbar extension provided by the part in the view or `nullptr`
     * if the part doesn't provide a `KParts::StatusBarExtension`
     */
    KParts::StatusBarExtension *statusBarExtension() const;

    /**
     * @brief The frame the view belongs to
     * @return the frame the view belongs to
     */
    KonqFrame *frame() const
    {
        return m_pKonqFrame;
    }

    /**
     * @brief The type the view is displaying
     *
     * This can be either mimetype of the URL shown in the part or `KParts::BrowserView`
     * (for parts like the sidebar or the embedded Konsole part).
     *
     * It should never be `KParts::ReadOnlyPart`, because all parts have this capability,
     * or `KParts::ReadWritePart`
     * @return The type the view is displaying
     */
    Konq::ViewType type() const
    {
        return m_type;
    }

    /**
     * @brief The mimetype the view is currently displaying
     *
     * @return the mimetype shown in the part or an invalid mimetype if type() is
     * a part capability
     */
    QMimeType mimeType() const;

    /**
     * @brief The capabilities of the part associated with the view
     * @return the capabilities of the part associated with the view
     */
    KParts::PartCapabilities partCapabilities() const {
        return Konq::partCapabilities(m_service);
    }

    /**
     * @brief Whether the part associated with the view would be able to display
     * a given mimetype
     *
     * @warning blindly relying on this function can lead to unwanted results. For
     * example, Kate part supports `text/html` which could lead to displaying web
     * pages in Kate part which (usually) is not what the user wants.
     *
     * @param mimeType the mimetype to check
     * @return `true` if the part associated with this view supports @p mimeType
     * (according to `QMimeType::inherits()`) and `false` otherwise
     */
    bool supportsMimeType(const QString &mimeType) const;

    /**
     * @brief Whether the view is showing a directory
     *
     * This method assumes that if the part supports the `inode/directory` mimetype,
     * then it will always be showing a directory.
     *
     * @return `true` if the part associated with the view is showing a directory and
     * `false` otherwise
     */
    bool showsDirectory() const;

    /**
     * @brief Whether the part is currently loading an URL
     *
     * @return `true` if an URL is currently being loaded and `false` otherwise
     */
    bool isLoading() const
    {
        return m_bLoading;
    }

    /**
     * @brief Tells the view that the part has started loading an URL
     *
     * @warning This also updates the main window toolbars and, if @p loading is `true`,
     * gives focus to the part widget, except when the URL which is being loaded
     * has a `konq` scheme.
     *
     * @param loading whether the part is loading an URL or has stopped doing so (for whatever reason)
     * @param hasPending whether there are actions to be executed on a delayed timer
     */
    void setLoading(bool loading, bool hasPending = false);

    /**
     * @brief Whether the view is locked to the current URL and view mode
     *
     * If the view is locked, any attempt to navigate to another URL or to change
     * the view mode will fail.
     *
     * @return `true` if the view is locked and `false` otherwise
     */
    bool isLockedLocation() const
    {
        return m_bLockedLocation;
    }

    /**
     * @brief Locks or unlocks the view to the current URL and view mode
     *
     * If the view is locked, any attempt to navigate to another URL or to change
     * the view mode will fail.
     *
     * @param b whether the view should be locked or unlocked
     */
    void setLockedLocation(bool b);

    /**
     * @brief Whether the view is in passive mode
     *
     * A view in passive mode can never become active. An example of such view is
     * the sidebar
     *
     * @return `true` if the view is in passive mode and `false` otherwise
     */
    bool isPassiveMode() const
    {
        return m_bPassiveMode;
    }

    /**
     * @brief Turns passive mode on or off
     *
     * If passive mode is turned on and this view was the active one, it activates
     * another view.
     *
     * @param mode whether to turn passive mode on or off
     */
    void setPassiveMode(bool mode);

    /**
     * @brief Whether this view is linked with other views
     * @return `true` if this view is linked with other views and `false` otherwise
     */
    bool isLinkedView() const
    {
        return m_bLinkedView;
    }

    /**
     * @brief Turn on or off linked mode
     *
     * This also updated the link indicator in the main window, if needed.
     *
     * @param mode whether linked mode should be turned on or off
     */
    void setLinkedView(bool mode);

    /**
     * @brief Sets whether this is a view which can be toggle on or off or it's a normal view
     * @param b `true` if this view can be turned on or off (such as the sidebar) or `false` if it's a regular view
     */
    void setToggleView(bool b)
    {
        m_bToggleView = b;
    }

    /**
     * @brief Whether this is a view which can be toggle on or off or it's a normal view
     * @return `true` if this view can be turned on or off (such as the sidebar) or `false` if it's a regular view
     */
    bool isToggleView() const
    {
        return m_bToggleView;
    }

    /**
     * @brief Sets whether this view should always display the same URL as the active view
     * @param b `true` if this view should always display the same URL as the active view and
     * `false` otherwise
     */
    void setFollowActive(bool b)
    {
        m_bFollowActive = b;
    }

    /**
     * @brief Whether this view should always display the same URL as the active view
     * `true` if this view should always display the same URL as the active view and
     * `false` otherwise
     */
    bool isFollowActive()
    {
        return m_bFollowActive;
    }

    /**
     * @brief Whether this view is locked in the current view mode
     *
     * Currently, toggle views and passive views are always locked to the current view mode
     * and they are the only ones to do so.
     *
     * @return `true` if this is a toggle view or a passive view and `false` otherwise
     */
    bool isLockedViewMode() const
    {
        return m_bToggleView || m_bPassiveMode;
    }

    /**
     * @brief Whether the view can navigate to the given URL
     *
     * The view can always navigate to @p newUrl unless it's locked and @p newUrl is different
     * from the current URL. If the view is following the current view, then it can navigate to
     * @p newUrl only if it's the same url as that of the current view
     *
     * @param newUrl the URL the view wants to navigate to
     * @return `true` if the view can navigate to @p newUrl and `false` otherwise
     */
    bool canNavigateTo(const QUrl &newUrl) const;

    /**
     * @brief The current viewmode used by this view
     *
     * Some parts, such as Dolphin Part, can display an URL in different ways, called
     * view modes (for Dolphin Part these are: Icon Mode, Details Mode, Compact Mode).
     * The user can then choose which view mode to use.
     *
     * @return the name of the view mode which being used by the part associated with
     * this view. It returns an empty string if the part doesn't provide multiple view
     * modes
     */
    QString internalViewMode() const;

    /**
     * Switch the internal view mode in this view -- only meaningful
     * when the part implements several view modes internally, like DolphinPart.
     */
    /**
     * @brief Changes the view mode used by the current part
     *
     * Some parts, such as Dolphin Part, can display an URL in different ways, called
     * view modes (for Dolphin Part these are: Icon Mode, Details Mode, Compact Mode).
     * The user can then choose which view mode to use.
     *
     * This method does nothing if the part doesn't provide multiple view modes
     *
     * @param viewMode the name of the view mode to use
     */
    void setInternalViewMode(const QString &viewMode);

    /**
     * @brief The metadata of the plugin providing the current part
     * @return the metadata of the plugin providing the current part
     */
    KPluginMetaData service() const
    {
        return m_service;
    }

    /**
     * @brief The caption associated with the view
     *
     * This is the text to be shown in the tab bar when the view is active
     *
     * @return The caption associated with the view
     */
    QString caption() const
    {
        return m_caption;
    }

    /**
     * @brief A list of plugins which provide parts able to display the current URL
     * @return a list of plugins which provide parts able to display the current URL
     */
    QVector<KPluginMetaData> partServiceOffers()
    {
        return m_partServiceOffers;
    }

    /**
     * @brief A list of services corresponding to applications able to open the current URL
     * @return a list of services corresponding to applications able to open the current URL
     */
    KService::List appServiceOffers()
    {
        return m_appServiceOffers;
    }

    /**
     * @brief The main window the frame associated with the view belongs to
     * @return the main window the frame associated with the view belongs to
     */
    KonqMainWindow *mainWindow() const
    {
        return m_pMainWindow;
    }

    /**
     * @brief Calls a signal or slot with no arguments on the NavigationExtension provided by the current part
     *
     * If the part doesn't provide a navigation part, this function does nothing.
     * @param methodName the name of the signal or slot to call. It should be a signal or slot
     * which takes no arguments
     * @return `true` if the method was called and `false` it it wasn't (because
     * it doesn't exist or takes different parameters)
     */
    bool callExtensionMethod(const char *methodName);

    /**
     * @brief Calls a signal or slot with a boolean argument on the NavigationExtension provided by the current part
     *
     * If the part doesn't provide a navigation part, this function does nothing.
     *
     * @param methodName the name of the signal or slot to call. It should be a signal or slot
     * which takes a single boolean argument
     * @return `true` if the method was called and `false` it it wasn't (because
     * it doesn't exist or takes different parameters)
     */
    bool callExtensionBoolMethod(const char *methodName, bool value);

    /**
     * @brief Calls a signal or slot with a `QUrl` argument on the NavigationExtension provided by the current part
     *
     * If the part doesn't provide a navigation part, this function does nothing.
     *
     * @param methodName the name of the signal or slot to call. It should be a signal or slot
     * which takes a single argument of type `const QUrl &`
     * @return `true` if the method was called and `false` it it wasn't (because
     * it doesn't exist or takes different parameters)
     */
    bool callExtensionURLMethod(const char *methodName, const QUrl &value);

    /**
     * @brief Assigns a name to the part currently associated with the view
     *
     * @warning This changes the `objectName()` of the part. It has no effect on
     * the view itself.
     *
     * @param name the name to give to the part
     */
    void setViewName(const QString &name);

    /**
     * @brief The object name of the part associated with the view
     *
     * @return the object name of the part associated with the view or an empty string
     * if no part is associated with the view
     */
    QString viewName() const;

    /**
     * @brief Enables or disables the context menu for the part
     *
     * This connects or disconnects several popup menu-related signals emitted by the
     * part's navigation or browser extension.
     *
     * @param b whether to enable or disable the context menu
     */
    void enablePopupMenu(bool b);

    /**
     * @brief Whether the context menu is enabled or disabled
     * @return whether the context menu is enabled or disabled
     */
    bool isPopupMenuEnabled() const
    {
        return m_bPopupMenuEnabled;
    }

    /**
     * @brief A list of selected files in the part associated with the view
     *
     * If the part doesn't support selecting files, this list will always be empty.
     * Currently, only Dolphin part supports selecting files.
     *
     * @return A list of selected files
     */
    KFileItemList selectedItems() const
    {
        return m_selectedItems;
    }

    /**
     * @brief Updates the configuration so that it matches the configuration files
     *
     * It also ensures that the associated part updates its configuration
     */
    void reparseConfiguration();

    /**
     * @brief Disables scrollbars in the associated part
     */
    void disableScrolling();

    /**
     * @brief The object path representing the view in DBus
     *
     * If needed, this also registers the view with DBus
     * @return the object path representing the view in DBus
     */
    QString dbusObjectPath();

    /**
     * @brief The object path representing the part associated with the view in DBus
     * @return the object path representing the part associated with the view in DBus
     */
    QString partObjectPath();

    // Called before reloading this view. Sets args.reload to true, and offers to repost form data.
    // Returns false in case the reload must be canceled.

    /**
     * @brief Prepares for reloading the view
     *
     * If the URL shown in the view is the result of data sent with POST, it asks the user
     * whether to confirm reloading.
     *
     * @param args the arguments to pass to the part to open the URL. It's not `const`
     * because this method needs to change its `referrer` metadata
     * @param browserArgs the browser arguments to pass to the part to open the URL. It's not `const`
     * because this method needs to call setReload() on it
     */
    bool prepareReload(KParts::OpenUrlArguments &args, BrowserArguments &browserArgs);

    /**
     * @brief Overload of setLocationBarURL(const QString &)
     * @param locationBarURL the URL to show to the user
     */
    void setLocationBarURL(const QUrl &locationBarURL);

    /**
     * Gives focus to the part's widget, after we just opened a URL in this part.
     * Does nothing on error:/ urls, so that the user can fix the wrong URL more easily.
     */

    /**
     * @brief Gives focus to the part's widget, unelss the view is showing an `error:` URL
     *
     * This function is meant to be called just after a new URL has been opened in the part.
     * The idea is that when a new URL is opened, most likely the user wants to interact with
     * the part, except when it displays an error. In that case, leaving the focus where it is
     * allows the user to more easily fix the error (in particular if the error is the result
     * of a wrong URL entered in the location bar).
     */
    void setFocus();

    /**
     * @brief Whether this view is showing an error page
     *
     * @note This function returns the correct value even before the part has finished
     * loading the URL, at a time when url() still returns the old URL.
     *
     * @return `true` if the view is showing an error page and `false` otherwise
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
     * @brief Changes the part and/or the internal view mode used to display the current URL
     *
     * If the new part is the same as the current part, only the view mode will be changed, if needed.
     *
     * If the current URL is a temporary file, according to #m_tempFile, it __won't be deleted__. This is because
     * we want to display the same URL, not navigate to another one. Deleting the temporary file would mean
     * the new part doesn't actually have a file to display.
     *
     * @param newPluginId the plugin id of the new part to use
     * @param newInternalViewMode the new view mode for the part, if any. Ignored if empty
     */
    void switchViewMode(const QString &newPluginId, const QString &newInternalViewMode = {});

    /**
     * @brief Stores the information needed to restore the state of the view
     *
     * The history-related information is directly read in #m_lstHistory and #m_lstHistoryIndex,
     * while the part-specific information is stored in the placeholder part returned by
     * placeholderPart().
     *
     * This function allows to load the content of the view only when it actually becomes visible
     * @param type the view type. It should be a mimetype and not a `KParts::PartCapability` because
     * (currently) toggle views can't be delayed
     * @param serviceName the id of the part to use
     * @param openUrl whether an URL should be loaded when the part will be created
     * @param url the URL to load when the part will be created
     * @param lockedLocation whether to block the URL when the view will be created
     * @param grp the object to read configuration options from
     * @param prefix the prefix to add to configuration options names when reading from @p grp
     */
    void storeDelayedLoadingData(const Konq::ViewType &type, const QString &serviceName, bool openUrl, const QUrl &url,
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

    /**
     * @brief The tab containing the view as a KonqFrameBase object
     *
     * Since the KonqFrameBase class hierarchy doesn't provide a concept of `tab`,
     * this return, among the frames directly or indirectly containing the view,
     * the one whose parent is a frame of tyep KonqFrameBase::Tabs.
     *
     * @return the parent frame representing the tab containing the view or `nullptr`
     * if no such frame exists (which should never happen)
     */
    KonqFrameBase* tab() const;

    /**
     * @brief A list of all the views linked to this one
     *
     * The views linked to this one are all the views in the same tab for which isLinkedView()
     * returns `true`, except for this one. If this view doesn't have the #m_bLinkedView
     * flag set, it means that it isn't linked to any other view, so an empty list is returned
     * @return the list of all views linked to this one or an empty list if this list isn't
     * linked to other views.
     */
    QList<KonqView*> linkedViews() const;

Q_SIGNALS:

    /**
     * @brief Signal emitted when the part associated with the view changed
     *
     * @note This signal is emitted *after* the part has been changed (but before
     * deleting the old one)
     *
     * @param childView the view itself
     * @param oldPart the part which was previously associated with the view
     * @param newPart the part which is now associated with the view
     */
    void sigPartChanged(KonqView *childView, KParts::ReadOnlyPart *oldPart, KParts::ReadOnlyPart *newPart);

    /**
     * @brief Signal emitted after the part has finished loading an URL
     * @param view the view itself
     */
    void viewCompleted(KonqView *view);

    /**
     * @brief Signal emitted when the URL shown in the view changes
     * @param url the new URL
     */
    void urlChanged(const QUrl &url);

    /**
     * @brief Signal emitted when the view's caption changes
     * @param caption the new caption
     */
    void captionChanged(const QString &caption);

public Q_SLOTS:
    /**
     * Store location-bar URL in the child view
     * and updates the main view if this view is the current one
     * May be different from url.
     */

    /**
     * @brief Changes the URL to display in the location bar
     *
     * This also updates the location bar in the main window and the tab icon if needed.
     *
     * @param locationBarURL the new URL to display in the location bar
     */
    void setLocationBarURL(const QString &locationBarURL);

    /**
     * @brief Sets the favicon for the current URL in KonqPixmapProvider
     *
     * This function does nothing if the user chose to disable favicons
     *
     * @param iconUrl the URL of the icon to use for the current URL
     */
    void setIconURL(const QUrl &iconURL);

    /**
     * @brief Changes the icon of the tab containing the view
     *
     * @param url the URL of the icon to use
     */
    void setTabIcon(const QUrl &url);

    /**
     * @brief Sets the caption associated with the view
     *
     * The caption is used in places like the tab bar.
     *
     * After the caption has been changed, the captionChanged() signal is emitted.
     *
     * @param caption the new caption to use. If this represents realUrl() and it's
     * different from requestedUrl(), requestedUrl() will be used instead
     */
    void setCaption(const QString &caption);

    /**
     * @brief Sets the security level associated with the page
     *
     * @param pageSecurity the security level associated with the page. It must be
     * one of the values in KonqMainWindow::PageSecurity
     */
    void setPageSecurity(int pageSecurity);

    /**
     * @brief Shows in the statusbar a message in response to a `KJob::infoMessage()` signal
     *
     * @param j the job (unused)
     * @param msg the message produced by the job
     */
    void slotInfoMessage(KJob *j, const QString &msg);

    /**
     * @brief Works around bugs with `QWebEngineView::setFocus()`
     *
     * These bugs (https://bugreports.qt.io/browse/QTBUG-122153
     * and https://qt-project.atlassian.net/browse/QTBUG-133649)
     * cause `QWebEngineView::setFocus()` not to actually give focus to the view.
     * The first bug was fixed in Qt 6.6.3 and the second in 6.8.3.
     *
     * This slot is connected to to WebEnginePart's `completed()` and `completedWithPendingAction()`
     * signals after starting loading an URL. When it's called, it calls `setFocus()` on
     * the widget, then disconnects itself. This works because calling `setFocus()` after the
     * `loadFinished()` signal has been emitted always works.
     *
     * @todo Remove this workaround when depending on a Qt version greater than 6.8.2
     */
    void forceWebEnginePartFocus();

private Q_SLOTS:

    /**
     * @brief Slot called when the part associated with the view starts loading data
     *
     * It makes the necessary connections with the signals emitted by the job used by the part
     *
     * @param job the job which the part is using to load data
     */
    void slotStarted(KIO::Job *job);

    /**
     * @brief Slot called when the part finished loading data
     *
     * It calls slotCompleted(bool) passing `false` as argument. It's used when
     * the part finishes loading data and emits the `KParts::ReadOnlyPart::completed()` signal
     */
    void slotCompleted();

    /**
     * @brief Overload of slotCompleted()
     *
     * It allows to specify whether or not the part has pending actions.
     *
     * @param bool hasPending whether there are pending actions. This should be `false`
     * if called in response to the `KParts::ReadOnlyPart::completed()` signal and
     * `true` if called in response to the `KParts::ReadOnlyPart::completedWithPendingAction()`
     */
    void slotCompleted(bool hasPending);

    /**
     * @brief Slot called when the part aborted loading
     *
     * @param errMsg the error message describing why loading was aborted or an empty
     * string if the user chose to abort loading
     */
    void slotCanceled(const QString &errMsg);

    /**
     * @brief Slot called in response to the `KJob::percentChanged()` signal
     *
     * It updates the statusbar to show the loading percentage.
     *
     * @param job unused
     * @param percent a percentage representing the progress of the loading job
     */
    void slotPercent(KJob *job, unsigned long percent);

    /**
     * @brief Slot called in response to the `KJob::speed()` signal
     *
     * It updates the statusbar with information on the speed of the job.
     * @param job unused
     * @param bytesPerSecond the speed of the job
     */
    void slotSpeed(KJob *job, unsigned long bytesPerSecond);

    //The four slots below are called in response to the several variants of the NavigationExtension::popupMenu signal.
    //There are four variants of this signal: two come from KParts::NavigationExtension and two from BrowserExtension,
    //which (as of KF6) has been moved to Konqueror. For each of them, there are two variants, one taking a KFileItemList
    //and one taing a QUrl.
    //
    //Since these slots need to be disconnected, we can't just use a lambda

    /**
     * @brief Slot called in response to the overload of BrowserExtension::browserPopupMenu() signal  which takes a `KFileItemList
     *
     * It displays a popup menu for the given files.
     *
     * @param global the position where the popup menu should be displayed
     * @param items the list items representing the files the popup applies to
     * @param args information about how to open the files
     * @param bargs Konqueror-specific information about how to open the files
     * @param flags actions to enable or disable in the popup menu
     * @param actionGroups named groups of actions which should be inserted into the popup
     *
     * @todo Unify with slotPopupMenuFiles()
     */
    void slotBrowserPopupMenuFiles(const QPoint &global,
        const KFileItemList &items, const KParts::OpenUrlArguments &args, const BrowserArguments &bargs,
        KParts::NavigationExtension::PopupFlags flags, const KParts::NavigationExtension::ActionGroupMap &actionGroups);

    /**
     * @brief Slot called in response to the overload of `KParts::NavigationExtension::popupMenu()` signal which takes a `KFileItemList`
     *
     * It works as browserPopupMenuFiles() except that it doesn't take the BrowserArguments parameter and
     * so it can be connected to `KParts::NavigationExtension::popupMenu()`. It should *not* be connected
     * to parts which provide a BrowserExtension.
     *
     * @param global the position where the popup menu should be displayed
     * @param items the list items representing the files the popup applies to
     * @param args information about how to open the files
     * @param flags actions to enable or disable in the popup menu
     * @param actionGroups named groups of actions which should be inserted into the popup
     */
    void slotPopupMenuFiles(const QPoint &global,
        const KFileItemList &items, const KParts::OpenUrlArguments &args, KParts::NavigationExtension::PopupFlags flags,
        const KParts::NavigationExtension::ActionGroupMap &actionGroups);

    /**
     * @brief Slot called in response to the overload of BrowserExtension::browserPopupMenu() signal which takes a `QUrl`
     *
     * It displays a popup menu for the given url.
     *
     * @param global the position where the popup menu should be displayed
     * @param url the list items representing the files the popup applies to
     * @param mode the file type of the URL (S_IFREG, S_IFDIR...)
     * @param args information about how to open the URL, in particular its mimetype
     * @param bargs Konqueror-specific information about how to open the URL
     * @param flags actions to enable or disable in the popup menu
     * @param actionGroups named groups of actions which should be inserted into the popup
     *
     * @todo Unify with slotPopupMenuUrl()
     */
    void slotBrowserPopupMenuUrl(const QPoint &global,
        const QUrl &url, mode_t mode, const KParts::OpenUrlArguments &arguments, const BrowserArguments &bargs,
        KParts::NavigationExtension::PopupFlags flags, const KParts::NavigationExtension::ActionGroupMap &actionGroups);

    /**
     * @brief Slot called in response to the overload of `KParts::NavigationExtension::popupMenu() signal which takes a `QUrl`
     *
     * It works as browserPopupMenuUrl() except that it doesn't take the BrowserArguments parameter and
     * so it can be connected to `KParts::NavigationExtension::popupMenu()`. It should *not* be connected
     * to parts which provide a BrowserExtension.
     *
     * @param global the position where the popup menu should be displayed
     * @param url the list items representing the files the popup applies to
     * @param mode the file type of the URL (S_IFREG, S_IFDIR...)
     * @param args information about how to open the URL, in particular its mimetype
     * @param flags actions to enable or disable in the popup menu
     * @param actionGroups named groups of actions which should be inserted into the popup
     *
     * @todo Unify with slotBrowserPopupMenuUrl()
     */
    void slotPopupMenuUrl(const QPoint &global,
        const QUrl &url, mode_t mode, const KParts::OpenUrlArguments &arguments, KParts::NavigationExtension::PopupFlags flags,
        const KParts::NavigationExtension::ActionGroupMap &actionGroups);

    /**
     * @brief Slot called in response to the part's `urlChanged()` signal
     *
     * It updates the view URL to match @p newUrl and emits the urlChanged() signal.
     *
     * When openUrl() is called for a file which has been transparently downloaded,
     * the part will emit the `urlChanged()` signal with the new URL of the downloaded
     * file. In this case, this function emits the urlChanged() signal passing the
     * requested URL instead of the downloaded one and doesn't change the view's URL
     * @param newUrl the new URL of the view
     */
    void updateUrl(const QUrl &newUrl);

    /**
     * Connected to the NavigationExtension
     */
    /**
     * @brief Slot connected to `KParts::NavigationExtension::selectionInfo()`
     *
     * It sends a KonqFileSelectionEvent corresponding to the selected items.
     *
     * @param items the items which have been selected
     */
    void slotSelectionInfo(const KFileItemList &items);

    /**
     * @brief Slot connected to `KParts::NavigationExtension::mouseOverInfo()`
     *
     * It sends a KonqFileMouseOverEvent event corresponding to the item the mouse
     * is over.
     *
     * @param item the item corresponding to the file the mouse is over
     */
    void slotMouseOverInfo(const KFileItem &item);

    /**
     * @brief Slot connected to `KParts::NavigationExtension::openURLNotify()`
     *
     * It updates history to include the current URL and, if needed, updates the
     * toolbar actions.
     */
    void slotOpenURLNotify();

    /**
     * @brief Slot connected to `KParts::NavigationExtension::enableAction()`
     *
     * It updates the status of the action in the main window.
     *
     * @param name the name of the action
     * @param enabled whether the action should be enabled or disabled
     */
    void slotEnableAction(const char *name, bool enabled);

    /**
     * @brief Slot connected to `KParts::NavigationExtension::setActionText()`
     *
     * It changes the text of the given action.
     *
     * @param name the name of the action whose text should be changed
     * @param text the new text of the action
     */
    void slotSetActionText(const char *name, const QString &text);

    /**
     * @brief Slot connected to `KParts::NavigationExtension::moveTopLevelWidget()`
     *
     * It moves the main window containing the view.
     *
     * @param x the x coordinate of the new window position
     * @param y the y coordinate of the new window position
     */
    void slotMoveTopLevelWidget(int x, int y);

    /**
     * @brief Slot connected to `KParts::NavigationExtension::resizeTopLevelWidget()`
     *
     * It resizes the window, but only if there's just one tab, to avoid possible conflicts
     * between different resizing requests.
     *
     * @param w the requested new width for the window
     * @param h the requested new height for the window
     *
     * @todo Maybe, in case of multiple tabs (or multiple views?) resize the window
     * only if it's smaller than the requested size
     */
    void slotResizeTopLevelWidget(int w, int h);

    /**
     * @brief Slot connected to `KParts::NavigationExtension::requestFocus()`
     *
     * It activates the tab containing the view.
     *
     * @param part unused. The part which emitted the signal
     */
    void slotRequestFocus(KParts::ReadOnlyPart *part);

private:
    /**
     * @brief Replace the current part with a new part, created by @p viewFactory.
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
     * @brief Makes the necessary connections to the current part's signals
     *
     * This method should be called after a new part has been created.
     */
    void connectPart();

    /**
     * @brief Creates a new entry in the history
     *
     * It truncates any forward history, then adds a new, empty, entry and marks
     * it as the current one.
     */
    void createHistoryEntry();

    /**
     * @brief Adds the given history entry to history
     *
     * The added entry becomes the most recent entry in history.
     *
     * If history would become longer than allowed, the oldest entries are deleted.
     *
     * @param historyEntry the entry to append
     */
    void appendHistoryEntry(HistoryEntry *historyEntry);

    /**
     * @brief Updates the current entry in the history with the current information from the view
     *
     * Some information, including the part state as set by `KParts::NavigationExtension::saveState()`
     * (if the part provides it) are only updated if @p needsReload is `false`.
     *
     * @param needsReload `true` if the part needs to finish loading and `false` if it's fully loaded.
     * If `true`, some information aren't updated because they're not ready yet
     */
    void updateHistoryEntry(bool needsReload);

    /**
     * @brief Performs the necessary operations before opening an URL
     *
     * It updates #m_bErrorURL and sends a `KParts::OepnUrlEvent` for the URL which
     * is being opened.
     *
     * @param url the URL which will be opened
     * @param args information about how the URL will be opened
     */
    void aboutToOpenURL(const QUrl &url, const KParts::OpenUrlArguments &args = KParts::OpenUrlArguments());

    /**
     * @brief Updates the part mimetype so that it matches #m_type
     *
     * If #m_type corresponds to a part capability, an empty mimetype is used.
     */
    void setPartMimeType();

    /**
     * @brief Deletes the current URL if it was a temporary one
     *
     * It does nothing if the URL isn't a temporary one (if #m_tempFile is empty)
     */
    void finishedWithCurrentURL();

    /**
     * @brief Override of `QObject::eventFilter()`
     *
     * It handles drag and drop events:
     * - in case of a drag enter event, it accepts the event if it contains at least
     * one URL, its scheme is not `javascript` and the event didn't originate in the
     * part's widget or its children
     * - in case of a drop event, it emits `KParts::NavigationExtension::openUrlRequest()`
     * for the first URL if the event has at least one URL.
     *
     * @param obj the object the event was sent to
     * @param e the event
     * @return `false`, meaning that processing the event should go on
     */
    bool eventFilter(QObject *obj, QEvent *e) override;

    /**
     * @brief Casts the part associated with the view to a Konq::PlaceholderPart
     * @return the part associated with the view cast to a Konq::PlaceholderPart or `nullptr` if it's not a Konq::PlaceholderPart
     */
    Konq::PlaceholderPart *placeholderPart() const;

////////////////// private members ///////////////

    KParts::ReadOnlyPart *m_pPart; //!< The part associated with the view, if any

    /**
     * @brief A version of the URL in the view suitable to be shown to the user
     *
     * This is the string shown in the location bar, but it could also be used elsewhere.
     *
     * In most cases, it corresponds `url().toDisplayString()`, but sometimes it
     * doesn't, for example if viewing a directory with an `index.html` file and
     * the setting to automatically show the contents of `index.html` is on.
     */
    QString m_sLocationBarURL;
    QString m_sTypedURL; //!< The URL the user entered in the location bar to load url()
    KonqMainWindow::PageSecurity m_pageSecurity; //!< The security level of the currently shown page

    /**
     * @brief A list of selected items in the current part
     *
     * It only makes sense for certain parts (currently, only Dolphin part) and it's
     * always empty when it doesn't make sense
     */
    KFileItemList m_selectedItems;

    /**
     * @brief The full history
     *
     * The history is a list made of:
     * - back history
     * - current entry (corresponding to #m_lstHistoryIndex)
     * - forward history
     */
    QList<HistoryEntry *> m_lstHistory;
    int m_lstHistoryIndex; //!< The current position in the history

    /**
     * @brief The POST data that resulted the page currently shown
     *
     * This used to be needed to restore the page and in case of a reload. With
     * QtWebEngine, there's no way to obtain this data and in any case, it shouldn't
     * be needed anymore.
     *
     * @todo Determine whether this is still needed and remove it if it isn't
     */
    QByteArray m_postData;
    /**
     * @brief The mimetype of the POST data
     *
     * With `QtWebEngine`, this isn't used or needed anymore
     *
     * @todo Determine whether this is still needed and remove it if it isn't
     */
    QString m_postContentType;
    /**
     * @brief Whether this page was the result of a POST request
     *
     * With `QtWebEngine` this information isn't available anymore
     *
     * @todo Determine whether this is still needed and remove it if it isn't
     */
    bool m_doPost;

    /**
     * @brief The referrer that was used to obtain this page
     *
     * @todo See whether this is still useful with `QtWebEngine` and remove it
     * if it isn't
     */
    QString m_pageReferrer;

    KonqMainWindow *m_pMainWindow; //!< The main window the view is associated with
    UrlLoader *m_loader = nullptr; //!< The UrlLoader used to load the current URL
    KonqFrame *m_pKonqFrame; //!< The frame where the part widget is

    uint m_bLoading: 1; //!< Whether the part is loading an URL
    uint m_bLockedLocation: 1; //!< Whether the view should remain in the current URL
    uint m_bPassiveMode: 1; //!< Whether the view is in passive mode
    uint m_bLinkedView: 1; //!< Whether the view is linked to other views
    uint m_bToggleView: 1; //!< Whether the view is a toggle view
    uint m_bLockHistory: 1; //!< Whether history can't be changed
    uint m_bAborted: 1; //!< Whether the last attempt to open and URL failed
    uint m_bGotIconURL: 1; //!< Whether the icon for the current URL has already been set
    uint m_bPopupMenuEnabled: 1; //!< Whether popup menus are enabled for the current part
    uint m_bFollowActive: 1; //!< Whether this view should always attempt to show the same URL as the active view
    uint m_bPendingRedirection: 1; //!< Whether there's a pending action
    uint m_bBuiltinView: 1; //!< Whether the part associated with the view is a built in Konqueror
    /**
     * @brief Whether drop should be handled for the part
     *
     * Handling is turned on if the part doesn't provide a `KParts::NavigationExtension` or
     * if that extension as a `urlDropHandling` property with value `true`.
     */
    uint m_bURLDropHandling: 1;
    uint m_bDisableScrolling: 1; //!< Whether scrolling in the part should be disabled
    uint m_bErrorURL: 1; //!< Whether the part is displaying an error page
    QVector<KPluginMetaData> m_partServiceOffers; //!< The plugins available to display the mimetype for the current URL
    KService::List m_appServiceOffers; //!< The applications available to open the mimetype for the current URL
    KPluginMetaData m_service; //!< The metadata of the plugin providing the current part
    /**
     * @brief The type of the part associated with the view
     *
     * This can represent either the mimetype shown in the view or the `KParts::BrowserView` capability
     */
    Konq::ViewType m_type;
    QString m_caption; //!< The caption for the view
    QString m_tempFile; //!< Whether the URL shown in the view is temporary and must be deleted when navigating away from it
    QString m_dbusObjectPath; //!< The DBus path representing the view

    /**
     * @brief Struct which contains the real URL shown in the view's part and the URL requested by the user
     *
     * In many cases, the URL requested by the user will be the one shown in the part: in this case, ViewUrl::requested
     * will be `std::nullopt`. When a remote file should be shown in a part which isn't the browser part
     * (i.e. WebEnginePart), it needs to be downloaded, but this should remain transparent to the user. In this
     * case, ViewUrl::real will contain the URL of the downloaded file, but ViewUrl::requested will be the
     * URL of the remote file.
     *
     * In most cases, url() should be used to decide whether #real or #requested should be used. Only in very
     * specific circumstances you should explicitly use #real or #requested.
     */
    struct ViewUrl {
        QUrl real; //!< The URL which is displayed in the part
        std::optional<QUrl> requested; //!< The URL requested by the user

        /**
         * @brief The logical URL shown in the view
         *
         * @return `requested.value()` if #requested has a value and #real otherwise
         */
        QUrl url() const {return requested ? *requested : real;}
    };

    /**
     * @brief The URL shown in the view
     *
     * Using `m_url.real` is the same as using `m_pPart->url()`. The reason it needs
     * to be stored is to be able to compare it with the argument passed to updateUrl().
     */
    ViewUrl m_url;

    bool m_hasBrowserExtension = false; //!< Whether the associated part has a BrowserExtension or not
};

#endif
