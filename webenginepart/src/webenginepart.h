/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2009 Dawit Alemayehu <adawit@kde.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef WEBENGINEPART_H
#define WEBENGINEPART_H

#include "kwebenginepartlib_export.h"

#include <QWebEnginePage>

#include <KParts/ReadOnlyPart>
#include <QUrl>

namespace KParts {
  class BrowserExtension;
  class StatusBarExtension;
}

class QWebEngineView;
class WebEngineView;
class WebEnginePage;
class SearchBar;
class PasswordBar;
class FeaturePermissionBar;
class KUrlLabel;
class WebEngineBrowserExtension;
class WebEngineWallet;

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
    explicit WebEnginePart(QWidget* parentWidget = nullptr, QObject* parent = nullptr,
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

    /**
     * Connects the appropriate signals from the given page to the slots
     * in this class.
     */
    void connectWebEnginePageSignals(WebEnginePage* page);

    void slotShowFeaturePermissionBar(const QUrl &origin, QWebEnginePage::Feature);

    void setWallet(WebEngineWallet* wallet);

public Q_SLOTS:
    void exitFullScreen();

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

private:
    WebEnginePage* page();
    const WebEnginePage* page() const;
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
    WebEngineBrowserExtension* m_browserExtension;
    KParts::StatusBarExtension* m_statusBarExtension;
    WebEngineView* m_webView;
    WebEngineWallet* m_wallet;
};

#endif // WEBENGINEPART_H
