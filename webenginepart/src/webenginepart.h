/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2007 Trolltech ASA
    SPDX-FileCopyrightText: 2008 Urs Wolfer <uwolfer @ kde.org>
    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/
#ifndef WEBENGINEPART_H
#define WEBENGINEPART_H

#include "kwebenginepartlib_export.h"

#include "browserextension.h"
#include "actondownloadedfilebar.h"

#include <QWebEnginePage>

#include <kparts_version.h>
#include <KParts/ReadOnlyPart>
#include <QUrl>
#include <QWebEngineScript>
#include <QPointer>

#include <KMessageWidget>

namespace KParts {
  class NavigationExtension;
  class StatusBarExtension;
}

class QWebEngineView;
class QWebEngineProfile;
class WebEngineView;
class WebEnginePage;
class SearchBar;
class PasswordBar;
class FeaturePermissionBar;
class KUrlLabel;
class WebEngineNavigationExtension;
class WebEngineWallet;
class KPluginMetaData;
class WebEnginePartControls;
class QTemporaryDir;

namespace KonqInterfaces {
    class DownloadJob;
}

namespace WebEngine {
    class ActOnDownloadedFileBar;
}

/**
 * A KPart wrapper for the QtWebEngine's browser rendering engine.
 *
 * This class attempts to provide the same type of integration into KPart
 * plugin applications, such as Konqueror, in much the same way as KHTML.
 *
 * Unlink the KHTML part however, access into the internals of the rendering
 * engine are provided through existing QtWebEngine class ; @see QWebEngineView.
 *
 */
class KWEBENGINEPARTLIB_EXPORT WebEnginePart : public KParts::ReadOnlyPart
{
    Q_OBJECT
    Q_PROPERTY( bool modified READ isModified )
public:
    explicit WebEnginePart(QWidget* parentWidget, QObject* parent,
                         const KPluginMetaData& metaData,
                         const QByteArray& cachedHistory = QByteArray(),
                         const QStringList& = QStringList());
    ~WebEnginePart() override;

    /**
     * Re-implemented for internal reasons. API remains unaffected.
     *
     * @see KParts::ReadOnlyPart::openUrl
     */
    bool openUrl(const QUrl &) override;

    /**
     * Actually loads the URL in the page
     */
    void loadUrl(const QUrl & _u);

    /**
     * Re-implemented for internal reasons. API remains unaffected.
     *
     * @see KParts::ReadOnlyPart::closeUrl
     */
    bool closeUrl() override;

    /**
     * Returns a pointer to the render widget used to display a web page.
     *
     * @see QWebEngineView.
     */
    QWebEngineView *view() const;

    /**
     * Checks whether the page contains unsubmitted form changes.
     *
     * @return @p true if form changes exist.
     */
    bool isModified() const;

    class SpellCheckerManager* spellCheckerManager();

    class WebEnginePartDownloadManager* downloadManager();

    /**
     * Connects the appropriate signals from the given page to the slots
     * in this class.
     */
    void connectWebEnginePageSignals(WebEnginePage* page);

    void slotShowFeaturePermissionBar(const QUrl &origin, QWebEnginePage::Feature);

    void setWallet(WebEngineWallet* wallet);

    WebEngineWallet* wallet() const;

    /**
     * @brief Changes the page object associated with the part
     *
     * This currently includes:
     * - calling WebEngineView::setPage
     * - making the part a child of the view
     * - connecting signals between part and page
     * - injecting scripts into the page
     * @param newPage the new page
     */
    void setPage(WebEnginePage *newPage);

    WebEnginePage* page();
    const WebEnginePage* page() const;
    BrowserExtension *browserExtension() const;

    /**
     * @brief The profile used by the part
     *
     * @note Currently all parts use the default profile, so this just returns KonqWebEnginePart::Profile::defaultProfile().
     * Using this function ensures that in the future this may be changed (for example to implement private profiles)
     *
     * @return the profile used by the part
     */
    QWebEngineProfile *profile() const;

    /**
     * @brief The default profile
     * @note Currently, all parts use the default profile, but this may change in the future
     * @return the default profile
     */
    static QWebEngineProfile *defaultProfile();

    bool isWebEnginePart() const {return true;}

    Q_PROPERTY(bool isWebEnginePart READ isWebEnginePart)

    static QTemporaryDir& temporaryDir();

    /**
     * @brief Tells this part to emit the `NavigationExtension::setLocationBarUrl()` signal
     * when the page starts loading rather than when it finishes loading
     *
     * Usually, the `NavigationExtension::setLocationBarUrl()` is emitted in response
     * to the `QWebEnginePage::urlChanged()` signal after the new URL has been loaded.
     * In some cases, however, we need the signal to be emitted earlier. For example,
     * when creating a new window or tab, waiting for the `urlChanged()` signal would
     * leave the location bar empty for a noticeable time. Calling this function with
     * a non-empty URL causes the `NavigationExtension::setLocationBarUrl()` signal to
     * be emitted in response to the `QWebEnginePage::loadStarted()` signal instead.
     *
     * This functionality is automatically reset when the page finishes loading
     * so there's no need to do it manually.
     *
     * @note This functionality doesn't work if the new URL is loaded using the history
     * javascript API, since when that happens, the page won't emit the `loadStarted()` signal.
     *
     * @param url the URL to use when emitting the `setLocationBarUrl()` early. Passing
     * an empty URL resets the usual behavior.
     */
    void setEarlyLocationBarUrl(const QUrl &url);

public Q_SLOTS:
    void exitFullScreen();
    void setInspectedPart(KParts::ReadOnlyPart *part);

    /**
     * @brief Displays a widget which the user can use to display or open a file he has just finished downloading
     */
    void displayActOnDownloadedFileBar(KonqInterfaces::DownloadJob *job);

    void displayPrintPreview();

protected:
    /**
     * Re-implemented for internal reasons. API remains unaffected.
     *
     * @see KParts::ReadOnlyPart::guiActivateEvent
     */
    void guiActivateEvent(KParts::GUIActivateEvent *) override;

    /**
     * Re-implemented for internal reasons. API remains unaffected.
     *
     * @see KParts::ReadOnlyPart::openFile
     */
    bool openFile() override;

    /**
     * @brief Connects the part with signals from KonqInterfaces::SpeedDial
     */
    void connectToSpeedDialSignals();

private Q_SLOTS:
    void slotShowSecurity();
    void slotShowSearchBar();
    void slotLoadStarted();
    void slotLoadAborted(const QUrl &);
    void slotLoadFinished(bool);
    void slotStartedNavigatingTo(const QUrl &newUrl);

    void slotSearchForText(const QString &text, bool backward);
    void slotLinkHovered(const QString &);
    //void slotSaveFrameState(QWebFrame *frame, QWebHistoryItem *item);
    //void slotRestoreFrameState(QWebFrame *frame);
    void slotLinkMiddleOrCtrlClicked(const QUrl&);
    void slotSelectionClipboardUrlPasted(const QUrl&, const QString&);

    /**
     * @brief Slot called in response to WebEnginePage::urlChanged signal
     *
     * It main task is to call setUrl and emit the setLocationBar signal. However, due to the fact that
     * WebEnginePage::loadStarted isn't always emitted (in particular, it isn't emitted when the history API
     * is used, see https://bugreports.qt.io/browse/QTBUG-115589) it will sometimes also call slotLoadStarted.
     *
     * @private
     * In particular, what this does is:
     * - check whether @p url is the URL for which WebEnignePage::acceptNavigationRequest was called, which
     *   is stored in #m_lastRequestedUrl
     * - if the two URLs are equal, it means that the WebEnginePage::loadStarted signal was emitted (it is always emitted
     *   when WebEngePage::acceptNavigationRequest is called), then it doesn't need to call slotLoadStarted
     * - if the two URLs are different, it means that WebEnginePage::loadStarted signal was not emitted, so it needs to
     *   call slotLoadStarted
     *
     * @param url the new URL
     * @see WebEngineNavigationExtension::saveState
     */
    void slotUrlChanged(const QUrl &url);
    void resetWallet();
    void slotShowWalletMenu();
    void slotLaunchWalletManager();
    void togglePasswordStorableState(bool on);
    void slotRemoveCachedPasswords();
    void slotSetTextEncoding(const QString &codecName);
    void slotSetStatusBarText(const QString& text);
    void slotWindowCloseRequested();
    void slotSaveFormDataRequested(const QString &, const QUrl &);
    void slotSaveFormDataDone();
    void slotWalletSavedForms(const QUrl &url, bool success);
    void slotFillFormRequestCompleted(bool);

    void slotFeaturePolicyChosen(FeaturePermissionBar *bar, QWebEnginePage::Feature feature, QWebEnginePage::PermissionPolicy policy);
    void deleteFeaturePermissionBar(FeaturePermissionBar *bar);

    void updateWalletStatusBarIcon();
    void walletFinishedFormDetection(const QUrl &url, bool found, bool autoFillableFound);
    void updateWalletActions();
    void reloadAfterUAChange(const QString &);

    /**
     * @brief Records the URL for which WebEnginePart::acceptNavigationRequest() was last called
     * @param page the page which emitted the signal (unused)
     * @param url the URL for which acceptNavigationRequest was called
     */
    void recordNavigationAccepted(WebEnginePage *page, const QUrl &url);

private:
    static void initWebEngineUrlSchemes();

    void deleteStatusBarWalletLabel();

    struct WalletData{
        enum Member{HasForms, HasAutofillableForms, HasCachedData};
        bool hasForms;
        bool hasAutoFillableForms;
        bool hasCachedData;
    };
    //Always use the following functions to change the values of m_walletData, as they automatically update the UI
    void updateWalletData(WalletData::Member which, bool status);
    void updateWalletData(std::initializer_list<bool> data);

    void attemptInstallKIOSchemeHandler(const QUrl &url);

    void initActions();
    void createWalletActions();
    void updateActions();

    bool m_emitOpenUrlNotify;

    /**
     * @brief Whether to have slotUrlChanged() always emit the `KParts::NavigationExtension::setLocationBarUrl` signal
     *
     * Usually, slotUrlChanged() will emit the signal only if the new URL and the original URL are different. However,
     * when loading a new page, the URL will have already been changed (from slotLoadStarted()) so that the new URL will
     * look the same as the old one. Setting `m_forceEmittingLocationBar` to `true` ensure that slotUrlChanged() will
     * emit the `setLocationBarUrl()` signal even if the two URLs are the same. slotUrlChanged() will then reset this
     * to `false`.
     *
     * @warning This should only be changed by slotLoadStarted() and slotUrlChanged()
     */
    bool m_forceEmittingLocationBar = false;

    WalletData m_walletData;
    bool m_doLoadFinishedActions;
    KUrlLabel* m_statusBarWalletLabel;
    SearchBar* m_searchBar;
    PasswordBar* m_passwordBar;
    QVector<FeaturePermissionBar*> m_permissionBars;
    WebEngineNavigationExtension* m_browserExtension;
    KParts::StatusBarExtension* m_statusBarExtension;
    WebEngineView* m_webView;
    WebEngineWallet* m_wallet;
    QUrl m_previousUrl;

    /**
     * @brief The URL to use when emitting the `NavigationExtension::setLocationBarUrl()` signal
     * when a page starts loading rather than when it finishes loading
     *
     * If empty, the `NavigationExtension::setLocationBarUrl()` signal will be emitted
     * after the page has loaded.
     *
     * @see setEarlyLocationBarUrl()
     */
    QUrl m_earlyLocationBarUrl;

    QPointer<WebEngine::ActOnDownloadedFileBar> m_actOnDownloadedFileWidget = nullptr; //!< Widget allowing the user to open a file which was downloaded right now

    /**
     * @brief The URL for which WebEnginePart::acceptNavigationRequest was last called
     *
     * This is used by slotUrlChanged to decide whether it needs to call slotLoadStarted or not
     */
    QUrl m_lastRequestedUrl;
};


#endif // WEBENGINEPART_H
