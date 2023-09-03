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

#include <kfileitem.h>
#include <kparts/mainwindow.h>
#include <KParts/PartActivateEvent>
#include <kservice.h>

#include "konqcombo.h"
#include "konqframe.h"
#include "konqframecontainer.h"
#include "konqopenurlrequest.h"

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

namespace KParts
{
class BrowserExtension;
class ReadOnlyPart;
class OpenUrlArguments;
struct BrowserArguments;
}

class KonqExtendedBookmarkOwner;

class KONQ_TESTS_EXPORT KonqMainWindow : public KParts::MainWindow, public KonqFrameContainerBase
{
    Q_OBJECT
    Q_PROPERTY(int viewCount READ viewCount)
    Q_PROPERTY(int linkableViewsCount READ linkableViewsCount)
    Q_PROPERTY(QString locationBarURL READ locationBarURL)
    Q_PROPERTY(bool fullScreenMode READ fullScreenMode)
    Q_PROPERTY(QString currentTitle READ currentTitle)
    Q_PROPERTY(QString currentURL READ currentURL)
public:
    enum ComboAction { ComboClear, ComboAdd, ComboRemove };
    enum PageSecurity { NotCrypted, Encrypted, Mixed };

    explicit KonqMainWindow(const QUrl &initialURL = QUrl());
    ~KonqMainWindow() override;

    /**
     * Filters the URL and calls the main openUrl method.
     */
    void openFilteredUrl(const QString &url, const KonqOpenURLRequest &req);

    /**
     * Convenience overload for openFilteredUrl(url, req)
     */
    void openFilteredUrl(const QString &url, bool inNewTab = false, bool tempFile = false);

    /**
     * Convenience overload for openFilteredUrl(url, req)
     */
    void openFilteredUrl(const QString &_url, const QString &_mimeType, bool inNewTab, bool tempFile);

    void applyMainWindowSettings(const KConfigGroup &config) override;

    /**
     * It's not override since KMainWindow variant isn't virtual
     */
    void saveMainWindowSettings(KConfigGroup &config);

public Q_SLOTS:
    /**
    * The main openUrl method.
    */
    void openUrl(KonqView *view, const QUrl &url,
                 const QString &serviceType = QString(),
                 const KonqOpenURLRequest &req = KonqOpenURLRequest::null,
                 bool trustedSource = false); // trustedSource should be part of KonqOpenURLRequest, probably

public:
    /**
     * Called by openUrl when it knows the mime type (either directly,
     * or using KonqRun).
     * \param mimeType the mimetype of the URL to open. Always set.
     * \param url the URL to open.
     * \param childView the view in which to open the URL. Can be 0, in which
     * case a new tab (or the very first view) will be created.
     */
    bool openView(QString mimeType, const QUrl &url, KonqView *childView,
                  const KonqOpenURLRequest &req = KonqOpenURLRequest::null);

//     bool openView(const QString &mimetype, const QUrl &url, KService::Ptr service);

    void abortLoading();

    void openMultiURL(const QList<QUrl> &url);

    /// Returns the view manager for this window.
    KonqViewManager *viewManager() const
    {
        return m_pViewManager;
    }

    /// KXMLGUIBuilder methods, reimplemented for delayed bookmark-toolbar initialization
    QWidget *createContainer(QWidget *parent, int index, const QDomElement &element, QAction *&containerAction) override;
    void removeContainer(QWidget *container, QWidget *parent, QDomElement &element, QAction *containerAction) override;

    /// KMainWindow methods, for session management
    void saveProperties(KConfigGroup &config) override;
    void readProperties(const KConfigGroup &config) override;

    void setInitialFrameName(const QString &name);

    void reparseConfiguration();

    /// Called by KonqViewManager
    void insertChildView(KonqView *childView);
    /// Called by KonqViewManager
    void removeChildView(KonqView *childView);

    KonqView *childView(KParts::ReadOnlyPart *view);
    KonqView *childView(KParts::ReadOnlyPart *callingPart, const QString &name, KParts::ReadOnlyPart **part);

    // Total number of views
    int viewCount() const
    {
        return m_mapViews.count();
    }

    // Number of views not in "passive" mode and not locked
    int activeViewsNotLockedCount() const;

    // Number of views that can be linked, i.e. not with "follow active view" behavior
    int linkableViewsCount() const;

    // Number of main views (non-toggle non-passive views)
    int mainViewsCount() const;

    typedef QMap<KParts::ReadOnlyPart *, KonqView *> MapViews;

    const MapViews &viewMap() const
    {
        return m_mapViews;
    }

    KonqView *currentView() const;

    /** URL of current part, or URLs of selected items for directory views */
    QList<QUrl> currentURLs() const;

    // Only valid if there are one or two views
    KonqView *otherView(KonqView *view) const;

    /// Overloaded of KMainWindow
    void setCaption(const QString &caption) override;
    /// Overloaded of KMainWindow -- should never be called, or if it is, we ignore "modified" anyway
    void setCaption(const QString &caption, bool modified) override
    {
        Q_UNUSED(modified);
        setCaption(caption);
    }

    /**
    * Change URL displayed in the location bar
    */
    void setLocationBarURL(const QString &url);
    /**
    * Overload for convenience
    */
    void setLocationBarURL(const QUrl &url);
    /**
    * Return URL displayed in the location bar - for KonqViewManager
    */
    QString locationBarURL() const;
    void focusLocationBar();

    /**
    * Set page security related to current view
    */
    void setPageSecurity(PageSecurity);

    void enableAllActions(bool enable);

    void disableActionsNoView();

    void updateToolBarActions(bool pendingActions = false);
    void updateOpenWithActions();
    void updateViewActions();

    bool sidebarVisible() const;

    bool fullScreenMode() const;

    KonqView* createTabForLoadUrlRequest(const QUrl &url, const KonqOpenURLRequest &request);

    /**
    * @return the "link view" action, for checking/unchecking from KonqView
    */
    KToggleAction *linkViewAction()const
    {
        return m_paLinkView;
    }

    void enableAction(const char *name, bool enabled);
    void setActionText(const char *name, const QString &text);

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

    void linkableViewCountChanged();
    void viewCountChanged();

    // operates on all combos of all mainwindows of this instance
    // up to now adds an entry or clears all entries
    static void comboAction(int action, const QString &url,
                            const QString &senderId);

#ifndef NDEBUG
    void dumpViewList();
#endif

    // KonqFrameContainerBase implementation BEGIN

    bool accept(KonqFrameVisitor *visitor) override;

    /**
     * Insert a new frame as the mainwindow's child
     */
    void insertChildFrame(KonqFrameBase *frame, int index = -1) override;
    /**
     * Call this before deleting one of our children.
     */
    void childFrameRemoved(KonqFrameBase *frame) override;

    void saveConfig(KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options, KonqFrameBase *docContainer, int id = 0, int depth = 0) override;

    void copyHistory(KonqFrameBase *other) override;

    void setTitle(const QString &title, QWidget *sender) override;
    void setTabIcon(const QUrl &url, QWidget *sender) override;

    QWidget *asQWidget() override;

    KonqFrameBase::FrameType frameType() const override;

    KonqFrameBase *childFrame()const;

    void setActiveChild(KonqFrameBase *activeChild) override;

    // KonqFrameContainerBase implementation END

    /**
     * When using RMB on a tab, remember the tab we are showing a popup for.
     */
    void setWorkingTab(int index);

    static bool isMimeTypeAssociatedWithSelf(const QString &mimeType);
    static bool isMimeTypeAssociatedWithSelf(const QString &mimeType, const KService::Ptr &offer);

    bool refuseExecutingKonqueror(const QString &mimeType);

    QString currentTitle() const;
    // Not used by konqueror itself; only exists for the Q_PROPERTY,
    // which I guess is used by scripts and plugins...
    QString currentURL() const;

    void updateHistoryActions();

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

    // Public for unit tests
    void prepareForPopupMenu(const KFileItemList &items, const KParts::OpenUrlArguments &args, const KParts::BrowserArguments &browserArgs);

    /**
     * @brief The last time the window was deactivated
     *
     * @return the last time the window was deactivated, as milliseconds from Epoch. If the window was never deactivated, this is 0
     */
    qint64 lastDeactivationTime() const;

Q_SIGNALS:
    void viewAdded(KonqView *view);
    void viewRemoved(KonqView *view);
    void popupItemsDisturbed();
    void aboutToConfigure();

public Q_SLOTS:
    void updateViewModeActions();

    void activateTab(int index);

    void slotInternalViewModeChanged();

    void slotCtrlTabPressed();

    void slotPopupMenu(const QPoint &global, const KFileItemList &items, const KParts::OpenUrlArguments &args, const KParts::BrowserArguments &browserArgs, KParts::BrowserExtension::PopupFlags flags, const KParts::BrowserExtension::ActionGroupMap &);
    void slotPopupMenu(const QPoint &global, const QUrl &url, mode_t mode, const KParts::OpenUrlArguments &args, const KParts::BrowserArguments &browserArgs, KParts::BrowserExtension::PopupFlags f, const KParts::BrowserExtension::ActionGroupMap &);

    void slotOpenURLRequest(const QUrl &url, KonqOpenURLRequest &req);

    void openUrl(KonqView *childView, const QUrl &url, KonqOpenURLRequest &req);

    void slotCreateNewWindow(const QUrl &url, KonqOpenURLRequest& req,
                             const KParts::WindowArgs &windowArgs = KParts::WindowArgs(),
                             KParts::ReadOnlyPart **part = nullptr);

    void slotNewWindow();
    void slotDuplicateWindow();
    void slotSendURL();
    void slotSendFile();
    void slotCopyFiles();
    void slotMoveFiles();
    void slotOpenLocation();
    void slotOpenFile();

    // View menu
    void slotViewModeTriggered(QAction *action);
    void slotLockView();
    void slotLinkView();
    void slotReload(KonqView *view = nullptr, bool softReload = true);
    void slotForceReload();
    void slotStop();

    // Go menu
    void slotUp();
    void slotBack();
    void slotForward();
    void slotHome();
    void slotGoHistory();

    void slotAddClosedUrl(KonqFrameBase *tab);

    void slotConfigure(QString startingModule = QString());
    void slotConfigureDone();
    void slotConfigureToolbars();
    void slotConfigureExtensions();
    void slotConfigureSpellChecking();
    void slotNewToolbarConfig();

    void slotUndoAvailable(bool avail);

    void slotPartChanged(KonqView *childView, KParts::ReadOnlyPart *oldPart, KParts::ReadOnlyPart *newPart);

    void urlLoaderFinished(UrlLoader *loader);
    void slotClearLocationBar();

    // reimplement from KParts::MainWindow
    void slotSetStatusBarText(const QString &text) override;

    // public for KonqViewManager
    void slotPartActivated(KParts::Part *part);

    void slotGoHistoryActivated(int steps);

    void slotAddTab();
    void slotSplitViewHorizontal();
    void slotSplitViewVertical();
    void slotRemoveOtherTabs();
    void slotRemoveTabPopup();

private Q_SLOTS:
    void slotViewCompleted(KonqView *view);

    void slotURLEntered(const QString &text, Qt::KeyboardModifiers);

    void slotLocationLabelActivated();

    void slotDuplicateTab();
    void slotDuplicateTabPopup();

    void slotBreakOffTab();
    void slotBreakOffTabPopup();
    void breakOffTab(int);

    void slotPopupNewWindow();
    void slotPopupThisWindow();
    void slotPopupNewTab();
    void slotPopupPasteTo();
    void slotRemoveView();

    void slotRemoveOtherTabsPopup();

    void slotReloadPopup();
    void slotReloadAllTabs();
    void slotRemoveTab();
    void removeTab(int tabIndex);
    void removeOtherTabs(int tabIndex);

    void slotActivateNextTab();
    void slotActivatePrevTab();
    void slotActivateTab();

    void slotDumpDebugInfo();

    void slotOpenEmbedded(const KPluginMetaData &part);

    // Connected to KGlobalSettings
    void slotReconfigure();

    void slotForceSaveMainWindowSettings();

    void slotOpenWith();

    void updateProxyForWebEngine(bool updateProtocolManager = true);

#if 0
    void slotGoMenuAboutToShow();
#endif
    void slotUpAboutToShow();
    void slotBackAboutToShow();
    void slotForwardAboutToShow();

    void slotClosedItemsListAboutToShow();
    void updateClosedItemsAction();

    void slotSessionsListAboutToShow();
    void saveCurrentSession();
    void manageSessions();
    void slotSessionActivated(QAction *action);

    void slotUpActivated(QAction *action);
    void slotBackActivated(QAction *action);
    void slotForwardActivated(QAction *action);
    void slotHomePopupActivated(QAction *action);
    void slotGoHistoryDelayed();

    void slotCompletionModeChanged(KCompletion::CompletionMode);
    void slotMakeCompletion(const QString &);
    void slotSubstringcompletion(const QString &);
    void slotRotation(KCompletionBase::KeyBindingType);
    void slotMatch(const QString &);
    void slotClearHistory();
    void slotClearComboHistory();

    void slotClipboardDataChanged();
    void slotCheckComboSelection();

    void slotShowMenuBar();
    void slotShowStatusBar();

    void slotOpenURL(const QUrl &);

    void slotIconsChanged();

    bool event(QEvent *) override;

    void slotMoveTabLeft();
    void slotMoveTabRight();

    void slotAddWebSideBar(const QUrl &url, const QString &name);

    void slotUpdateFullScreen(bool set);   // do not call directly

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

    /**
    * Reimplemented for internal reasons. The API is not affected.
    */
    void showEvent(QShowEvent *event) override;

    bool makeViewsFollow(const QUrl &url,
                         const KParts::OpenUrlArguments &args,
                         const KParts::BrowserArguments &browserArgs, const QString &serviceType,
                         KonqView *senderView);

    void applyKonqMainWindowSettings();

    void viewsChanged();

    void updateLocalPropsActions();

    void closeEvent(QCloseEvent *) override;

    bool askForTarget(const KLocalizedString &text, QUrl &url);
    
private:
    
    enum class FullScreenState {
        NoFullScreen,
        OrdinaryFullScreen,
        CompleteFullScreen
    };
    
    struct FullScreenData {
        FullScreenState previousState;
        FullScreenState currentState;
        bool wasMenuBarVisible;
        bool wasStatusBarVisible;
        bool wasSidebarVisible;
        
        void switchToState(FullScreenState newState);
    };

private Q_SLOTS:
    void slotUndoTextChanged(const QString &newText);

    void slotIntro();
    void slotItemsRemoved(const KFileItemList &);
    /**
    * Loads the url displayed currently in the lineedit of the locationbar, by
    * emulating a enter key press event.
    */
    void goURL();

    void bookmarksIntoCompletion();

    void initBookmarkBar();

    void showPageSecurity();
    
    void toggleCompleteFullScreen(bool on);

    /**
     * Copies the "checkerEnabledByDefault" setting from the Sonnet configuration file
     * to Konqueror's own and emits the spellCheckConfigurationChanged signal
     */
    void updateSpellCheckConfiguration();

    void inspectCurrentPage();
    
private:
    void updateWindowIcon();

    QString detectNameFilter(QUrl &url);

    /**
    * takes care of hiding the bookmarkbar and calling setChecked( false ) on the
    * corresponding action
    */
    void updateBookmarkBar();

    /**
    * Adds all children of @p group to the static completion object
    */
    static void addBookmarksIntoCompletion(const KBookmarkGroup &group);

    /**
    * Returns all matches of the url-history for @p s. If there are no direct
    * matches, it will try completing with http:// prepended, and if there's
    * still no match, then http://www. Due to that, this is only usable for
    * popupcompletion and not for manual or auto-completion.
    */
    static QStringList historyPopupCompletionItems(const QString &s = QString());

    void startAnimation();
    void stopAnimation();

    void setUpEnabled(const QUrl &url);

    void checkDisableClearButton();
    void initCombo();
    void initActions();

    void popupNewTab(bool infront, bool openAfterCurrentPage);
    void addClosedWindowToUndoList();
    /**
    * Tries to find a index.html (.kde.html) file in the specified directory
    */
    static QString findIndexFile(const QString &directory);

    void connectExtension(KParts::BrowserExtension *ext);
    void disconnectExtension(KParts::BrowserExtension *ext);

    void plugViewModeActions();
    void unplugViewModeActions();

    void splitCurrentView(Qt::Orientation orientation);

    QObject *lastFrame(KonqView *view);

    QLineEdit *comboEdit();

    void saveGlobalProperties(KConfig * sessionConfig) override;

    void readGlobalProperties(KConfig * sessionConfig) override;
    
private: // members
    KonqUndoManager *m_pUndoManager;

    KNewFileMenu *m_pMenuNew;

    QAction *m_paPrint;

    KBookmarkActionMenu *m_pamBookmarks;

    KToolBarPopupAction *m_paUp;
    KToolBarPopupAction *m_paBack;
    KToolBarPopupAction *m_paForward;
    KToolBarPopupAction *m_paHomePopup;
    /// Action for the trash that contains closed tabs/windows
    KToolBarPopupAction *m_paClosedItems;
    KActionMenu *m_paSessions;
    QAction *m_paHome;

    QAction *m_paSplitViewHor;
    QAction *m_paSplitViewVer;
    QAction *m_paAddTab;
    QAction *m_paDuplicateTab;
    QAction *m_paBreakOffTab;
    QAction *m_paRemoveView;
    QAction *m_paRemoveTab;
    QAction *m_paRemoveOtherTabs;
    QAction *m_paActivateNextTab;
    QAction *m_paActivatePrevTab;

    KToggleAction *m_paLockView;
    KToggleAction *m_paLinkView;
    QAction *m_paReload;
    QAction *m_paForceReload;
    QAction *m_paReloadAllTabs;
    QAction *m_paUndo;
    QAction *m_paCut;
    QAction *m_paCopy;
    QAction *m_paPaste;
    QAction *m_paStop;

    QAction *m_paCopyFiles;
    QAction *m_paMoveFiles;

    QAction *m_paMoveTabLeft;
    QAction *m_paMoveTabRight;

    QAction *m_paConfigureExtensions;
    QAction *m_paConfigureSpellChecking;

    KonqAnimatedLogo *m_paAnimatedLogo;

    KBookmarkBar *m_paBookmarkBar;

#if 0
    KToggleAction *m_paFindFiles;
#endif

    KToggleAction *m_paShowMenuBar;
    KToggleAction *m_paShowStatusBar;

    KToggleFullScreenAction *m_ptaFullScreen;

    QAction *m_paShowDeveloperTools;

    bool m_fullyConstructed: 1;
    bool m_bLocationBarConnected: 1;
    bool m_bURLEnterLock: 1;
    // Set in constructor, used in slotRunFinished
    bool m_bNeedApplyKonqMainWindowSettings: 1;
    bool m_urlCompletionStarted: 1;

    FullScreenData m_fullScreenData;
    
    int m_goBuffer;
    Qt::MouseButtons m_goMouseState;
    Qt::KeyboardModifiers m_goKeyboardState;

    MapViews m_mapViews;

    QPointer<KonqView> m_currentView;

    KBookmarkMenu *m_pBookmarkMenu;
    KonqExtendedBookmarkOwner *m_pBookmarksOwner;
    KActionCollection *m_bookmarksActionCollection;
    bool m_bookmarkBarInitialized;

    KonqViewManager *m_pViewManager;
    KonqFrameBase *m_pChildFrame;

    // RMB on a tab: we need to store the tab number until the slots are called
    int m_workingTab;

    // Store a number of things when opening a popup, they are needed
    // in the slots connected to the popup's actions.
    // TODO: a struct with new/delete to save a bit of memory?
    QString m_popupMimeType;
    QUrl m_popupUrl;
    KFileItemList m_popupItems;
    KParts::OpenUrlArguments m_popupUrlArgs;
    KParts::BrowserArguments m_popupUrlBrowserArgs;

    KCMultiDialog *m_configureDialog;

    QLabel *m_locationLabel;
    QPointer<KonqCombo> m_combo;
    static KConfig *s_comboConfig;
    KUrlCompletion *m_pURLCompletion;
    // just a reference to KonqHistoryManager's completionObject
    static KCompletion *s_pCompletion;

    ToggleViewGUIClient *m_toggleViewGUIClient;

    QString m_initialFrameName;

    QList<QAction *> m_openWithActions;
    KActionMenu *m_openWithMenu;
    KActionMenu *m_viewModeMenu;
    QActionGroup *m_viewModesGroup;
    QActionGroup *m_closedItemsGroup;
    QActionGroup *m_sessionsGroup;

    static QList<KonqMainWindow *> *s_lstMainWindows;

    QUrl m_currentDir; // stores current dir for relative URLs whenever applicable

    QPointer<KonqHistoryDialog> m_historyDialog;

    /* The two variables below are used to store information about special popup
    * windows. These windows, mostly requested through javascript window.open API,
    * are required to have no toolbars showing. Since hiding all toolbars can lead
    * to a malicious site attempting to fool the user by mimicing native input dialogs,
    * (aka spoofing), Konqueror will NOT hide its location toolbar by default.
    */
    bool m_isPopupWithProxyWindow;
    QPointer<KonqMainWindow> m_popupProxyWindow;

    //The last time the window was deactivated, stored as millisecond from epoch
    qint64 m_lastDeactivationTime = 0;
    
    friend class KonqBrowserWindowInterface;
};

#endif // KONQMAINWINDOW_H
