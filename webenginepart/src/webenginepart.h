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

#include <QWebEnginePage>

#include <kparts_version.h>
#include <KParts/ReadOnlyPart>
#include <QUrl>
#include <QWebEngineScript>

namespace KParts {
  class BrowserExtension;
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
class WebEngineBrowserExtension;
class WebEngineWallet;
class KPluginMetaData;
class WebEnginePartControls;

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

    QWebEngineProfile *profile() const;

    bool isWebEnginePart() const {return true;}

    Q_PROPERTY(bool isWebEnginePart READ isWebEnginePart)

public Q_SLOTS:
    void exitFullScreen();
    void setInspectedPart(KParts::ReadOnlyPart *part);

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

private Q_SLOTS:
    void slotShowSecurity();
    void slotShowSearchBar();
    void slotLoadStarted();
    void slotLoadAborted(const QUrl &);
    void slotLoadFinished(bool);

    void slotSearchForText(const QString &text, bool backward);
    void slotLinkHovered(const QString &);
    //void slotSaveFrameState(QWebFrame *frame, QWebHistoryItem *item);
    //void slotRestoreFrameState(QWebFrame *frame);
    void slotLinkMiddleOrCtrlClicked(const QUrl&);
    void slotSelectionClipboardUrlPasted(const QUrl&, const QString&);

    void slotUrlChanged(const QUrl &);
    void resetWallet();
    void slotShowWalletMenu();
    void slotLaunchWalletManager();
    void togglePasswordStorableState(bool on);
    void slotRemoveCachedPasswords();
    void slotSetTextEncoding(QTextCodec*);
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

    WalletData m_walletData;
    bool m_doLoadFinishedActions;
    KUrlLabel* m_statusBarWalletLabel;
    SearchBar* m_searchBar;
    PasswordBar* m_passwordBar;
    QVector<FeaturePermissionBar*> m_permissionBars;
    WebEngineBrowserExtension* m_browserExtension;
    KParts::StatusBarExtension* m_statusBarExtension;
    WebEngineView* m_webView;
    WebEngineWallet* m_wallet;
};

#endif // WEBENGINEPART_H
