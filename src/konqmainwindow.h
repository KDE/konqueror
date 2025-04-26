/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 2000-2004 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2007 Eduardo Robles Elvira <edulix@gmail.com>
    SPDX-FileCopyrightText: 2007 Daniel García Moreno <danigm@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQMAINWINDOW_H
#define KONQMAINWINDOW_H

#include "konqprivate_export.h"

#include <QMap>
#include <QPointer>
#include <QList>
#include <QUrl>
#include <QUuid>

#include <kfileitem.h>
#include <kparts/mainwindow.h>
#include <KParts/PartActivateEvent>
#include <kservice.h>
#include <KParts/NavigationExtension>

#include "konqcombo.h"
#include "konqframe.h"
#include "konqframecontainer.h"
#include "konqopenurlrequest.h"
#include "configdialog.h"
#include "windowargs.h"

#include <KConfigGroup>

class QActionGroup;
class KUrlCompletion;
class QLabel;
class KLocalizedString;
class KToggleFullScreenAction;
class KonqUndoManager;
class QAction;
class QAction;
class KActionCollection;
class KActionMenu;
class KBookmarkGroup;
class KBookmarkMenu;
class KBookmarkActionMenu;
class KCMultiDialog;
class KNewFileMenu;
class KToggleAction;
class KBookmarkBar;
class KonqView;
class KonqFrameContainerBase;
class KonqFrameContainer;
class KToolBarPopupAction;
class KonqAnimatedLogo;
class KonqViewManager;
class ToggleViewGUIClient;
class KonqRun;
class KConfigGroup;
class KonqHistoryDialog;
struct HistoryEntry;
class QLineEdit;
class UrlLoader;
class FullScreenManager;

namespace KParts
{
//TODO KF6: when removing compatibility with KF5, uncomment the line below
// class NavigationExtension;
class ReadOnlyPart;
class OpenUrlArguments;
}

namespace Konq {
    class ViewType;
}

namespace KonqImplementations {
    class KonqWindow;
};

class KonqExtendedBookmarkOwner;

/**
 * @brief Class implementing each of Konqueror main windows
 *
 */
class KONQ_TESTS_EXPORT KonqMainWindow : public KParts::MainWindow, public KonqFrameContainerBase
{
    Q_OBJECT
    Q_PROPERTY(int viewCount READ viewCount)
    Q_PROPERTY(int linkableViewsCount READ linkableViewsCount)
    Q_PROPERTY(QString locationBarURL READ locationBarURL)
    Q_PROPERTY(bool fullScreenMode READ fullScreenMode)
    Q_PROPERTY(QString currentTitle READ currentTitle)
    Q_PROPERTY(QString currentURL READ currentURL)

    /**
     * @brief Type used as argument for the internal constructor.
     *
     * @see KonqMainWindow(const ConstructorArg&);
     */
    using ConstructorArg = std::variant<QUrl, bool>;

    /**
     * @brief Internal constructor which can be used to create both a regular and a preloaded window
     *
     * This constructor is called by both KonqMainWindow(QUrl) and by createPreloaded(). Depending
     * on the value of @p arg, it creates either a regular window or a preloaded window.
     *
     * @param arg information about the window to create. If it contains a `QUrl`, it creates a regular
     * window with that URL. If it contains a `bool` and that is `true`, it creates a preloaded window;
     * if the `bool` is `false`, it creates a regular window using the starting URL configured by the user.
     */
    explicit KonqMainWindow(const ConstructorArg &arg);

public:

    /**
     * @brief Enum describing actions performed in the location bar of another window
     * which need to be propagated in other windows
     */
    enum ComboAction {
        ComboClear, //!< The location bar history has been cleared
        ComboAdd, //!< A new item has been added to the location bar history
        ComboRemove //!< An item has been removed from the location bar history
    };

    /**
     * @brief Enum describing the security status of the current page
     */
    enum PageSecurity {
        NotCrypted, //!< The page is not encrypted
        Encrypted,  //!< The page is encrypted
        Mixed //!< The page is partially encrypted
    };

    /**
     * @brief Constructor
     *
     * It creates a new main window with a view displaying the given URL, if any.
     * If @p initialUrl is empty, no view will be created and everything inside
     * the main window will be disabled, including the location bar and most of
     * the menu entries. This means that the user won't be able to do anything
     * with Konqueror, so only pass an empty @p initialURL if you later call
     * openUrl() or a similar function to actually open the URL.
     *
     * @param initialURL the URL to display initially. If empty, no view will be
     * created
     */
    explicit KonqMainWindow(const QUrl &initialURL = QUrl());

    /**
     * @brief Destructor
     *
     * It deletes several member variables and removes the window from #s_lstMainWindows
     * then, if this is the last window in that list, it also deletes the list.
     * If the list only contains another window and that window is a preloaded window,
     * it closes that to avoid leaving the application running but with no visible
     * window.
     */
    ~KonqMainWindow() override;

    /**
     * @brief Creates a preloaded window
     *
     * @return the new preloaded window
     */
    static KonqMainWindow* createPreloaded();

    /**
     * @brief Finds the most suitable window to open an URL
     *
     * This is used when another application asks Konqueror to open an URL to
     * avoid, for example, using a window which is in another activity or on
     * another desktop.
     *
     * @return the active window or the last deactivated window in the current
     * activity and desktop. If there is no suitable window in the current activity and
     * desktop, `nullptr` is returned
     */
    static KonqMainWindow* findMostSuitableWindow();

    /**
     * @brief Attempts to transform the given string in an URL and opens it
     *
     * It uses KonqMisc::konqFilteredURL() to attempt to transform @p url into a
     * real URL. If it succeeds (or if @p url already was a real URL), it opens
     * it, otherwise nothing is done.
     *
     * This function is usually called when attempting to open an URL from data
     * entered by the user, in particular in the location bar.
     *
     * @param url the text to transform in an URL which will be then opened
     * @param req the object containing information about how the URL should be
     * opened
     */
    void openFilteredUrl(const QString &url, const KonqOpenURLRequest &req);

    /**
     * @brief Overload of openFilteredUrl(const QString &, const KonqOpenURLRequest &)
     *
     * It works as openFilteredUrl(const QString &, const KonqOpenURLRequest &).
     *
     * @param url the text to transform in an URL which will be then opened
     * @param inNewTab `true` to open the URL in a new tab instead of a new window
     * and `false` otherwise
     * @param tempFile whether to remove the URL once the part displaying it has
     * been closed
     */
    void openFilteredUrl(const QString &url, bool inNewTab = false, bool tempFile = false);

    /**
     * @brief Overload of openFilteredUrl(const QString &, const KonqOpenURLRequest &)
     *
     * It works as openFilteredUrl(const QString &, const KonqOpenURLRequest &).
     *
     * @param _url the text to transform in an URL which will be then opened
     * @param _mimeType the mimetype of the URL to open or an empty string if it's
     * unknown
     * @param inNewTab `true` to open the URL in a new tab instead of a new window
     * and `false` otherwise
     * @param tempFile whether to remove the URL once the part displaying it has
     * been closed
     */
    void openFilteredUrl(const QString &_url, const QString &_mimeType, bool inNewTab, bool tempFile);

    /**
     * @brief Override of `KParts::MainWindow::applyMainWindowSettings()`
     *
     * It restores the statusbar (since in Konqueror the statusbar is not a child
     * of the main window, `KParts::MainWindow::applyMainWindowSettings()` won't
     * restore it) and moves the window to the correct activities.
     *
     * @see ActivityManager
     */
    void applyMainWindowSettings(const KConfigGroup &config) override;

    /**
     * @brief As `KMainWindow::saveMainWindowSettings()`
     *
     * It does the same as `KMainWindow::saveMainWindowSettings()` and also saves
     * the visibility of the statusbar.
     *
     * If the window is preloaded, it does nothing since the user can't see a
     * preloaded window, which means that it would have default settings which
     * shouldn't override those set by the user.
     *
     * @internal
     * It's not override since KMainWindow variant isn't virtual
     * @endinternal
     */
    void saveMainWindowSettings(KConfigGroup &config);

public Q_SLOTS:
    /**
    * @brief Opens the given URL
    *
    * The URL can be opened in @p view or in a view in a new tab, depending on the
    * user setting. Depending on @p req, the URL can be opened, embedded or saved.
    *
    * Depending on whether the mimetype is known or not and of whether the URL points
    * to a local file or not, opening the URL can start immediately or asynchronously:
    * if the mimetype is known or the URL is local, it starts immediately. Otherwise,
    * a `KIO::MimeTypeFinderJob` is use to determine the mimetype, which means that
    * opening the URL will only start after it has finished and so it will start
    * asynchronously. Opening the URL itself is, of course, always asynchronous.
    *
    * If the URL in invalid or has an unsupported scheme, an error page will be
    * displayed instead.
    *
    * This function takes care of setting the modified status of the combo box to
    * `false`, starting the animation and changing the content of the location bar
    * if needed.
    *
    * Calling this function on a preloaded window outside the constructor marks the
    * window as regular window so that it's not considered preloaded anymore.
    *
    * @param view the view which requested to load the URL. It can be `nullptr`
    * @param url the URL to open
    * @param serviceType the mimetype of the URL or the service to use. An empty
    * string means that the mimetype is unknown and should be determined automatically
    * @param req the object containing information about how to open the URL
    * @param trustedSource in case the URL points to an executable, whether it's secure to run it.
    * Usually, this will be `true` for local files and `false` for remote URLs
    */
    void openUrl(KonqView *view, const QUrl &url,
                 const QString &serviceType = QString(),
                 const KonqOpenURLRequest &req = KonqOpenURLRequest::null,
                 bool trustedSource = false); // trustedSource should be part of KonqOpenURLRequest, probably

public:
    /**
     * @brief Opens the given URL in the appropriate view, creating it if needed
     *
     * This function also checks whether the view is being followed, whether it's
     * following the active view or it's locked and does what is necessary to ensure
     * these properties are respected (for example, making following views also open
     * @p url).
     *
     * @param type the type of the URL to open. Always set.
     * @param url the URL to open.
     * @param childView the view in which to open the URL. Can be `nullptr`, in which
     * case a new tab (or the very first view) will be created.
     * @param req the request describing how the URL should be opened
     * @param requestedUrl the URL which the user requested. An empty URL means that
     * the user requested @p URL. If it's not empty, it means that, while the user
     * requested this URL, the actual URL to open is @p url. This usually happens
     * if @p requestedUrl needed to be downloaded to a temporary file before being
     * opened
     */
    bool openView(Konq::ViewType type, const QUrl &url, KonqView *childView,
                  const KonqOpenURLRequest &req = KonqOpenURLRequest::null, QUrl requestedUrl={});

    /**
     * @brief Halts loading the URL in the current view
     */
    void abortLoading();

    /**
     * @brief Opens multiple URLs
     *
     * Each URL is opened in a new tab.
     *
     * @param url the URLs to open
     */
    void openMultiURL(const QList<QUrl> &url);

    /**
     * @brief Returns the view manager for this window
     * @return the view manager for this window
     */
    KonqViewManager *viewManager() const
    {
        return m_pViewManager;
    }

    /**
     * @brief Override of `KXMLGUIBuilder::createContainer()`
     *
     * It works as the base class version and also takes care of delayed initialization
     * of the bookmarks toolbar and of fixing the accelerators for the `edit`
     * and `tools` menu.
     *
     * @param parent the parent for the container
     * @param index the index where the container should be inserted into the parent container/widget
     * @param element the element from the DOM tree describing the container
     * @param containerAction the action created for this container
     */
    QWidget *createContainer(QWidget *parent, int index, const QDomElement &element, QAction *&containerAction) override;

    /**
     * @brief Override of `KXMLGUIBuilder::removeContainer()`
     *
     * It works as the base class version and also clears the bookmarks toolbar
     */
    void removeContainer(QWidget *container, QWidget *parent, QDomElement &element, QAction *containerAction) override;

    /// KMainWindow methods, for session management
    /**
     * @brief Override of `KMainWindow::saveProperties()`
     *
     * It saves instance specific properties for session management, in particular the
     * views configuration, the window uuid and the activities the window belongs to.
     *
     * @param config the configuration group where properties should be saved
     */
    void saveProperties(KConfigGroup &config) override;

    /**
     * @brief Override of `KMainWindow::readProperties()`
     *
     * It reads the properties saved with saveProperties() and applies the saved
     * main window settings.
     *
     * @param config the configuration group  properties should be read from
     */
    void readProperties(const KConfigGroup &config) override;

    /**
     * @brief Sets the name of the initial frame
     * @param name the name of the initial frame
     */
    void setInitialFrameName(const QString &name);

    /**
     * @brief Applies the user settings after they have changed
     */
    void reparseConfiguration();

    /// Called by KonqViewManager
    /**
     * @brief Function called when a view has been added to the view manager
     *
     * It inserts the view in #m_mapViews, makes the necessary signal-slot connections
     * and emits the viewAdded() signal.
     *
     * This is called by KonqViewManager::setupView().
     *
     * @param childView the new view
     */
    void insertChildView(KonqView *childView);
    /// Called by KonqViewManager

    /**
     * @brief Function called when a view has been removed from the view manager
     *
     * It disconnects the view from the main window, removes it from #m_mapViews
     * and emits the viewRemoved() signal.
     *
     * @param childView the new view
     */
    void removeChildView(KonqView *childView);

    /**
     * @brief The view associated with the given part
     *
     * @param view the part
     * @return the view associated with @p view
     */
    KonqView *childView(KParts::ReadOnlyPart *view);

    /**
     * @brief The view with the given name
     *
     * If more than one views have the name @p name, the one corresponding to the
     * part @p callingPart is returned.
     *
     * @param name the name of the view to return
     * @param callingPart the part associated to the view to return in case there
     * are multiple parts with name @p name
     * @param [out] part if not `nullptr`, it will be set to point to the part
     * associated with the returned view
     * @return the view with name @p name. If multiple views have that name and
     * one of them is associated with @p callingPart, that view is returned,
     * otherwise another one is returned. If no view has name @p name, `nullptr`
     * is returned
     */
    KonqView *childView(KParts::ReadOnlyPart *callingPart, const QString &name, KParts::ReadOnlyPart **part);

    /**
     * @brief The total number of views
     *
     * @return the total number of views
     */
    int viewCount() const
    {
        return m_mapViews.count();
    }

    /**
     * @brief The number of views which aren't in passive mode and aren't locked
     * @return the number of views which aren't in passive mode and aren't locked
     */
    int activeViewsNotLockedCount() const;

    /** @brief The number of views that can be linked
     *
     * These are all the views in the current tab which aren't following the
     * active view.
     *
     * @return The number of views that can be linked
     */
    int linkableViewsCount() const;

    /**
     * @brief The number of non-toggle, non-passive views
     * @return the number of non-toggle, non-passive views
     */
    int mainViewsCount() const;

    /**
     * @brief A map associating each part to the corresponding view
     */
    typedef QMap<KParts::ReadOnlyPart *, KonqView *> MapViews;

    /**
     * @brief A map associating each part to the corresponding view
     * @return a map associating each part to the corresponding view
     */
    const MapViews &viewMap() const
    {
        return m_mapViews;
    }

    /**
     * @brief The current view
     * @return the current view
     */
    KonqView *currentView() const;

    /** URL of current part, or URLs of selected items for directory views */
    /**
     * @brief The list of selected items in the current view or the URL of the
     * current view
     *
     * @return the list of selected items in the current view if the current view
     * provides such list and a list containing only the URL of the current view
     * otherwise
     */
    QList<QUrl> currentURLs() const;

    /**
     * @brief If there are only two views in the window, returns that which is not the given one
     *
     * If there are more than two windows, the behavior is undefined.
     *
     * @warning This function only works if there are exactly two views in the
     * window, _not_ in the current tab.
     *
     * @param view one of the two views
     * @return the view which is not @p view or `nullptr` if there's only one view.
     * If there are more than two views, the behavior is undefined
     */
    KonqView *otherView(KonqView *view) const;

    /**
     * @brief Override of KParts::MainWindow::setCaption(const QString &)
     *
     * It uses a squeezed version of the caption. The full caption is stored inside
     * the current view and can be retrieved using KonqView::caption().
     *
     * @param caption the new caption
     */
    void setCaption(const QString &caption) override;

    /**
     * @brief Override of KParts::MainWindow::setCaption(const QString &, bool)
     *
     * It works as setCaption(const QString &). The @p modified parameter is never
     * used.
     *
     * @param caption the new caption
     * @param modified whether the document is modified. Unused
     */
    void setCaption(const QString &caption, bool modified) override
    {
        Q_UNUSED(modified);
        setCaption(caption);
    }

    /**
     * @brief Changes the URL displayed in the location bar
     *
     * This also updates the window icon to match the new URL. It does nothing if
     * the user changed the URL in the location bar after the last call to openUrl()
     *
     * @param url the new URL to display in the location bar
     */
    void setLocationBarURL(const QString &url);

    /**
    * @brief Overload of setLocationBarURL(const QString &url);
    *
    * @param url the new URL to display in the location bar
    */
    void setLocationBarURL(const QUrl &url);

    /**
    * @brief The URL displayed in the location bar
    * @return the URL displayed in the location bar
    */
    QString locationBarURL() const;

    /**
     * @brief Gives focus to the location bar
     *
     * If the location bar doesn't exist or isn't visible, it does nothing.
     */
    void focusLocationBar();

    /**
     * @brief Sets the page security icon corresponding to the current view
     *
     * @param sec the new page security status
     */
    void setPageSecurity(PageSecurity sec);

    /**
     * @brief Sets up the initial enabled or disabled status of actions
     *
     * If @p enable is `false`, all actions are disabled. If @p enable is `true`,
     * it enables all the actions except those for which it doesn't make sense
     * (for example, the `back` action if the history is empty).
     *
     * This function expects to be called with `true` exactly one time, when setting
     * up the first view for the window: the status of some action is chosen
     * according to this assumption.
     *
     * @param enable `true` to enable all actions for which it makes sense and
     * `false` to disable all actions
     */
    void enableAllActions(bool enable);

    /**
     * @brief Updates the enabled status for actions knowing that there aren't any
     * views
     *
     * It disables all the actions which don't make sense without a view and enables
     * the others.
     */
    void disableActionsNoView();

    /**
     * @brief Enables or disables toolbar actions depending on whether they make
     * sense given the current window status
     *
     * It does nothing if there isn't an active view.
     *
     * @param pendingActions whether there are actions which can be interrupted. It's
     * used to decide whether the "stop" action needs to be enabled or not
     */
    void updateToolBarActions(bool pendingActions = false);

    /**
     * @brief Updates the "Open with" menu and the "Open with ..." actions
     *
     * @note This will delete the old menu (#m_openWithMenu) and the "open with..."
     * actions (#m_openWithActions).
     */
    void updateOpenWithActions();

    /**
     * @brief Updates the enabled status of actions which depend on the current
     * view
     *
     * @warning Don't make changes which depend on the @link KonqView::url() url()@endlink
     * of the current view, as this function may be called before that url is
     * updated (for example when going back in history). For those changes, use
     * updateToolBarActions() instead
     */
    void updateViewActions();

    /**
     * @brief Whether the sidebar is visible or not
     * @return `true` if the sidebar is visible and `false` otherwise
     */
    bool sidebarVisible() const;

    /**
     * @brief Whether the window is in full screen mode
     * @return `true` if the window is in full screen mode and `false` otherwise
     */
    bool fullScreenMode() const;

    /**
     * @brief Creates a new tab with a view to open the given URL in
     *
     * This just creates the view in a position depending on
     * @link KonqOpenURLRequest::openAfterCurrentPage request.openAfterCurrentPage@endlink
     * sets its caption and moves it in the foreground if neede according to
     * @link KonqOpenURLRequest::newTabInFront request.newTabInFront@endlink. It
     * doesn't open the URL.
     *
     * @param url the URL
     * @param request information about how the URL should be opened
     * @return the view in the new tab or `nullptr` if such view couldn't be created
     */
    KonqView* createTabForLoadUrlRequest(const QUrl &url, const KonqOpenURLRequest &request);

    /**
     * @brief The action to toggle the "linked" status for a view
     * @return the action to toggle the "linked" status for a view
    */
    KToggleAction *linkViewAction()const
    {
        return m_paLinkView;
    }

    /**
     * @brief Enables or disables the given action
     *
     * If an action with name @p name doesn't exist, nothing is done
     * @param name the name of the action
     * @param enabled `true` if the action should be enabled and `false` if it should
     * be disabled
     */

    void enableAction(const char *name, bool enabled);
    /**
     * @brief Changes the text of the given action
     *
     * If an action with name @p name doesn't exist, nothing is done
     * @param name the name of the action
     * @param text the new text of the action
     */
    void setActionText(const char *name, const QString &text);

    /**
     * @brief A list of all existing `KonqMainWindow`
     *
     * @return a pointer to a list of all main windows or `nullptr` if no main
     * window exists.
     * @warning Unless you are sure that at least one main window exists, you should
     * check that this is not `nullptr` before using it.
     */
    static QList<KonqMainWindow *> *mainWindowList()
    {
        return s_lstMainWindows;
    }

    /**
     * @brief A list of all existing `KonqMainWindow`
     *
     * @return A list of all `KonqMainWindow`. This list can be empty
     * @note This function is similar to mainWindowList(). The only differences is that it doesn't return a pointer,
     * so there's no need to check for `nullptr`, and that the list can't be modified.
     * @internal The returned list is `const` to avoid the fact that modifying it would affect ::s_lstMainWindows, but
     * only if the latter isn't `nullptr`, which could be confusing for the user.
     * @todo Check whether it's possible to make `s_lstMainWindows` not be a pointer: in this case, this function can be
     * removed and mainWindowList() can be used in its place.
     */
    static QList<KonqMainWindow*> const mainWindows() {return s_lstMainWindows ? *s_lstMainWindows : QList<KonqMainWindow*>{};}

    /**
     * @brief Updates the elements related to the number of linkable views
     *
     * It changes the state of the "Link View" action depending on whether there
     * are more than one view in the current tab and unlinks the current view if
     * it's the only view in the tab.
     */
    void linkableViewCountChanged();

    /**
     * @brief Updates the elements related to the number of views
     *
     * It does the same as linkableViewCountChanged() and also updates the view-related
     * actions.
     */
    void viewCountChanged();

    // operates on all combos of all mainwindows of this instance
    // up to now adds an entry or clears all entries
    /**
     * @brief Performs an action on the combo box of all main windows
     *
     * This is a function called by KonqApplication in response to signals emitted
     * by KonqCombo via DBus.
     *
     * The possible actions are those described in ComboAction.
     *
     * The main window which originally emitted the DBus signal also saves the
     * combo history.
     * @param action the action to be performed. It must be one of the values in
     * ComboAction
     * @param url the url associated with the action. If @p action is ComboClear,
     * this is unused
     * @param senderId the identifier of the application which originally sent
     * the message
     */
    static void comboAction(int action, const QString &url,
                            const QString &senderId);

#ifndef NDEBUG
    /**
     * @brief Prints on standard error all the views in the window
     */
    void dumpViewList();
#endif

    // KonqFrameContainerBase implementation BEGIN

    /**
     * @brief Implementation of KonqFrameBase::accept()
     *
     * It calls the corresponding method of #m_pChildFrame.
     *
     * @return `true` if no error occurred and `false` otherwise
     */
    bool accept(KonqFrameVisitor *visitor) override;

    /**
     * @brief Implementation of KonqFrameContainerBase::insertChildFrame()
     *
     * It sets @p frame as the only child frame, replacing the previous child frame
     * if it existed.
     * @note The previous child frame isn't deleted
     * @param frame the new child frame
     * @param index unused
     */
    void insertChildFrame(KonqFrameBase *frame, int index = -1) override;

    /**
     * @brief Implementation of KonqFrameContainerBase::childFrameRemoved()
     *
     * It sets pointers associated with the child frame to `nullptr`.
     * @warning Remember to call this function before deleting child frames.
     *
     * @param frame unused
     */
    void childFrameRemoved(KonqFrameBase *frame) override;

    /**
     * @brief Implementation of KonaFrameBase::saveConfig()
     *
     * It calls the @link KonqFrameBase::saveConfig() saveConfig()@endlink method
     * of the child frame.
     *
     * @param config the group where to write the information
     * @param prefix a string to add to each key to make it unique in the group
     * @param options which information to include in the config group
     * @param docContainer the doc container
     * @param id an identifier to use
     * @param depth the level inside the frame hierarchy
     */
    void saveConfig(KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options, KonqFrameBase *docContainer, int id = 0, int depth = 0) override;

    /**
     * @brief Implementation of KonqFrameBase::copyHistory()
     *
     * It calls the @link KonqFrameBase::copyHistory() copyHistory()@endlink method
     * of the child frame.
     *
     * @param other the frame to copy history information from
     */
    void copyHistory(KonqFrameBase *other) override;

    /**
     * @brief Implementation of KonqFrameBase::setTitle()
     *
     * It does nothing.
     *
     * @param title unused
     * @param sender unused
     */
    void setTitle(const QString &title, QWidget *sender) override;

    /**
     * @brief Implementation of KonqFrameBase::setTabIcon()
     *
     * It does nothing.
     *
     * @param url unused
     * @param sender unused
     */
    void setTabIcon(const QUrl &url, QWidget *sender) override;

    /**
     * @brief Implementation of KonqFrameBase::asQWidget()
     *
     * @return this object
     */
    QWidget *asQWidget() override;

    /**
     * @brief Implementation of KonqFrameBase::frameType()
     *
     * @return KonqFrameBase::MainWindow
     */
    KonqFrameBase::FrameType frameType() const override;

    /**
     * @brief The only child frame
     *
     * @return the only child frame (#m_pChildFrame)
     */
    KonqFrameBase *childFrame() const;

    /**
     * @brief Implementation of KonqFrameContainerBase::setActiveChild()
     *
     * It does nothing
     *
     * @param activeChild unused
     */
    void setActiveChild(KonqFrameBase *activeChild) override;

    // KonqFrameContainerBase implementation END

    /**
     * @brief Stores the index of the tab where a context menu is being shown
     *
     * This is necessary when displaying a context menu.
     *
     * @param index the index of the tab
     */
    void setWorkingTab(int index);

    /**
     * @brief Whether the given application is Konqueror
     *
     * This is used to avoid attempting to open an URL in an external Konqueror
     * instance, because that would lead to an endless loop.
     *
     * @param mimeType unused
     * @param offer the application to test
     * @return `true` if @p offer is actually `Konqueror` or `kfmclient` and `false`
     * if it's another application
     */
    static bool isMimeTypeAssociatedWithSelf(const QString &mimeType, const KService::Ptr &offer);

    /**
     * @brief Whether, according to the user preferences, the application to open
     * the given mimetype is Konqueror
     *
     * This is used to avoid attempting to open an URL with mimetype @p mimeType
     * in an external Konqueror instance, because that would lead to an endless loop.
     *
     * @param mimeType the mimetype to test
     * @return `true` if, according to the user preferences, the application to
     * open an URL with mimetype @p mimeType is `Konqueror` or `kfmclient` and
     * `false` if it's another application
     */
    static bool isMimeTypeAssociatedWithSelf(const QString &mimeType);

    /**
     * @brief The title of the main window
     * @return the title of the main window
     */
    QString currentTitle() const;

    /**
     * @brief The URL of the current view
     * @note This is currently unused by Konqueror itself but can be used by
     * plugins and scripts
     * @return the URL of the current view
     */
    QString currentURL() const;

    /**
     * @brief Updates history related actions
     *
     * It toggles the "Back" and "Forward" action depending on whether in the
     * current view history there's a previous and a next entry.
     */
    void updateHistoryActions();

    /**
     * @brief Whether this is a preloaded window
     *
     * A preloaded window is a window which has never been shown to the user and
     * has a blank URL. Showing a preloaded window makes it loose its preloaded
     * status.
     *
     * @return `true` if the window is preloaded and `false` otherwise
     */
    bool isPreloaded() const;

    /**
     * @brief The index of the current tab
     * @return the index of the current tab
     */
    int currentTabIndex() const;

    /**
     * @brief The number of tabs
     * @return the number of tabs
     */
    int tabsCount() const;

    /**
     * @brief The tab in the given position
     *
     * @param idx the position of the tab
     * @return the tab at position @p idx
     *
     * @warning This assumes that the tab container has already been created
     */
    KonqFrameBase *tab(int idx) const;

    // Public for unit tests
    /**
     * @brief Collects information needed to show the popup menu
     *
     * @param items a list of selected items when the popup is requested
     * @param args information on how to open the files
     * @param browserArgs other information on how to open the files
     */
    void prepareForPopupMenu(const KFileItemList &items, const KParts::OpenUrlArguments &args, const BrowserArguments &browserArgs);

    /**
     * @brief The last time the window was deactivated
     *
     * @return the last time the window was deactivated, as milliseconds from Epoch. If the window was never deactivated, this is 0
     */
    qint64 lastDeactivationTime() const;

    /**
     * @brief An unique identifier for the window
     *
     * This is used by the activity support.
     * @return an unique identifier for the window
     */
    QString uuid() const {return m_uuid;}

    /**
     * @brief The activities the window is shown in
     *
     * @return a list of activities the window is shown in. If the window is shown
     * in all activities, an empty list is returned
     */
    QStringList activities() const;

    /**
     * @brief Changes the activities where the window is visible
     *
     * This function replaces the list of activities where the window is visible,
     * it doesn't add to it.
     *
     * @param ids the ids of the activities where the window should be visible
     */
    void setOnActivities(const QStringList &ids) const;

    /**
     * @brief Returns a list of all actions associated with toggable views
     *
     * @return a list of all actions associated with toggable views
     */
    QList<QAction*> toggleViewActions() const;

    /**
     * @brief Utility function which adjusts the Konq::Settings::newTabInFront setting according to whether the user is pressing the `shift` key
     *
     * Since the `shift` key reverses the value of the Konq::Settings::newTabInFront, when `shift` is pressed, this function returns the opposite
     * of the option.
     *
     * @return `Konq::Settings::newTabInFront()` if the user isn't pressing the `shift` key and `!Konq::Settings::newTabInFront()` otherwise
     */
    static bool newTabInFront(Qt::KeyboardModifiers mods);

    /**
     * @brief The starting directory for "Save As" dialogs when downloading files
     * @return the starting directory for "Save As" dialogs when downloading files
     */
    QString saveDir() const;

    /**
     * @brief Changes the starting directory for "Save As" dialogs when downloading files
     * @param dir the new starting directory. An empty value means using a default directory
     */
    void setSaveDir(const QString &dir);

    /**
     * @brief Whether this window should not be used for opening links requested from outside the window itself
     *
     * The user chooses to make a window protected when he doesn't want links clicked in other widnows or outside
     * Konqueror to be opened inside that window. Links clicked in views inside the window can be opened inside it
     * and create new tabs inside it.
     *
     * @return `true` if the window is protected and `false` otherwise
     */
    bool isProtected() const;

    /**
     * @brief The URL the user chose to open new window in
     *
     * It's the same as passing Konq::Settings::startUrl() to KonqMisc::konqFilteredUrl()
     *
     * @return the start URL passed through the appropriate URI filters
     */
    static QUrl startUrl();

    /**
     * @brief A list of all main windows which aren't preloaded windows
     * @return a list of all main windows which aren't preloaded windows
     */
    static QList<KonqMainWindow*> regularWindows();

Q_SIGNALS:

    /**
     * @brief Signal emitted when a new view has been added
     * @param view the new view
     */
    void viewAdded(KonqView *view);

    /**
     * @brief Signal emitted after a view has  been removed
     *
     * When this signal is emitted, the view has already been removed from the
     * main window but it hasn't yet been deleted.
     *
     * @param view the removed view
     */
    void viewRemoved(KonqView *view);

    /**
     * @brief Signal emitted when some items associated with the popup menu have
     * been removed
     */
    void popupItemsDisturbed();

    /**
     * @brief Signal emitted just before showing the configuration dialog
     */
    void aboutToConfigure();

    /**
     * @brief Signal emitted before closing the window
     * @param window the window which was closed (`this`)
     */
    void closing(KonqMainWindow *window);

public Q_SLOTS:

    /**
     * @brief Updates the "View mode" submenu
     *
     * This deletes the old menu and creates a new one containing the actions
     * to show the current URL in the available parts
     */
    void updateViewModeActions();

    /**
     * @brief Activates the given tab
     *
     * @param index the index of the tab to activate
     */
    void activateTab(int index);

    /**
     * @brief Slot called when switching the view mode for a single part
     *
     * This is called, for example, when switching from Icons to Details mode
     * in Dolphin part.
     *
     * It checks the action corresponding to the current view mode.
     */
    void slotInternalViewModeChanged();

    /**
     * @brief Slot called when the user presses the `Ctr+Tab` key sequence
     *
     * It activates the next tab.
     */
    void slotCtrlTabPressed();

    /**
     * @brief Slot called when a popup menu should be shown
     *
     * It creates and displays the popup menu. It's called in response to the
     * `KParts::NavigationExtension::slotPopupMenu()` signal which takes a `KFileItemList` as
     * second argument.
     *
     * @param global global coordinates where the popup should be shown
     * @param items list of file items which the popup applies to
     * @param args information on how to open URLs
     * @param browserArgs other information on how to open URLs
     * @param flags enables/disables certain builtin actions in the popupmenu
     * @param actionGroups named groups of actions which should be inserted into the popup
     * @param currentView the view for which the menu should be displayed
     */
    void slotPopupMenu(const QPoint &global, const KFileItemList &items, const KParts::OpenUrlArguments &args,
                       const BrowserArguments &browserArgs, KParts::NavigationExtension::PopupFlags flags,
                       const KParts::NavigationExtension::ActionGroupMap &actionGroups, KonqView* currentView);

    /**
     * @brief Overload of @link slotPopupMenu(const QPoint &, const KFileItemList &, const KParts::OpenUrlArguments &,
                       const BrowserArguments &, KParts::NavigationExtension::PopupFlags ,
                       const KParts::NavigationExtension::ActionGroupMap &, KonqView* ) slotPopupMenu()@endlink
     * It creates and displays the popup menu. It's called in response to the
     * `KParts::NavigationExtension::slotPopupMenu()` signal which takes a `QUrl` as second argument.
     * @param global global coordinates where the popup should be shown
     * @param url the URL this popup applies to
     * @param mode the file type of the URL
     * @param args information on how to open URLs
     * @param browserArgs other information on how to open URLs
     * @param f enables/disables certain builtin actions in the popupmenu
     * @param actionGroups named groups of actions which should be inserted into the popup
     * @param currentView the view for which the menu should be displayed
     */
    void slotPopupMenu(const QPoint &global, const QUrl &url, mode_t mode, const KParts::OpenUrlArguments &args,
                       const BrowserArguments &browserArgs, KParts::NavigationExtension::PopupFlags f,
                       const KParts::NavigationExtension::ActionGroupMap &actionGroups, KonqView* currentView);

    /**
     * @brief Slot called in response to the `openUrlRequestDelayed()` and
     * BrowserExtension::browserOpenurlRequestDelayed() signals
     *
     * It finds or create a suitable view to open the URL, then calls openUrl(KonqView*, const QUrl&, KonqOpenURLRequest&)
     *
     * @param url the URL to open
     * @param req information about how to open the URL
     */
    void slotOpenURLRequest(const QUrl &url, KonqOpenURLRequest &req);

    /**
     * @brief Overload of openUrl(KonqView*, const QUrl&, const QString&, const KonqOpenURLRequest&, bool)
     *
     * @param childView the view to open the URL in
     * @param url the URL to open
     * @param req information about how the URL should be opened
     */
    void openUrl(KonqView *childView, const QUrl &url, KonqOpenURLRequest &req);

    /**
     * @brief Slot called in response to a part's `createNewWindow()` or
     * @link BrowserExtension::browserCreateNewWindow() browserCreateNewWindow()@endlink
     * signal
     *
     * It creates a new window and opens the given URL inside it.
     *
     * @note This is not called in response to the "New window" action
     * @warning This function uses `QObject::sender()` so it can only be called
     * as response to a signal.
     *
     * @param url the URL top open in the new window
     * @param req information about how to open the URL
     * @param windowArgs information about how to create the window
     * @param [out] part a pointer where to store the part which displays @p url
     * in the new window
     */
    void slotCreateNewWindow(const QUrl &url, KonqOpenURLRequest& req,
                             const WindowArgs &windowArgs = WindowArgs(),
                             KParts::ReadOnlyPart **part = nullptr);

    /**
     * @brief Slot called in response to the "New Window" action
     *
     * It creates a new window pointing to the starting URL chosen by the user.
     * If preloading is enabled and a preloaded window exists, it will be used
     * and a new window will be preloaded.
     *
     * @see KonqMainWindowFactory::createNewWindow()
     */
    void slotNewWindow();

    /**
     * @brief Slot called in response to the "Duplicate window" action
     *
     * It creates and shows a new window which is a duplicate of this one.
     *
     * @see KonqViewManager::duplicateWindow()
     */
    void slotDuplicateWindow();

    /**
     * @brief Slot called in response to the "Send link address" action
     *
     * It creates an e-mail whose body contains link to the currently selected
     * files or to the current view URL if the current part doesn't have selected
     * files. The mail is then opened in the application associated with the `mailto`
     * protcol.
     */
    void slotSendURL();

    /**
     * @brief Slot called in response to the "Send file..." action
     *
     * It creates an e-mail having the files selected in the current view as
     * attachments, then opens the mail in the application associated with the
     * `mailto` protcol. If there aren't any selected files in the current view,
     * the URL of the view is attached.
     *
     * If any of the files are actually directories, a zip file is created for
     * each of them in a temporary directory and these files are attached to the
     * e-mail.
     *
     * @todo Delete the temporary directory used to store the zip files after
     * use. The problem is finding out when they have been used.
     */
    void slotSendFile();

    /**
     * @brief Copies the currently selected files to a destination chosen by the user
     *
     * If the current view doesn't have selected files, the URL of the view is copied
     * instead.
     */
    void slotCopyFiles();

    /**
     * @brief Moves the currently selected files to a destination chosen by the user
     *
     * If the current view doesn't have selected files, the URL of the view is copied
     * instead.
     */
    void slotMoveFiles();

    /**
     * @brief Slot called in response to the "Open location" action
     *
     * It sets focus to the location bar and selects its text
     */
    void slotOpenLocation();

    /**
     * @brief Slot called in response to the "Open File" action
     *
     * Shows a dialog where the user can choose a local file, then opens it as if
     * it had been entered in the location bar.
     */
    void slotOpenFile();

    // View menu
    /**
     * @brief Slot called when the user uses one of the actions to change the part
     * for the current URL
     *
     * It opens the URL in the part associated with the chosen part
     * @param action the action triggered by the user
     */
    void slotViewModeTriggered(QAction *action);

    /**
     * @brief Slot called in response to the "Lock to current location" action
     *
     * It switches the locked status of the current view
     */
    void slotLockView();

    /**
     * @brief Slot called in response to the "Link view" action
     *
     * If there are exactly two views in the current tab, it switches their linked
     * status. If there is just one window (which should never happen because then
     * the action is disabled) or more than two, it only changes the
     * linked status of the current view.
     */
    void slotLinkView();

    /**
     * @brief Reloads the given view
     *
     * This is the slot called when the user triggers the "Reload" action.
     *
     * @param view the view to reload. If `nullptr`, the current view will be
     * reloaded
     */
    void slotReload(KonqView *view = nullptr);

    /**
     * @brief Forces a hard reload of the current view
     *
     * @warning Currently, this does exactly the same as slotReload()
     */
    void slotForceReload();

    /**
     * @brief Slot called in response to the "Stop" action
     *
     * It stops loading the current view and the loading animation and shows an
     * appropriate message in the statusbar.
     */
    void slotStop();

    // Go menu
    /**
     * @brief Slot called in response to the "Up" action
     *
     * It opens the URL "above" the current one. The URL "above" is the one returned
     * by KonqView::upUrl()
     */
    void slotUp();

    /**
     * @brief Slot called in response to the "Back" action
     *
     * It opens the previous URL in history
     */
    void slotBack();

    /**
     * @brief Slot called in response to the "Forward" action
     *
     * It opens the next URL in history
     */
    void slotForward();

    /**
     * @brief Slot called in response to the "Home" action
     *
     * If the current view supports displaying directories, it opens the home
     * directory (according to `QDir::homePath()`), otherwise it opens the home
     * pages chosen by the user.
     */
    void slotHome();

    /**
     * @brief Slot called in response to the "Show History" action
     *
     * It displays the history dialog
     */
    void slotGoHistory();

    /**
     * @brief Slot called when a tab is about to be removed
     *
     * Adds the given tab to the list of closed tabs
     *
     * @param tab the removed tab
     */
    void slotAddClosedUrl(KonqFrameBase *tab);

    /**
     * @brief Slot called in response to the "Settings" action
     *
     * It shows the configuration dialog. The aboutToConfigure() signal is emitted
     * before showing the dialog.
     *
     * If the dialog had already been shown, it starts displaying the same module
     * it had when it was last closed. If it is shown for the first time, it
     * displays the first module.
     */
    void slotConfigure();

    /**
     * @brief Overload of slotConfigure()
     *
     * It behaves as slotConfigure() except that the configuration is shown displaying
     * the given module.
     *
     * @param module the module to display when the dialog is shown
     */
    void slotConfigure(Konq::ConfigDialog::Module module);

    /**
     * @brief Deletes the configuration dialog
     */
    void slotConfigureDone();

    /**
     * @brief Slot called in response to the "Configure toolbars" action
     *
     * It shows a dialog where the user can configure the toolbars
     */
    void slotConfigureToolbars();

    /**
     * @brief Slot called in response to the "Configure extensions" action
     *
     * It displays a dialog where the user can select the plugins Konqueror should
     * load.
     *
     * @note The loading and unloading of plugins is done by the dialog itself.
     * @see KonqExtensionManager
     */
    void slotConfigureExtensions();

    /**
     * @brief Slot called in response to the "Configure spell checking" action
     *
     * It shows a dialog where the user can configure the spell checker and applies
     * the changes made by the user.
     */
    void slotConfigureSpellChecking();

    /**
     * @brief Slot called when the user presses the Apply or Ok button in the
     * toolbar configuration dialog
     *
     * It applies the changes made by the user.
     */
    void slotNewToolbarConfig();

    /**
     * @brief Slot called when the availability of undo operations change
     *
     * It enables or disables the "Undo" action depending on the availability of
     * undo operations.
     *
     * @param avail `true` if there are undo operations available and `false` if
     * there aren't
     */
    void slotUndoAvailable(bool avail);

    /**
     * @brief Slot called when the active part changes
     *
     * It updates #m_mapViews, inserts the part in the part manager, activates
     * the new part if the old part was the active one and updates the view-related
     * actions.
     *
     * @param childView the view whose part has changed
     * @param oldPart the original part of the view
     * @param newPart the new part of the view
     */
    void slotPartChanged(KonqView *childView, KParts::ReadOnlyPart *oldPart, KParts::ReadOnlyPart *newPart);

    /**
     * @brief Slot called when a UrlLoader finishes to load an URL
     *
     * In case of errors, it emits the DBus signal telling instances to remove the URL
     * from the combo box, stops the animation, tells the view to stop loading and
     * resets the location bar URL to the last working one (unless the URL was
     * entered by the user).
     *
     * If #m_bNeedApplyKonqMainWindowSettings is `true`, it also applies the main
     * window settings if there weren't errors.
     */
    void urlLoaderFinished(UrlLoader *loader);

    /**
     * @brief Slot called in response to the "Clear location bar" action
     *
     * It stops loading the current URL, clears the temporary URL from the location
     * bar and gives it focus.
     */
    void slotClearLocationBar();

    // reimplement from KParts::MainWindow
    /**
     * @brief Override of `KParts::MainWindow::slotSetStatusBarText()`
     *
     * It does nothing, since each view has its own status bar.
     * @param text unused
     */
    void slotSetStatusBarText(const QString &text) override;

    // public for KonqViewManager
    /**
     * @brief Slot called when a new part is activated
     *
     * It exits full screen, makes the necessary connections and disconnections,
     * enables and disables the appropriate actions, creates the part's GUI and
     * updates the caption and the location bar.
     *
     * @param part the new active part
     */
    void slotPartActivated(KParts::Part *part);

    /**
     * @brief Activates an history entry
     *
     * @note This doesn't immediately load the new history item. It does so with
     * a delay of 0.
     *
     * @param steps the number of steps of the history entry to activate relative
     * to the current one. If positive, it moves forwards in history; if negative,
     * it moves backwards
     */
    void slotGoHistoryActivated(int steps);

    /**
     * @brief Slot called in response to the "New Tab" action
     *
     * It creates a new empty tab and gives focus to the location bar.
     */
    void slotAddTab();

    /**
     * @brief Slot called in response to the "Split Left/Right" action
     *
     * Splits the current tab horizontally.
     *
     * @see splitCurrentView()
     */
    void slotSplitViewHorizontal();

    /**
     * @brief Slot called in response to the "Split Top/Bottom" action
     *
     * Splits the current tab vertically.
     *
     * @see splitCurrentView()
     */
    void slotSplitViewVertical();

    /**
     * @brief Slot called in response to the "Close Other Tabs" action
     *
     * It removes all tabs except the current one.
     */
    void slotRemoveOtherTabs();

    /**
     * @brief Slot called in response to the "Close tab" action in a tab's context menu
     *
     * It closes the current tab (asynchronously)
     */
    void slotRemoveTabPopup();

    /**
     * @brief Slot called in response to the "Reload all tabs"
     *
     * It reloads all tabs, after asking the user what to do for tabs with unsubmitted
     * changes.
     */
    void slotReloadAllTabs();

    /**
     * @brief Slot called in response to the "Close other tabs" action in a tab's context menu
     *
     * It closes (asynchronously) all tabs except the one where the context menu was shown
     */
    void slotRemoveOtherTabsPopup();

    /**
     * @brief Saves the main window settings
     *
     * For some windows saving window settings is not appropriate, for example for windows
     * with no toolbars opened by javascript using `window.open`. In these cases,
     * this function does nothing
     */
    void forceSaveMainWindowSettings();

    /**
     * @brief Slot called in response to the "Duplicate tab" action in a tab's context menu
     *
     * It duplicates the tab corresponding to the popup.
     */
    void slotDuplicateTabPopup();

    /**
     * @brief Slot called in response to the "Reload" action in a tab's context menu
     *
     * It reloads the active view in tab corresponding to the popup.
     */
    void slotReloadPopup();

    /**
     * @brief Slot called in response to the "Detach tab" action in a tab's context menu
     *
     * It creates a new window with a single tab having the same content of the tab
     * corresponding to the popup, then remove the original tab.
     */
    void slotBreakOffTabPopup();

private Q_SLOTS:

    /**
     * @brief Slot called in response to a view @link KonqView::viewCompleted viewCompleted@endlink
     * signal
     *
     * It updates the directory of the URL completion object so that it matches
     * that in the location bar.
     * @param view the view which has completed loading. Unused.
     */
    void slotViewCompleted(KonqView *view);

    /**
     * @brief Slot called when the user presses Enter in the location bar or otherwise activates it
     *
     * It opens the URL entered by the user in this tab or in a new tab depending
     * on the keyboard modifiers.
     *
     * @param text the text in the location bar. It doesn't need to be a full URL,
     * as `KUriFilter` will be used to attempt creating a real URL from it
     * @param modifiers the keyboard modifiers when the user activates the location
     * bar
     */
    void slotURLEntered(const QString &text, Qt::KeyboardModifiers modifiers);

    /**
     * @brief Slot called when the user activates the location label
     *
     * It gives focus to the location bar and selects its contents.
     */
    void slotLocationLabelActivated();

    /**
     * @brief Slot called in response to the "duplicate tab" action
     *
     * It creates a new tab which is a copy of the current one.
     */
    void slotDuplicateTab();

    /**
     * @brief Slot called in response to the "detach tab" action
     *
     * It creates a new window with a single tab having the same content of the
     * current tab, then removes the current tab.
     */
    void slotBreakOffTab();

    /**
     * @brief Detaches the tab with the given index
     *
     * It creates a new window with a single tab having the same content of the
     * tab with index @p tabIndex, then removes that tab. In case the tab contains
     * modified data, it asks confirmation from the user.
     */
    void breakOffTab(int tabIndex);

    /**
     * @brief Slot called in response to the "Open in new window" action in the
     * popup menu
     *
     * It opens each of the items associated with the menu in a new window.
     * @note A new window is opened for each file.
     */
    void slotPopupNewWindow();

    /**
     * @brief Slot called in response to the "Open in this window" popup menu action
     *
     * It opens the first of the URLs associated with the popup in the current
     * window.
     */
    void slotPopupThisWindow();

    /**
     * @brief Slot called in response to the "Open in new tab" popup menu action
     *
     * It opens the URLs associated with the popup each in a new tab.
     *
     * If the window where the popup menu was displayed is a popup window, the tabs
     * are opened in the non-popup window associated with it and that window is
     * raised.
     */
    void slotPopupNewTab();

    /**
     * @brief Slot called in response to the "Paste into" popup menu action
     *
     * It calls the `pasteTo()` method of the part corresponding to the popup using
     * KonqView::callExtensionURLMethod().
     */
    void slotPopupPasteTo();

    /**
     * @brief Slot called in response to the "Close active view" action
     *
     * It removes the active view, after asking confirmation from the user if the
     * view contained unsent data, and activates another view.
     */
    void slotRemoveView();

    /**
     * @brief Slot called in response to the "Close current tab" action
     *
     * It removes the current tab, after asking the user for confirmation if it
     * contains unsent data.
     */
    void slotRemoveTab();

    /**
     * @brief Removes the tab with the given index
     *
     * If the tab contained unsent data, the user is asked for confirmation.
     *
     * It updates the view actions after removing the tab.
     * @param tabIndex the index of the tab to remove. If there isn't a tab associated
     * with that index, the function does nothing.
     */
    void removeTab(int tabIndex);

    /**
     * @brief Remove all tabs in the window except the one with the given index
     *
     * The user is asked for confirmation before closing the tabs, and another
     * confirmation is asked for tabs which contain unsent data.
     *
     * After removing the tabs, the view-related actions are updated.
     *
     * @param tabIndex the index of the only tab to keep
     */
    void removeOtherTabs(int tabIndex);

    /**
     * @brief Slot called in response to the "Activate next tab" action
     *
     * It activates the tab next to the active one
     */
    void slotActivateNextTab();

    /**
     * @brief Slot called in response to the "Activate previous tab" action
     *
     * It activates the tab previous to the active one
     */
    void slotActivatePrevTab();

    /**
     * @brief Slot connected to the "Activate tab ..." actions
     *
     * It activates the tab whose index matches the object name of the sender.
     *
     * @internal The sender must have a name ending with two digits, which are
     * considered the index of the tab increased by one.
     */
    void slotActivateTab();

    /**
     * @brief Debug function which prints the list and full hierarchy of views
     */
    void slotDumpDebugInfo();

    /**
     * @brief Slot called in response to the "Preview in ..." actions via PopupMenuGUIClient::openEmbedded
     *
     * It displays the URL of the current view in the part @p part.
     *
     * @param part the part to preview the current URL in
     */
    void slotOpenEmbedded(const KPluginMetaData &part);

    // Used to be connected to KGlobalSettings
    /**
     * @brief Read again the configuration file
     */
    void slotReconfigure();

    /**
     * @brief Slot called in response to one of the "Open with..." actions
     *
     * It determines the action which has been chosen by the user using `sender()`,
     * then launches it with the URL of the current view.
     *
     * It does nothing if there's no current view.
     *
     * @note This function uses `sender()`, so it must only be called from a signal-slot
     * connection.
     */
    void slotOpenWith();

    /**
     * @brief Synchronizes WebEnginePart proxy settings with the KDE ones
     *
     * It reads the KDE proxy settings and tries to apply them to WebEnginePart.
     * If the user's settings can't be applied to `WebEnginePart` because `QtWebEngine`
     * doesn't support them, the user is warned.
     *
     * This function is connected via DBus to the `reparseSlaveConfiguration`
     * signal of the KIO scheduler.
     *
     * @param updateProtocolManager whether to call `KProtocolManager::reparseConfiguration()`
     * before updating settings. Pass `false` only if you know that protocol settings
     * haven't changed.
     */
    void updateProxyForWebEngine(bool updateProtocolManager = true);

#if 0
    void slotGoMenuAboutToShow();
#endif

    /**
     * @brief Slot called before the popup menu associated with the "Up" action
     * is shown
     *
     * It fills the popup with actions to go to each ancestor URL of the current
     * view URL
     */
    void slotUpAboutToShow();

    /**
     * @brief Slot called before the popup menu associated with the "Back" action
     * is shown
     *
     * It fills the popup with actions for all past history entries
     */
    void slotBackAboutToShow();

    /**
     * @brief Slot called before the popup menu associated with the "Forward" action
     * is shown
     *
     * It fills the popup with actions for all future history entries
     */
    void slotForwardAboutToShow();

    /**
     * @brief Slot called before the popup menu associated with the "Closed item" action
     * is shown
     *
     * It fills the popup with actions to reopen each of the closed items and an
     * action to clear the list of closed items
     */
    void slotClosedItemsListAboutToShow();

    /**
     * @brief Updates the "Closed items" action
     *
     * It enables or disables it depending on the availability of closed items
     * and changes its text according to KonqUndoManager::undoText()
    */
    void updateClosedItemsAction();

    /**
     * @brief Slot called before the session list popup menu is shown
     *
     * It fills the menu with a list of existing sessions and actions to create
     * a new session and to manage sessions.
     */
    void slotSessionsListAboutToShow();

    /**
     * @brief Slot called in response to the "Save session as" action
     *
     * It displays the NewKonqSessionDlg dialog where the user can choose to save the
     * sessions.
     */
    void saveCurrentSession();

    /**
     * @brief Slot called in response to the "Manage sessions" action
     *
     * It displays the KonqSessionDlg dialog which allows the user to manage saved
     * sessions.
     */
    void manageSessions();

    /**
     * @brief Slot called when the user clicks on one of the session actions
     *
     * It restores the session corresponding to the action.
     *
     * @param action the activated action
     */
    void slotSessionActivated(QAction *action);

    /**
     * @brief Slot called in response to the actions in the "Up" popup menu
     *
     * It opens the URL associated with the action.
     *
     * @param action the action which was activated
     */
    void slotUpActivated(QAction *action);

    /**
     * @brief Slot called in response to the actions in the "Back" popup menu
     *
     * It goes back in history by a number of steps corresponding to the data stored
     * in the action.
     *
     * This function assumes that the data associated with the action is an int
     * corresponding to the number of steps to move back in history.
     *
     * @param action the action which was activated
     */
    void slotBackActivated(QAction *action);

    /**
     * @brief Slot called in response to the actions in the "Forward" popup menu
     *
     * It goes forward in history by a number of steps corresponding to the data stored
     * in the action.
     *
     * This function assumes that the data associated with the action is an int
     * corresponding to the number of steps to move forward in history.
     *
     * @param action the action which was activated
     */
    void slotForwardActivated(QAction *action);

    /**
     * @brief Slot called in response to the actions in the "Home" popup menu
     *
     * It opens the URL associated with the action
     *
     * @param action the action which was activated
     */
    void slotHomePopupActivated(QAction *action);

    /**
     * @brief Slot called by the timer in slotGoHistoryActivated()
     *
     * It loads the history entry corresponding to #m_goBuffer. It determines
     * whether to use a new tab or window or the current tab using #m_goKeyboardState
     * and #m_goKeyboardState.
     */
    void slotGoHistoryDelayed();

    /**
     * @brief Slot called when the user changes the completion mode for the combo box
     *
     * It propagates the changes to other main windows.
     *
     * @param m the new completion mode
     */
    void slotCompletionModeChanged(KCompletion::CompletionMode m);

    /**
     * @brief Slot called in response to the KonqCombo::completion() signal
     *
     * It attempts to find a completion for the string in the combo box. It first
     * tries a completion using #m_pURLCompletion. If no completion is found there,
     * it tries using the global #s_pCompletion completer.
     *
     * @p text the text the user typed in the combo box and which should be completed
     */
    void slotMakeCompletion(const QString &text);

    /**
     * @brief Slot called in response to the KonqCombo::substringCompletion() signal
     *
     * It finds substring completions from both #m_pURLCompletion and #s_pCompletion.
     * Once completions have been found, they're added to the combobox.
     *
     * The order in which completions are added depends on whether the current URL
     * is local or not: if it's local, completions from #m_pURLCompletion have
     * precedence, otherwise completions from #s_pCompletion do.
     *
     * @p text the text the user typed in the combo box and which should be completed
     */
    void slotSubstringcompletion(const QString &text);

    /**
     * @brief Slot called in response to the KonqCombo::textRotation() signal
     *
     * It finds the next of the previous completion entry, depending on @p type.
     * It first look into #m_pURLCompletion and then in #s_pCompletion. If a completion
     * is found, it passes it to the combobox.
     *
     * This function sets #m_urlCompletionStarted to `false`, so that when slotMatch()
     * is called in response to the next `match()` signal, it will do nothing.
     *
     * @param type the keybinding which triggered the signal. If it's not `PrevCompletionMatch`
     * or `NextCompletionMatch`, this function only changes #m_urlCompletionStarted
     */
    void slotRotation(KCompletionBase::KeyBindingType type);

    /**
     * @brief Slot called in response to the url completion object `match()` signal
     *
     * If a match has been found, it either calls KonqCombo::setCompletedItems()
     * with all completions found by both #m_pURLCompletion and
     * #s_pCompletion or calls KonqCombo::setCompletedText()
     * with @p match, depending on whether or not the completion mode requires a popup
     * menu.
     *
     * This function does nothing if #m_urlCompletionStarted is `false`. After
     * a call to this function, #m_urlCompletionStarted will always be `false`
     *
     * @p match the match found by the completion object
     */
    void slotMatch(const QString &match);

    /**
     * @brief Slot called in response to the `cleared()` signal of the combobox
     *
     * It calls KonqHistoryProvider::emitCleared() on the single KonqHistoryProvider
     * obect.
     */
    void slotClearHistory();

    /**
     * @brief Slot called in response to the HistoryProvider::cleared signal
     *
     * It clears the combo box history, unless it's empty.
     */
    void slotClearComboHistory();

    /**
     * @brief Slot called in response to the `QClipboard::dataChangedSignal()`
     *
     * It enables or disables the copy, cut and paste actions depending on the
     * clipboard contents.
     *
     * This is only used if no part has focus, as otherwise the part itself takes
     * care of the actions.
     */
    void slotClipboardDataChanged();

    /**
     * @brief Enables or disables the "Copy" and "Cut" actions depending on whether
     * there's a selection in the combo box
     *
     * If there's selected text in the combo box the actions are enabled, otherwise
     * they're disabled.
     */
    void slotCheckComboSelection();

    /**
     * @brief Slot called in response to the "Show Menubar" action
     *
     * It toggles the visibility of the menu.
     */
    void slotShowMenuBar();

    /**
     * @brief Slot called in response to the "Show statusbar" action
     *
     * It toggles the visiblity of the status bar.
     */
    void slotShowStatusBar();

    /**
     * @brief Slot called in response to the one of the actions in the list of
     * most recent or most used URLs
     *
     * It opens the URL associated with the action.
     * @param url the URL to open
     */
    void slotOpenURL(const QUrl &url);

    /**
     * @brief Slot called in response to changes in the icons provided by the KonqPixmapProvider
     *
     * It updates the icons for each view and the application window.
     */
    void slotIconsChanged();

    /**
     * @brief Override of `KParts::MainWindow::event()`
     *
     * It:
     * - propagates events of type KonqFileSelectionEvent, KonqFileMouseOverEvent,
     * KParts::PartActivateEvent and KParts::OpenUrlEvent to all parts
     * - updates the activation time for `QEvent::ActivationChanged` events
     * - shows a message in the status bar for `QEvent::StatusTip` events
     *
     * @return `true` for KonqFileSelectionEvent, KonqFileMouseOverEvent,
     * KParts::PartActivateEvent events and the same as `KParts::MainWindow::event()
     * for all other events
     */
    bool event(QEvent *) override;

    /**
     * @brief Slot called in response to the "Move tab to the left" action
     *
     * It switches the position of the current tab with the tab on its left.
     */
    void slotMoveTabLeft();

    /**
     * @brief Slot called in response to the "Move tab to the right" action
     *
     * It switches the position of the current tab with the tab on its right.
     */
    void slotMoveTabRight();

    /**
     * @brief Slot called when the user asks to add a web sidebar module for the
     * given URL
     *
     * It asks the user for confirmation, then asks the sidebar to create the module
     * by emitting the KParts::NavigationExtension::addWebSideBar() signal.
     * @param url the URL to create the module for
     * @param name the name of the module
     */
    void slotAddWebSideBar(const QUrl &url, const QString &name);

    /**
     * @brief Updates the full screen status
     *
     * @param set whether the full screen status should be enabled or disabled
     * @warning Don't call this function directly
     */
    void slotUpdateFullScreen(bool set);   // do not call directly

    /**
     * @brief Closes all windows except this one
     *
     * This is connected to the "Close other windows" action. If there are more than one other
     * window, the user is asked for confirmation before closing.
     */
    void slotCloseOtherWindows();

protected:

    /**
     * @brief Override of `KParts::MainWindow::eventFilter()`
     *
     * The event filter is installed on the combo box and has the following effects:
     * - for `FocusIn` and `FocusOut` events, it connects or disconnects signals
     * to update the status of the "Cut", "Copy" and "Paste" actions
     * - for `KeyPress` events, it calls slotCtrlTabPressed() if the pressed keys
     * correspond to the `Ctrl+Tab` shortcut and * resets the text to the URL of
     * the current view if `ESC` is pressed.
     *
     * @param obj the object which received the event
     * @param ev the event to filter
     * @return `true` for `KeyPress` event corresponding to `Ctrl+Tab` or `ESC`
     * and the same as `KParts::MainWindow::eventFilter()` in all other cases
     */
    bool eventFilter(QObject *obj, QEvent *ev) override;

   /**
    * @brief Override of `KParts::MainWindow::showEvent()`
    *
    * It marks the window as regular window (if it previously was preloaded) and
    * updates the status of the "Show Menubar", "Show statusbar" actions and
    * the visibility of the bookmark toolbar.
    *
    * @internal
    * Updating the status of the actions and the visibility of the toolbar can't
    * be done in the constructor because information about these settings
    * are stored in view profiles which are read after the constructor has finished
    * @endinternal
    *
    * @param event the event
    */
    void showEvent(QShowEvent *event) override;

    /**
     * @brief Makes the sidebar and all view following a given view display the same URL
     *
     * After a call to this function, all views linked with @p senderView will
     * display @p url. The sidebar is also made to show @p url, since it should
     * always follow the active view.
     *
     * @param url the URL to open
     * @param args information on how to open the URL
     * @param browserArgs other information on how to open the URL
     * @param type the mimetype of @p url or the part capability of the part to
     * use
     * @param senderView the view which has been asked to show @p url
     */
    bool makeViewsFollow(const QUrl &url,
                         const KParts::OpenUrlArguments &args,
                         const BrowserArguments &browserArgs,
                         const Konq::ViewType &type, KonqView *senderView);

    /**
     * @brief Shows the toggable views chosen by the user
     */
    void applyKonqMainWindowSettings();

    /**
     * @brief Updates all the view-related actions
     */
    void viewsChanged();

    /**
     * @brief Override of `KParts::MainWindow::closeEvent()`
     *
     * It asks the user confirmation when closing multiple tabs and when there are
     * pages with unsubmitted data, emits the closing() signal and informs all parts
     * that the window is being closed.
     *
     * @param e the event
     */
    void closeEvent(QCloseEvent *e) override;

    /**
     * @brief Shows a dialog which asks the user for an existing file or directory
     *
     * @param text the text to display in the dialog
     * @param [out] url variable where the chosen URL will be stored
     * @return `true` if the user chose an URL and `false` if the user canceled
     * the dialog
     */
    bool askForTarget(const KLocalizedString &text, QUrl &url);
    
private Q_SLOTS:

    /**
     * @brief Slot called when the undo text changes
     *
     * It updates the text of the undo action.
     * @param newText the new undo text
     */
    void slotUndoTextChanged(const QString &newText);

    /**
     * @brief Slot called in response to the "Konqueror Introduction" action
     *
     * It opens the introduction page.
     */
    void slotIntro();

    /**
     * @brief Slot called in response to the `KParts::NavigationExtension::itemsRemoved()` signal
     *
     * It removes the items from the list of items associated with the popup menu.
     * @param items the removed items
     */
    void slotItemsRemoved(const KFileItemList &items);

    /**
    * @brief Slot called in response to the "Go" action
    *
    * It loads the url displayed currently in the lineedit of the locationbar, by
    * emulating a enter key press event.
    */
    void goURL();

    /**
     * @brief Slot called when the delayed initialization happens
     *
     * It calls addBookmarksIntoCompletion() to add the bookmarks to the completion
     * object.
     */
    void bookmarksIntoCompletion();

    /**
     * @brief Creates the bookmarks toolbar
     *
     * Creates the bookmark bar and fills the corresponding toolbar.
     *
     * If the bookmark bar already exists, it's deleted.
     */
    void initBookmarkBar();

    /**
     * @brief Slot called in response to KonqCombo::showPageSecurity() signal
     *
     * It calls the current part's "security" action, if it exists.
     */
    void showPageSecurity();
    
    /**
     * @brief Toggles complete fullscreen mode
     *
     * Complete fullscreen mode is a mode where all UI elements (title bar, menu bar,
     * toolbars, statusbar, sidebar, tabbar) other UI elements are hidden.
     *
     * When entering complete fullscreen mode, this function shows a dialog telling
     * the user how to exit this mode, then hides all the UI elements.
     *
     * @todo Complete full screen mode is mostly used when a web page asks to be
     * shown in "full screen" mode, usually when the user is viewing a video. In
     * these circumstances, the part requesting the complete full screen mode
     * should fill the entire screen. Currently, however, this doesn't happen: if
     * the current tab contains split views, they will all remain visible.
     *
     * @param on whether complete fullscreen should be turned on or off
     */
    void toggleCompleteFullScreen(bool on);

    /**
     * @brief Updates the spellchecker configuration
     *
     * After the configuration has been updated, this function emits
     * causes the single instance of KonqSpellCheckingConfigurationDispatcher
     * to emit the @link KonqSpellCheckingConfigurationDispatcher::spellCheckingConfigurationChanged()
     * spellCheckingConfigurationChanged()@endlink signal.
     *
     * Since Sonnet doesn't provide an interface to determine whether the spell
     * checker should be enabled by default, this function copies the
     * "checkerEnabledByDefault" setting from the Sonnet configuration file
     * to Konqueror's own configuration file.
     */
    void updateSpellCheckConfiguration();

    /**
     * @brief Slot called in response to the "Inspect current page" action
     *
     * If the current part supports displaying a "developer tools" page, it splits
     * the current view vertically and shows the developer tools page in the the
     * lower half.
     *
     * Currently, only WebEnginePart provides a developer tools page.
     */
    void inspectCurrentPage();
    
private:

    /**
     * @brief Sets the window icon according to the current text in the location bar
     */
    void updateWindowIcon();

    /**
     * @brief Detects a shell glob at the end of the given URL
     *
     * A shell glob in the URL can be used, for example, to only show files with
     * a given extension in a directory: entering `/some/path/ *.txt` will show
     * only files with the `.txt` extension.
     *
     * @param [inout] url the URL where to detect the name filter. If a shell glob
     * is found, this function removes it from @p url
     * @return the shell glob or an empty string if no shell glob was found
     */
    QString detectNameFilter(QUrl &url);

    /**
    * @brief Hides the bookmark bar if it's empty
    */
    void updateBookmarkBar();

    /**
    * @brief Recursively adds all bookmarks a given bookmark group to the static
    * completion object
    *
    * For bookmarks corresponding to local, http and ftp URLs, it also adds a
    * version of the URL without the scheme.
    *
    * @param group the bookmark group
    */
    static void addBookmarksIntoCompletion(const KBookmarkGroup &group);

    /**
     * @brief A list of all suitable matches for popup autocompletion of the given string
     *
     * If no matches are found, it tries finding matches by prepending common
     * prefixes such as `http://` or `www.` to the string, or event `http://www.`
     *
     * @warning These completions can't be used for manual completion or autocompletion
     * due to the fact that they may have texted prepended to what the user entered.
     * They can only be used for the popup autocompletion.
     *
     * @param s the string to autocomplete
     * @return a list of possible completion strings.
     */
    static QStringList historyPopupCompletionItems(const QString &s = QString());

    /**
     * @brief Starts the animation of the Konqueror logo
     *
     * It also enables the "Stop" action.
     */
    void startAnimation();

    /**
     * @brief Stops the animation of the Konqueror logo
     *
     * It also disables the "Stop" action.
     */
    void stopAnimation();

    /**
     * @brief Enables or disables the "Up" action
     *
     * The action is enabled or disabled depending on whether there exists an URL
     * above @p url.
     *
     * @param url the URL to use for determining whether to enable or disable the
     * "Up" action
     */
    void setUpEnabled(const QUrl &url);

    /**
     * @brief Enable or disables the clear button in the combo box depending on
     * whether the "Clear" button exists in the location toolbar
     *
     * If the location toolbar contains the "Clear" button, the clear button in
     * the combobox is disabled as it would be a duplicate, otherwise it's enabled.
     */
    void checkDisableClearButton();

    /**
     * @brief Creates and initializes the combo box
     *
     * It makes the necessary signal-slot connections, sets the completion object
     * starts the delayed initialization of the bookmarks-based completion object
     * and and installs an event filter.
     */
    void initCombo();

    /**
     * @brief Creates and connects all actions
     */
    void initActions();

    /**
     * @brief Create new tabs for the items associated with the current popup menu
     *
     * The tabs are created in this window, unless its a popup window, in which
     * case they're created in its proxy window.
     *
     * @param infront whether or not to raise the last of the new tabs to the foreground
     * @param openAfterCurrentPage whether the tabs should be opened after the
     * current page or at the end of the tab bar
     */
    void popupNewTab(bool infront, bool openAfterCurrentPage);

    /**
     * @brief Adds the window to the list of closed views for undo operations
     */
    void addClosedWindowToUndoList();

    /**
    * @brief Attempts to find a index.html (.kde.html) file in the given directory
    *
    * It tries three variations of the name: `index.html`, `index.htm` and `index.HTML`,
    * in this order.
    *
    * @param directory the directory where to look for the file
    * @return the path of the file or an empty string if no such file was found.
    */
    static QString findIndexFile(const QString &directory);

    /**
     * @brief Connects the slots provided by a given `KParts::NavigationExtension`
     * with the corresponding actions
     *
     * The connected actions are those provided by `KParts::NavigationExtension::actionSlotMap()`.
     * They're connected with the `triggered()` signal of the action with same name
     * in actionCollection(). It also enables or disables the action depending on
     * the value returned by `KParts::NavigationExtension::isActionEnabled()` and
     * updates the action's text if a specific text is provided by the extension.
     *
     * @param ext the extension to connect to
     */
    void connectExtension(KParts::NavigationExtension *ext);

    /**
     * @brief Disconnects the signals connected by connectExtension()
     *
     * @note This doesn't change the enabled state of the actions or modify their
     * text.
     *
     * @param ext the extension to disconnect from
     */
    void disconnectExtension(KParts::NavigationExtension *ext);

    /**
     * @brief Inserts the "View mode" popup menu in the action collection
     *
     * The menu is filled with the actions to display the current URL in all
     * available parts.
     */
    void plugViewModeActions();

    /**
     * @brief Removes the "View mode" popup menu from the action collection
     */
    void unplugViewModeActions();

    /**
     * @brief Splits the current view
     *
     * Depending on the value returned by Settings::alwaysDuplicatePageWhenSplittingView(),
     * and on whether the current URL is local or not, the new view will display
     * the same URL of the current one or the starting page.
     *
     * @param orientation the orientation the view should be split in
     */
    void splitCurrentView(Qt::Orientation orientation);

    /**
     * @brief An object representing the tab containing the given view
     *
     * @param view the view
     * @return an object representing the tab where the view is or `nullptr` if
     * the view isn't in any tab. Callers of this function shouldn't rely on what
     * exactly this object is, but only on the fact that it's unique for each tab,
     * meaning that this function returns the same object for views in the same
     * tab and different objects for views in different tabs.
     */
    QObject *lastFrame(KonqView *view);

    /**
     * @brief The line edit associated with the combo box
     *
     * @return The line edit associated with the combo box or `nullptr` if the combo
     * box doesn't exist
     */
    QLineEdit *comboEdit();

    /**
     * @brief Attempts to show the given window behind this window
     *
     * Depending on window manager and platform support, this may or may not completely work
     *
     * @param window the window to show
     */
    void showBehindThis(KonqMainWindow *window);

private: // members

    //Interfaces
    KonqImplementations::KonqWindow *m_windowInterface; //!< The object implementing the KonqWindow interface

    bool m_isPreloaded; //!< Whether or not the window is preloaded

    KonqUndoManager *m_pUndoManager; //!< The undo manager

    KNewFileMenu *m_pMenuNew; //!< The popup menu containing the "New" actions for the context menu

    QAction *m_paPrint; //!< The "Print" action

    KBookmarkActionMenu *m_pamBookmarks; //!< The action containing the bookmarks menu

    QAction *m_paCloseOtherWindows; //!< An action to close all windows except this one
    QAction *m_paQuitKonqueror; //!< An Action to close all windows and quit Konqueror
    KToolBarPopupAction *m_paUp; //!< The popup menu for the "Up" action. It contains a list of all parent URLs
    KToolBarPopupAction *m_paBack; //!< The popup menu for the "Back" action. It contains the previous history entries
    KToolBarPopupAction *m_paForward; //!< The popup menu for the "Forward" action. It contains the following history entries
    KToolBarPopupAction *m_paHomePopup; //!< The popup menu for the "Home" action. It contains an action to go to the home page or the home directory
    /// Action for the trash that contains closed tabs/windows
    KToolBarPopupAction *m_paClosedItems; //!< The popup menu for the "Closed Item" action. It contains a list of closed tabs and windows
    KActionMenu *m_paSessions; //!< The popup menu for the "Session" action. It contains a list of saved sessions
    QAction *m_paHome; //!< The "Home" action

    QAction *m_paSplitViewHor; //!< The "Split view horizontally" action
    QAction *m_paSplitViewVer; //!< The "Split view vertically" action
    QAction *m_paAddTab; //!< The "New tab" action
    QAction *m_paDuplicateTab; //!< The "Duplicate current tab" action
    QAction *m_paBreakOffTab; //!< The "Detach tab" action
    QAction *m_paRemoveView; //!< The "Close view" action
    QAction *m_paRemoveTab; //!< The "Close tab" action
    QAction *m_paRemoveOtherTabs; //!< The "Close other tabs" action
    QAction *m_paActivateNextTab; //!< The "Next tab" action
    QAction *m_paActivatePrevTab; //!< The "Previous tab" action

    KToggleAction *m_paLockView; //!< The "Lock view" action
    KToggleAction *m_paLinkView; //!< The "Link view" action
    QAction *m_paReload; //!< The "Reload" action
    QAction *m_paForceReload; //!< The "Force Reload" action
    QAction *m_paReloadAllTabs; //!< The "Reload all tabs" action
    QAction *m_paUndo; //!< The "Undo" action
    QAction *m_paCut; //!< The "Cut" action
    QAction *m_paCopy; //!< The "Copy" action
    QAction *m_paPaste; //!< The "Paste" action
    QAction *m_paStop; //!< The "Stop" action

    QAction *m_paCopyFiles; //!< The "Copy Files" action
    QAction *m_paMoveFiles; //!< The "Move Files" action

    QAction *m_paMoveTabLeft; //!< The "Move Tab Left" action
    QAction *m_paMoveTabRight; //!< The "Move Tab Right" action

    QAction *m_paConfigureExtensions; //!< The "Configure Extensions" action
    QAction *m_paConfigureSpellChecking; //!< The "Configure Spell Checking" action

    KonqAnimatedLogo *m_paAnimatedLogo; //!< The animated logo button

    /**
     * @brief A representation of the bookmarks toolbar
     *
     * @warning This is not a `QToolBar`, but an object which contains a representation
     * of the toolbar contents and manages them.
     */
    KBookmarkBar *m_paBookmarkBar;

#if 0
    KToggleAction *m_paFindFiles;
#endif

    KToggleAction *m_paShowMenuBar; //!< The "Show Menu Bar" action
    KToggleAction *m_paShowStatusBar; //!< The "Show Status Bar" action

    KToggleFullScreenAction *m_ptaFullScreen; //!< The "Full Screen Mode" action

    QAction *m_paShowDeveloperTools; //!< The "Inspect Current Page" action

    KToggleAction *m_protectWindow; //!< The "Protect window" action

    /**
     * @brief Whether the constructor has finished running
     *
     * This is needed to avoid a situation when, in particular circumstances,
     * saveProperties() is called by the session manager before the constructor
     * finishes running.
     */
    bool m_fullyConstructed: 1;

    /**
     * @brief Whether the location bar has focus
     *
     * This is used to determine how to treat the "Copy", "Paste" and "Cut" actions
     */
    bool m_bLocationBarConnected: 1;

    /**
     * @brief Whether an `enter` key press in the location bar is being processed
     *
     * This avoid starting processing a new `enter` key press before the previous
     * one has been finished.
     */
    bool m_bURLEnterLock: 1;
    // Set in constructor, used in slotRunFinished
    /**
     * @brief Whether to call applyMainWindowSettings() when the first view is
     * created
     *
     * This will only be false if the main window is created with an empty URL
     */
    bool m_bNeedApplyKonqMainWindowSettings: 1;
    bool m_urlCompletionStarted: 1; //!< Whether an URL completion match is being done

    FullScreenManager *m_fullScreenManager; //!< The object which manages the full screen state

    /**
     * @brief The position in history the user chose to move to
     *
     * This is used by slotGoHistoryDelayed().
     */
    int m_goBuffer;

    /**
     * @brief The mouse buttons when the user activates one of the history-related actions
     *
     * This is used by slotGoHistoryDelayed().
     *
     * @todo Currently, this doesn't work because `QApplication::mouseButtons()`,
     * when called by the slot connected to the `triggered()` signal of a popup
     * menu action (for example, slotBackActivated()) always returns `Qt::NoButton`.
     */
    Qt::MouseButtons m_goMouseState;

    /**
     * @brief The keyboard modifiers when the user activates one of the history-related actions
     *
     * This is used by slotGoHistoryDelayed().
     */
    Qt::KeyboardModifiers m_goKeyboardState;

    MapViews m_mapViews; //!< The view associated to each part

    QPointer<KonqView> m_currentView; //!< The view which is visible and active

    /**
     * @brief An object representing the bookmarks menu
     *
     * Note that this is not a `QMenu`, but an object which mimics the
     * menu tree and contains the actions associated to each entry.
     */
    KBookmarkMenu *m_pBookmarkMenu;

    KonqExtendedBookmarkOwner *m_pBookmarksOwner; //!< The bookmark owner
    bool m_bookmarkBarInitialized; //!< Whether or not bookmarks have already been intialized

    KonqViewManager *m_pViewManager; //!< The object which manages all the views
    KonqFrameBase *m_pChildFrame; //!< The only frame child in the window. It actually is the tab widget

    int m_workingTab; //!< The index of the tab where the context menu is being shown

    // Store a number of things when opening a popup, they are needed
    // in the slots connected to the popup's actions.
    // TODO: a struct with new/delete to save a bit of memory?
    QString m_popupMimeType; //!< The mimetype of the items associated with the popup menu. It's the mimetype of the first of #m_popupItems
    QUrl m_popupUrl; //!< The URL associated with the popup menu. It's the URl of the first element of #m_popupItems
    KFileItemList m_popupItems; //!< The items associated with the popup menu
    KParts::OpenUrlArguments m_popupUrlArgs; //!< Information about how to open an URL in the popup menu
    BrowserArguments m_popupUrlBrowserArgs; //!< Other information about how to open an URL in the popup menu

    Konq::ConfigDialog *m_configureDialog; //!< The configuration dialog

    QLabel *m_locationLabel; //!< The location label
    QPointer<KonqCombo> m_combo; //!< The location bar

    /**
     * @brief The configuration object where combo box history is stored
     *
     * It corresponds to the `.config/konq_history` file.
     */
    static KConfig *s_comboConfig;

    KUrlCompletion *m_pURLCompletion; //!< Window specific completion object
    static KCompletion *s_pCompletion; //!< The global completion object. It actually is the same object as KonqHistoryManager completion object

    ToggleViewGUIClient *m_toggleViewGUIClient; //!< The object which manages toggle views such as the sidebar and the terminal emulator

    QString m_initialFrameName; //!< The name of the initial frame of the window

    QList<QAction *> m_openWithActions; //!< The "Open With" actions
    KActionMenu *m_openWithMenu; //!< The menu containing the "Open With" actions
    KActionMenu *m_viewModeMenu; //!< The menu containing the actions to change the part for the current URL
    QActionGroup *m_viewModesGroup; //!< Group for the different view modes
    QActionGroup *m_closedItemsGroup; //!< Group for the actions to restore closed items
    QActionGroup *m_sessionsGroup; //!< Group for the actions corresponding to saved sessions

    /**
     * @brief A list of all existing main window
     *
     * This is `nullptr` if no main window has yet been created.
     *
     * @note This includes preloaded windows
     */
    static QList<KonqMainWindow *> *s_lstMainWindows;

    QUrl m_currentDir; //!< The current directory, to be used for relative URLs whenever applicable

    QPointer<KonqHistoryDialog> m_historyDialog; //!< The history dialog

    /* The two variables below are used to store information about special popup
    * windows. These windows, mostly requested through javascript window.open API,
    * are required to have no toolbars showing. Since hiding all toolbars can lead
    * to a malicious site attempting to fool the user by mimicing native input dialogs,
    * (aka spoofing), Konqueror will NOT hide its location toolbar by default.
    */
    bool m_isPopupWithProxyWindow; //!< Whether or not this is a popup window
    QPointer<KonqMainWindow> m_popupProxyWindow; //!< The window which created the popup menu

    qint64 m_lastDeactivationTime = 0; //!< The last time the window was deactivated, stored as millisecond from epoch

    QString m_uuid; //!< Unique identifier for the window. Used for activities

    QString m_saveDir; //!< The starting directory for "Save As" dialogs when downloading files
    
    friend class KonqBrowserWindowInterface;
};


#endif // KONQMAINWINDOW_H
