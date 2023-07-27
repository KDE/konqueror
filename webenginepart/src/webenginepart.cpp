/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2007 Trolltech ASA
    SPDX-FileCopyrightText: 2008-2010 Urs Wolfer <uwolfer @ kde.org>
    SPDX-FileCopyrightText: 2008 Laurent Montel <montel@kde.org>
    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit@kde.org>
    SPDX-FileCopyrightText: 2013 Allan Sandfeld Jensen <sandfeld@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "webenginepart.h"
#include "webenginepartkiohandler.h"
#include "about/konq_aboutpage.h"

#include <webenginepart_debug.h>

//#include <QWebHistoryItem>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QUrlQuery>

#include "webenginepart_ext.h"
#include "webengineview.h"
#include "webenginepage.h"
#include "websslinfo.h"
#include "webhistoryinterface.h"
#include "webenginewallet.h"
#include "webengineparterrorschemehandler.h"
#include "webenginepartcookiejar.h"
#include "webengineurlrequestinterceptor.h"
#include "spellcheckermanager.h"
#include "webenginepartdownloadmanager.h"
#include "webenginepartcontrols.h"

#include "ui/searchbar.h"
#include "ui/passwordbar.h"
#include "ui/featurepermissionbar.h"
#include "settings/webenginesettings.h"
#include "ui/credentialsdetailswidget.h"

#include <kconfigwidgets_version.h>
#include <KCodecAction>
#include <KIO/Global>

#include <KActionCollection>
#include <KPluginMetaData>
#include <KUrlLabel>
#include <KMessageBox>
#include <KStringHandler>
#include <KAcceleratorManager>
#include <KFileItem>
#include <KMessageWidget>
#include <KProtocolInfo>
#include <KToggleAction>
#include <KParts/StatusBarExtension>
#include <KParts/GUIActivateEvent>
#include <KLocalizedString>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KSslInfoDialog>
#include <KProtocolManager>
#include <KParts/PartActivateEvent>
#include <KParts/BrowserInterface>
#include <KIO/ApplicationLauncherJob>

#include <QFile>
#include <QTextCodec>
#include <QCoreApplication>
#include <QVBoxLayout>
#include <QDBusInterface>
#include <QMenu>
#include <QStatusBar>
#include <QWebEngineScriptCollection>
#include <QDir>

#include "utils.h"
#include <kio_version.h>

WebEnginePart::WebEnginePart(QWidget *parentWidget, QObject *parent,
                         const KPluginMetaData& metaData,
                         const QByteArray& cachedHistory, const QStringList& /*args*/)
            :KParts::ReadOnlyPart(parent),
             m_emitOpenUrlNotify(true),
             m_walletData{false, false, false},
             m_doLoadFinishedActions(false),
             m_statusBarWalletLabel(nullptr),
             m_searchBar(nullptr),
             m_passwordBar(nullptr),
             m_wallet(nullptr)
{
    if (!WebEnginePartControls::self()->isReady()) {
        WebEnginePartControls::self()->setup(QWebEngineProfile::defaultProfile());
    }

    connect(WebEnginePartControls::self(), &WebEnginePartControls::userAgentChanged, this, &WebEnginePart::reloadAfterUAChange);

    setMetaData(metaData);

#if 0
    // NOTE: If the application does not set its version number, we automatically
    // set it to KDE's version number so that the default user-agent string contains
    // proper application version number information. See QWebEnginePage::userAgentForUrl...
    if (QCoreApplication::applicationVersion().isEmpty())
        QCoreApplication::setApplicationVersion(QString("%1.%2.%3")
                                                .arg(KDE::versionMajor())
                                                .arg(KDE::versionMinor())
                                                .arg(KDE::versionRelease()));
#endif
    setXMLFile(QL1S("webenginepart.rc"));

    // Create this KPart's widget
    QWidget *mainWidget = new QWidget (parentWidget);
    mainWidget->setObjectName(QStringLiteral("webenginepart"));

    // Create the WebEngineView...
    m_webView = new WebEngineView (this, parentWidget);

    // Create the browser extension.
    m_browserExtension = new WebEngineBrowserExtension(this, cachedHistory);

    // Add status bar extension...
    m_statusBarExtension = new KParts::StatusBarExtension(this);

    // Add a web history interface for storing visited links.
//    if (!QWebEngineHistoryInterface::defaultInterface())
//        QWebHistoryInterface::setDefaultInterface(new WebHistoryInterface(this));

    // Add text and html extensions...
    new WebEngineTextExtension(this);
    new WebEngineHtmlExtension(this);


    // Layout the GUI...
    QVBoxLayout* l = new QVBoxLayout(mainWidget);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);
    l->addWidget(m_webView);

    // Set the part's widget
    setWidget(mainWidget);

    // Set the web view as the focus object
    mainWidget->setFocusProxy(m_webView);

    // Connect the signals from the webview
    connect(m_webView, &QWebEngineView::titleChanged,
            this, &Part::setWindowCaption);
    connect(m_webView, &QWebEngineView::urlChanged,
            this, &WebEnginePart::slotUrlChanged);
//    connect(m_webView, SIGNAL(linkMiddleOrCtrlClicked(QUrl)),
//            this, SLOT(slotLinkMiddleOrCtrlClicked(QUrl)));
//    connect(m_webView, SIGNAL(selectionClipboardUrlPasted(QUrl,QString)),
//            this, SLOT(slotSelectionClipboardUrlPasted(QUrl,QString)));
    connect(m_webView, &QWebEngineView::loadFinished,
            this, &WebEnginePart::slotLoadFinished);

    // Init the QAction we are going to use...
    initActions();

    // Load plugins once we are fully ready
    setWallet(new WebEngineWallet(this, parentWidget ? parentWidget->window()->winId() : 0));

    setPage(page());
}

WebEnginePart::~WebEnginePart()
{
}

void WebEnginePart::setPage(WebEnginePage* newPage)
{
    WebEnginePage *oldPage = page();
    if (oldPage && oldPage != newPage) {
        m_webView->setPage(newPage);
        newPage->setParent(m_webView);
    }
    newPage->setPart(this);
    // Connect the signals from the page...
    connectWebEnginePageSignals(newPage);
}

WebEnginePage* WebEnginePart::page()
{
    if (m_webView)
        return qobject_cast<WebEnginePage*>(m_webView->page());
    return nullptr;
}

const WebEnginePage* WebEnginePart::page() const
{
    if (m_webView)
        return qobject_cast<const WebEnginePage*>(m_webView->page());
    return nullptr;
}

WebEnginePartDownloadManager * WebEnginePart::downloadManager()
{
    return WebEnginePartControls::self()->downloadManager();
}

SpellCheckerManager * WebEnginePart::spellCheckerManager()
{
    return WebEnginePartControls::self()->spellCheckerManager();
}

void WebEnginePart::initActions()
{
    QAction *action = actionCollection()->addAction(KStandardAction::SaveAs, QLatin1String("saveDocument"), m_browserExtension, &WebEngineBrowserExtension::slotSaveDocument);

    action = new QAction(QIcon::fromTheme(QStringLiteral("document-save-as")), i18n("Save Full HTML Page As..."), this);
    actionCollection()->addAction(QStringLiteral("saveFullHtmlPage"), action);
    connect(action, &QAction::triggered, m_browserExtension, &WebEngineBrowserExtension::slotSaveFullHTMLPage);

    action = new QAction(QIcon::fromTheme(QStringLiteral("document-print-preview")), i18n("Print Preview"), this);
    actionCollection()->addAction(QStringLiteral("printPreview"), action);
    connect(action, &QAction::triggered, m_browserExtension, &WebEngineBrowserExtension::slotPrintPreview);

    action = new QAction(QIcon::fromTheme(QStringLiteral("zoom-in")), i18nc("zoom in action", "Zoom In"), this);
    actionCollection()->addAction(QStringLiteral("zoomIn"), action);
    actionCollection()->setDefaultShortcuts(action, QList<QKeySequence> () << QKeySequence(QStringLiteral("CTRL++")) << QKeySequence(QStringLiteral("CTRL+=")));
    connect(action, &QAction::triggered, m_browserExtension, &WebEngineBrowserExtension::zoomIn);

    action = new QAction(QIcon::fromTheme(QStringLiteral("zoom-out")), i18nc("zoom out action", "Zoom Out"), this);
    actionCollection()->addAction(QStringLiteral("zoomOut"), action);
    actionCollection()->setDefaultShortcuts(action, QList<QKeySequence> () << QKeySequence(QStringLiteral("CTRL+-")) << QKeySequence(QStringLiteral("CTRL+_")));
    connect(action, &QAction::triggered, m_browserExtension, &WebEngineBrowserExtension::zoomOut);

    action = new QAction(QIcon::fromTheme(QStringLiteral("zoom-original")), i18nc("reset zoom action", "Actual Size"), this);
    actionCollection()->addAction(QStringLiteral("zoomNormal"), action);
    actionCollection()->setDefaultShortcut(action, QKeySequence(QStringLiteral("CTRL+0")));
    connect(action, &QAction::triggered, m_browserExtension, &WebEngineBrowserExtension::zoomNormal);

    action = new QAction(i18n("Zoom Text Only"), this);
    action->setCheckable(true);
    KConfigGroup cgHtml(KSharedConfig::openConfig(), "HTML Settings");
    bool zoomTextOnly = cgHtml.readEntry("ZoomTextOnly", false);
    action->setChecked(zoomTextOnly);
    actionCollection()->addAction(QStringLiteral("zoomTextOnly"), action);
    connect(action, &QAction::triggered, m_browserExtension, &WebEngineBrowserExtension::toogleZoomTextOnly);

    action = new QAction(i18n("Zoom To DPI"), this);
    action->setCheckable(true);
    bool zoomToDPI = cgHtml.readEntry("ZoomToDPI", false);
    action->setChecked(zoomToDPI);
    actionCollection()->addAction(QStringLiteral("zoomToDPI"), action);
    connect(action, &QAction::triggered, m_browserExtension, &WebEngineBrowserExtension::toogleZoomToDPI);


    action = KStandardAction::create(KStandardAction::SelectAll, m_browserExtension, &WebEngineBrowserExtension::slotSelectAll,
                                     actionCollection());
    action->setShortcutContext(Qt::WidgetShortcut);
    m_webView->addAction(action);

    KCodecAction *codecAction = new KCodecAction( QIcon::fromTheme(QStringLiteral("character-set")), i18n( "Set &Encoding" ), this, true );
    actionCollection()->addAction( QStringLiteral("setEncoding"), codecAction );
    connect(codecAction, &KCodecAction::codecTriggered, this, &WebEnginePart::slotSetTextEncoding);
    action = new QAction(i18n("View Do&cument Source"), this);
    actionCollection()->addAction(QStringLiteral("viewDocumentSource"), action);
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::Key_U));
    connect(action, &QAction::triggered, m_browserExtension, &WebEngineBrowserExtension::slotViewDocumentSource);

    action = new QAction(i18nc("Secure Sockets Layer", "SSL"), this);
    actionCollection()->addAction(QStringLiteral("security"), action);
    connect(action, &QAction::triggered, this, &WebEnginePart::slotShowSecurity);

    action = KStandardAction::create(KStandardAction::Find, this,
                                     &WebEnginePart::slotShowSearchBar, actionCollection());
    action->setWhatsThis(i18nc("find action \"whats this\" text", "<h3>Find text</h3>"
                              "Shows a dialog that allows you to find text on the displayed page."));

    createWalletActions();
}

void WebEnginePart::updateActions()
{
    m_browserExtension->updateActions();

    auto enableActionIf = [this](const QString &name, bool enable) {
        QAction* action = actionCollection()->action(name);
        if (action) {
            action->setEnabled(enable);
        }
    };

    const QString protocol (url().scheme());
    bool saveEnabled = protocol != QL1S("about") && protocol != QL1S("error") && protocol != QL1S("konq");

    enableActionIf(QL1S("saveDocument"), saveEnabled);
    enableActionIf(QL1S("saveFullHtmlPage"), saveEnabled);
    enableActionIf(QL1S("printPreview"), m_browserExtension->isActionEnabled("print"));
}

void WebEnginePart::connectWebEnginePageSignals(WebEnginePage* page)
{
    if (!page)
        return;

    connect(page, &QWebEnginePage::loadStarted, this, &WebEnginePart::slotLoadStarted);
    connect(page, &WebEnginePage::loadAborted, this, &WebEnginePart::slotLoadAborted);
    connect(page, &QWebEnginePage::linkHovered, this, &WebEnginePart::slotLinkHovered);
//    connect(page, SIGNAL(saveFrameStateRequested(QWebFrame*,QWebHistoryItem*)),
//            this, SLOT(slotSaveFrameState(QWebFrame*,QWebHistoryItem*)));
//    connect(page, SIGNAL(restoreFrameStateRequested(QWebFrame*)),
//            this, SLOT(slotRestoreFrameState(QWebFrame*)));
//    connect(page, SIGNAL(statusBarMessage(QString)),
//            this, SLOT(slotSetStatusBarText(QString)));
    connect(page, &QWebEnginePage::windowCloseRequested, this, &WebEnginePart::slotWindowCloseRequested);
//    connect(page, SIGNAL(printRequested(QWebFrame*)),
//            m_browserExtension, SLOT(slotPrintRequested(QWebFrame*)));
 //   connect(page, SIGNAL(frameCreated(QWebFrame*)),
 //           this, SLOT(slotFrameCreated(QWebFrame*)));

//    connect(m_webView, SIGNAL(linkShiftClicked(QUrl)),
//            page, SLOT(downloadUrl(QUrl)));

    connect(page, &QWebEnginePage::loadProgress, m_browserExtension, &KParts::BrowserExtension::loadingProgress);
    connect(page, &QWebEnginePage::selectionChanged, m_browserExtension, &WebEngineBrowserExtension::updateEditActions);
//    connect(m_browserExtension, SIGNAL(saveUrl(QUrl)),
//            page, SLOT(downloadUrl(QUrl)));

    connect(page, &QWebEnginePage::iconUrlChanged, [page, this](const QUrl& url) {
        if (WebEngineSettings::self()->favIconsEnabled()
            && !page->profile()->isOffTheRecord()){
                emit m_browserExtension->setIconUrl(url);
        }
    });
}

void WebEnginePart::setWallet(WebEngineWallet* wallet)
{
    if(m_wallet){
        disconnect(m_wallet, &WebEngineWallet::saveFormDataRequested,
                this, &WebEnginePart::slotSaveFormDataRequested);
        disconnect(m_wallet, &WebEngineWallet::fillFormRequestCompleted,
                this, &WebEnginePart::slotFillFormRequestCompleted);
        disconnect(m_wallet, &WebEngineWallet::walletClosed, this, &WebEnginePart::resetWallet);
        disconnect(m_wallet, &WebEngineWallet::formDetectionDone, this, &WebEnginePart::walletFinishedFormDetection);
        disconnect(m_wallet, &WebEngineWallet::saveFormDataCompleted, this, &WebEnginePart::slotWalletSavedForms);
        disconnect(m_wallet, &WebEngineWallet::walletOpened, this, &WebEnginePart::updateWalletActions);
    }
    m_wallet = wallet;
    if (m_wallet) {
        connect(m_wallet, &WebEngineWallet::saveFormDataRequested,
                this, &WebEnginePart::slotSaveFormDataRequested);
        connect(m_wallet, &WebEngineWallet::fillFormRequestCompleted,
                this, &WebEnginePart::slotFillFormRequestCompleted);
        connect(m_wallet, &WebEngineWallet::walletClosed, this, &WebEnginePart::resetWallet);
        connect(m_wallet, &WebEngineWallet::formDetectionDone, this, &WebEnginePart::walletFinishedFormDetection);
        connect(m_wallet, &WebEngineWallet::saveFormDataCompleted, this, &WebEnginePart::slotWalletSavedForms);
        connect(m_wallet, &WebEngineWallet::walletOpened, this, &WebEnginePart::updateWalletActions);
    }
}

WebEngineWallet* WebEnginePart::wallet() const
{
    return m_wallet;
}

void WebEnginePart::attemptInstallKIOSchemeHandler(const QUrl& url)
{
     if (KProtocolManager::defaultMimetype(url) == "text/html") { // man:, info:, etc.
        QWebEngineProfile *prof = QWebEngineProfile::defaultProfile();
        QByteArray scheme = url.scheme().toUtf8();
        //Qt complains about installing a scheme handler overriding the internal "about" scheme
        if (scheme != "about" && !prof->urlSchemeHandler(scheme)) {
            prof->installUrlSchemeHandler(scheme, new WebEnginePartKIOHandler(prof));
        }
    }

}

bool WebEnginePart::openUrl(const QUrl& _u)
{
    if (_u.isEmpty()) {
        return false;
    }

    QUrl u(_u);

    // If the URL given is a supported local protocol, e.g. "bookmark" but lacks
    // a path component, we set the path to "/" here so that the security context
    // will properly allow access to local resources.
    if (u.host().isEmpty() && u.path().isEmpty()
        && KProtocolInfo::protocolClass(u.scheme()) == QL1S(":local")) {
        u.setPath(QL1S("/"));
    }

    // Do not emit update history when url is typed in since the host
    // should handle that automatically itself.
    m_emitOpenUrlNotify = false;

    // Pointer to the page object...
    WebEnginePage* p = page();
    Q_ASSERT(p);

    KParts::BrowserArguments bargs (m_browserExtension->browserArguments());
    KParts::OpenUrlArguments args (arguments());

    if (!Utils::isBlankUrl(u)) {
        // Get the SSL information sent, if any...
        if (args.metaData().contains(QL1S("ssl_in_use"))) {
            WebSslInfo sslInfo;
            sslInfo.restoreFrom(KIO::MetaData(args.metaData()).toVariant());
            sslInfo.setUrl(u);
            p->setSslInfo(sslInfo);
        }
    }

    attemptInstallKIOSchemeHandler(u);

    // Set URL in KParts before emitting started; konq plugins rely on that.
    setUrl(u);
    m_doLoadFinishedActions = true;
    m_webView->loadUrl(u, args, bargs);
    return true;
}


bool WebEnginePart::closeUrl()
{
    m_webView->triggerPageAction(QWebEnginePage::Stop);
    m_webView->stop();
    return true;
}

QWebEngineView* WebEnginePart::view() const
{
    return m_webView;
}

bool WebEnginePart::isModified() const
{
    //return m_webView->isModified();
    return false;
}

// QWebEngineProfile * WebEnginePart::profile() const
// {
//     return WebEnginePartControls::self()->profile();
// }
//

void WebEnginePart::guiActivateEvent(KParts::GUIActivateEvent *event)
{
    if (event && event->activated() && m_webView) {
        emit setWindowCaption(m_webView->title());
    }
}

bool WebEnginePart::openFile()
{
    // never reached
    return false;
}


/// slots...

void WebEnginePart::slotLoadStarted()
{
    if(!Utils::isBlankUrl(url()) && url() != QUrl("konq:konqueror"))
    {
        emit started(nullptr);
    }
    updateActions();

    // If "NoEmitOpenUrlNotification" property is set to true, do not
    // emit the open url notification. Property is set by this part's
    // extension to prevent openUrl notification being sent when
    // handling history navigation requests (back/forward).
    const bool doNotEmitOpenUrl = property("NoEmitOpenUrlNotification").toBool();
    if (doNotEmitOpenUrl) {
        setProperty("NoEmitOpenUrlNotification", QVariant());
    } else {
        if (m_emitOpenUrlNotify) {
            emit m_browserExtension->openUrlNotify();
        }
    }
    // Unless we go via openUrl again, the next time we are here we emit (e.g. after clicking on a link)
    m_emitOpenUrlNotify = true;
}

void WebEnginePart::slotLoadFinished (bool ok)
{
    if (!ok || !m_doLoadFinishedActions)
        return;

    resetWallet();
    m_doLoadFinishedActions = false;

    // If the document contains no <title> tag, then set it to the current url.
    if (m_webView->title().trimmed().isEmpty()) {
        // If the document title is empty, then set it to the current url
        const QUrl url (m_webView->url());
        const QString caption (url.toString((QUrl::RemoveQuery|QUrl::RemoveFragment)));
        emit setWindowCaption(caption);

        // The urlChanged signal is emitted if and only if the main frame
        // receives the title of the page so we manually invoke the slot as a
        // work around here for pages that do not contain it, such as text
        // documents...
        slotUrlChanged(url);
    }

    if (m_wallet) {
        m_wallet->detectAndFillPageForms(page());
    }

    auto callback = [this](const QVariant &res) {
        bool hasRefresh = res.toBool();
        emit hasRefresh ? completedWithPendingAction() : completed();
    };
    page()->runJavaScript("hasRefreshAttribute()", QWebEngineScript::ApplicationWorld, callback);
    updateActions();
}

void WebEnginePart::slotLoadAborted(const QUrl & url)
{
    closeUrl();
    m_doLoadFinishedActions = false;
    if (url.isValid())
        emit m_browserExtension->openUrlRequest(url);
    else
        setUrl(m_webView->url());
}

void WebEnginePart::slotUrlChanged(const QUrl& url)
{
    // Ignore if empty
    if (url.isEmpty())
        return;

    // Ignore if error url
    if (url.scheme() == QL1S("error"))
        return;

    const QUrl u (url);

    // Ignore if url has not changed!
    if (this->url() == u)
      return;

    m_doLoadFinishedActions = true;
    setUrl(u);

    // Do not update the location bar with about:blank
    if (!Utils::isBlankUrl(url)) {
        //qCDebug(WEBENGINEPART_LOG) << "Setting location bar to" << u.prettyUrl() << "current URL:" << this->url();
        emit m_browserExtension->setLocationBarUrl(u.toDisplayString());
    }
}

void WebEnginePart::slotShowSecurity()
{
    if (!page())
        return;

    const WebSslInfo& sslInfo = page()->sslInfo();
    if (!sslInfo.isValid()) {
        KMessageBox::information(nullptr, i18n("The SSL information for this site "
                                    "appears to be corrupt."),
                            i18nc("Secure Sockets Layer", "SSL"));
        return;
    }

    KSslInfoDialog *dlg = new KSslInfoDialog (widget());
    dlg->setSslInfo(sslInfo.certificateChain(),
                    sslInfo.peerAddress().toString(),
                    url().host(),
                    sslInfo.protocol(),
                    sslInfo.ciphers(),
                    sslInfo.usedChiperBits(),
                    sslInfo.supportedChiperBits(),
                    KSslInfoDialog::certificateErrorsFromString(sslInfo.certificateErrors()));

    dlg->open();
}

#if 0
void WebEnginePart::slotSaveFrameState(QWebFrame *frame, QWebHistoryItem *item)
{
    if (!frame || !item) {
        return;
    }

    // Handle actions that apply only to the mainframe...
    if (frame == view()->page()->mainFrame()) {
    }

    // For some reason, QtWebEngine PORTING_TODO does not restore scroll position when
    // QWebHistory is restored from persistent storage. Therefore, we
    // preserve that information and restore it as needed. See
    // slotRestoreFrameState.
    const QPoint scrollPos (frame->scrollPosition());
    if (!scrollPos.isNull()) {
        // qCDebug(WEBENGINEPART_LOG) << "Saving scroll position:" << scrollPos;
        item->setUserData(scrollPos);
    }
}
#endif

#if 0
void WebEnginePart::slotRestoreFrameState(QWebFrame *frame)
{
    QWebEnginePage* page = (frame ? frame->page() : 0);
    QWebHistory* history = (page ? page->history() : 0);

    // No history item...
    if (!history || history->count() < 1)
        return;

    QWebHistoryItem currentHistoryItem (history->currentItem());

    // Update the scroll position if needed. See comment in slotSaveFrameState above.
    if (frame->baseUrl().resolved(frame->url()) == currentHistoryItem.url()) {
        const QPoint currentPos (frame->scrollPosition());
        const QPoint desiredPos (currentHistoryItem.userData().toPoint());
        if (currentPos.isNull() && !desiredPos.isNull()) {
            frame->setScrollPosition(desiredPos);
        }
    }
}
#endif

void WebEnginePart::slotLinkHovered(const QString& _link)
{
    QString message;

    if (_link.isEmpty()) {
        message = QL1S("");
        emit m_browserExtension->mouseOverInfo(KFileItem());
    } else {
        QUrl linkUrl (_link);
        const QString scheme = linkUrl.scheme();

        // Protect the user against URL spoofing!
        linkUrl.setUserName(QString());
        const QString link (linkUrl.toString());

        if (scheme == QL1S("mailto")) {
            message += i18nc("status bar text when hovering email links; looks like \"Email: xy@kde.org - CC: z@kde.org -BCC: x@kde.org - Subject: Hi translator\"", "Email: ");

            // Workaround: for QUrl's parsing deficiencies of "mailto:foo@bar.com".
            if (!linkUrl.hasQuery())
              linkUrl = QUrl(scheme + '?' + linkUrl.path());

            QMap<QString, QStringList> fields;
            QUrlQuery query(linkUrl);
            QList<QPair<QString, QString> > queryItems = query.queryItems();
            const int count = queryItems.count();

            for(int i = 0; i < count; ++i) {
                const QPair<QString, QString> queryItem (queryItems.at(i));
                //qCDebug(WEBENGINEPART_LOG) << "query: " << queryItem.first << queryItem.second;
                if (queryItem.first.contains(QL1C('@')) && queryItem.second.isEmpty())
                    fields[QStringLiteral("to")] << queryItem.first;
                if (QString::compare(queryItem.first, QL1S("to"), Qt::CaseInsensitive) == 0)
                    fields[QStringLiteral("to")] << queryItem.second;
                if (QString::compare(queryItem.first, QL1S("cc"), Qt::CaseInsensitive) == 0)
                    fields[QStringLiteral("cc")] << queryItem.second;
                if (QString::compare(queryItem.first, QL1S("bcc"), Qt::CaseInsensitive) == 0)
                    fields[QStringLiteral("bcc")] << queryItem.second;
                if (QString::compare(queryItem.first, QL1S("subject"), Qt::CaseInsensitive) == 0)
                    fields[QStringLiteral("subject")] << queryItem.second;
            }

            if (fields.contains(QL1S("to")))
                message += fields.value(QL1S("to")).join(QL1S(", "));
            if (fields.contains(QL1S("cc")))
                message += i18nc("status bar text when hovering email links; looks like \"Email: xy@kde.org - CC: z@kde.org -BCC: x@kde.org - Subject: Hi translator\"", " - CC: ") + fields.value(QL1S("cc")).join(QL1S(", "));
            if (fields.contains(QL1S("bcc")))
                message += i18nc("status bar text when hovering email links; looks like \"Email: xy@kde.org - CC: z@kde.org -BCC: x@kde.org - Subject: Hi translator\"", " - BCC: ") + fields.value(QL1S("bcc")).join(QL1S(", "));
            if (fields.contains(QL1S("subject")))
                message += i18nc("status bar text when hovering email links; looks like \"Email: xy@kde.org - CC: z@kde.org -BCC: x@kde.org - Subject: Hi translator\"", " - Subject: ") + fields.value(QL1S("subject")).join(QL1S(" "));
        } else if (scheme == QL1S("javascript")) {
            message = KStringHandler::rsqueeze(link, 150);
            if (link.startsWith(QL1S("javascript:window.open")))
                message += i18n(" (In new window)");
        } else {
            message = link;
#if 0
            QWebFrame* frame = page() ? page()->currentFrame() : 0;
            if (frame) {
                QWebHitTestResult result = frame->hitTestContent(page()->view()->mapFromGlobal(QCursor::pos()));
                QWebFrame* target = result.linkTargetFrame();
                if (frame->parentFrame() && target == frame->parentFrame()) {
                    message += i18n(" (In parent frame)");
                } else if (!target || target != frame) {
                    message += i18n(" (In new window)");
                }
            }
#endif
            KFileItem item (linkUrl, QString(), KFileItem::Unknown);
            emit m_browserExtension->mouseOverInfo(item);
        }
    }

    emit setStatusBarText(message);
}

void WebEnginePart::slotSearchForText(const QString &text, bool backward)
{
    QWebEnginePage::FindFlags flags; // = QWebEnginePage::FindWrapsAroundDocument;

    if (backward)
        flags |= QWebEnginePage::FindBackward;

    if (m_searchBar->caseSensitive())
        flags |= QWebEnginePage::FindCaseSensitively;

    //qCDebug(WEBENGINEPART_LOG) << "search for text:" << text << ", backward ?" << backward;
    page()->findText(text, flags, [this](bool found) {
        m_searchBar->setFoundMatch(found);
    });
}

void WebEnginePart::slotShowSearchBar()
{
    if (!m_searchBar) {
        // Create the search bar...
        m_searchBar = new SearchBar(widget());
        connect(m_searchBar, &SearchBar::searchTextChanged, this, &WebEnginePart::slotSearchForText);

        // This essentially duplicates the use of
        // KActionCollection::addAction(KStandardAction::StandardAction actionType, const QObject *receiver, const char *member)
        // with the Qt5 connect syntax.
        KStandardAction::create(KStandardAction::FindNext, m_searchBar, &SearchBar::findNext, actionCollection());
        KStandardAction::create(KStandardAction::FindPrev, m_searchBar, &SearchBar::findPrevious, actionCollection());

        QBoxLayout* lay = qobject_cast<QBoxLayout*>(widget()->layout());
        if (lay) {
          lay->addWidget(m_searchBar);
        }
    }
    const QString text = m_webView->selectedText();
    m_searchBar->setSearchText(text.left(150));
}

void WebEnginePart::slotLinkMiddleOrCtrlClicked(const QUrl& linkUrl)
{
    emit m_browserExtension->createNewWindow(linkUrl);
}

void WebEnginePart::slotSelectionClipboardUrlPasted(const QUrl& selectedUrl, const QString& searchText)
{
    if (!WebEngineSettings::self()->isOpenMiddleClickEnabled())
        return;

    if (!searchText.isEmpty() &&
        KMessageBox::questionTwoActions(m_webView,
                                   i18n("<qt>Do you want to search for <b>%1</b>?</qt>", searchText),
                                   i18n("Internet Search"), KGuiItem(i18n("&Search"), QStringLiteral("edit-find")),
                                   KStandardGuiItem::cancel(), QStringLiteral("MiddleClickSearch")) != KMessageBox::PrimaryAction)
        return;

    emit m_browserExtension->openUrlRequest(selectedUrl);
}

void WebEnginePart::deleteStatusBarWalletLabel()
{
    if (!m_statusBarWalletLabel) {
       return;
    }
    m_statusBarExtension->removeStatusBarItem(m_statusBarWalletLabel);
    delete m_statusBarWalletLabel;
    m_statusBarWalletLabel = nullptr;
}

void WebEnginePart::resetWallet()
{
    deleteStatusBarWalletLabel();
    updateWalletData({false, false, false});
    updateWalletActions();
}

void WebEnginePart::slotShowWalletMenu()
{
    QMenu *menu = new QMenu(nullptr);
    auto addAction = [this, menu](const QString &name) {
        QAction *a = actionCollection()->action(name);
        if (a->isEnabled()) {
            menu->addAction(a);
        }
    };
    addAction("walletFillFormsNow");
    addAction("walletCacheFormsNow");

    addAction("walletCustomizeFields");
    addAction("walletRemoveCustomization");
    menu->addSeparator();

    addAction("walletDisablePasswordCaching");
    addAction("walletRemoveCachedData");
    menu->addSeparator();

    addAction("walletShowManager");
    addAction("walletCloseWallet");

    KAcceleratorManager::manage(menu);
    menu->popup(QCursor::pos());
}

void WebEnginePart::slotLaunchWalletManager()
{
    const KService::Ptr kwalletManager = KService::serviceByDesktopName(QStringLiteral("org.kde.kwalletmanager5"));
    auto job = new KIO::ApplicationLauncherJob(kwalletManager);
    job->start();
}

void WebEnginePart::togglePasswordStorableState(bool on)
{
    if (!m_webView) {
        return;
    }
    QString host = m_webView->url().host();
    if (on) {
        WebEngineSettings::self()->removeNonPasswordStorableSite(host);
    } else {
        WebEngineSettings::self()->addNonPasswordStorableSite(host);
    }
    updateWalletActions();
    updateWalletStatusBarIcon();
}

void WebEnginePart::slotRemoveCachedPasswords()
{
    if (!m_wallet) {
        return;
    }

    m_wallet->removeFormData(page());
    updateWalletData(WalletData::HasCachedData, false);
}

void WebEnginePart::slotSetTextEncoding(QTextCodec * codec)
{
    // FIXME: The code below that sets the text encoding has been reported not to work.
    if (!page())
        return;

    QWebEngineSettings *localSettings = page()->settings();
    if (!localSettings)
        return;

    qCDebug(WEBENGINEPART_LOG) << "Encoding: new=>" << localSettings->defaultTextEncoding() << ", old=>" << codec->name();

    localSettings->setDefaultTextEncoding(codec->name());
    page()->triggerAction(QWebEnginePage::Reload);
}

void WebEnginePart::slotSetStatusBarText(const QString& text)
{
    const QString host (page() ? page()->url().host() : QString());
    if (WebEngineSettings::self()->windowStatusPolicy(host) == HtmlSettingsInterface::JSWindowStatusAllow)
        emit setStatusBarText(text);
}

void WebEnginePart::slotWindowCloseRequested()
{
    emit m_browserExtension->requestFocus(this);
#if 0
    if (KMessageBox::questionYesNo(m_webView,
                                   i18n("Close window?"), i18n("Confirmation Required"),
                                   KStandardGuiItem::close(), KStandardGuiItem::cancel())
        != KMessageBox::Yes)
        return;
#endif
    this->deleteLater();
}

void WebEnginePart::slotShowFeaturePermissionBar(const QUrl &origin, QWebEnginePage::Feature feature)
{
    auto findExistingBar = [origin, feature](FeaturePermissionBar *bar) {
        return bar->url() == origin && bar->feature() == feature;
    };
    auto found = std::find_if(m_permissionBars.constBegin(), m_permissionBars.constEnd(), findExistingBar);
    if (found != m_permissionBars.constEnd()) {
        return;
    }
    FeaturePermissionBar *bar = new FeaturePermissionBar(widget());
    m_permissionBars.append(bar);
    auto policyLambda = [this, bar](QWebEnginePage::Feature feature, QWebEnginePage::PermissionPolicy policy) {
        slotFeaturePolicyChosen(bar, feature, policy);
    };
    connect(bar, &FeaturePermissionBar::permissionPolicyChosen, this, policyLambda);
    connect(bar, &FeaturePermissionBar::done, this, [this, bar](){deleteFeaturePermissionBar(bar);});
    QBoxLayout* lay = qobject_cast<QBoxLayout*>(widget()->layout());
    if (lay) {
        lay->insertWidget(0, bar);
    }
    bar->setUrl(origin);
    bar->setFeature(feature);
    bar->animatedShow();
}

void WebEnginePart::slotFeaturePolicyChosen(FeaturePermissionBar* bar, QWebEnginePage::Feature feature, QWebEnginePage::PermissionPolicy policy)
{
    Q_ASSERT(bar && bar->feature() == feature);
    page()->setFeaturePermission(bar->url(), feature, policy);
}

void WebEnginePart::deleteFeaturePermissionBar(FeaturePermissionBar *bar)
{
    m_permissionBars.removeOne(bar);
    bar->deleteLater();
}

void WebEnginePart::slotSaveFormDataRequested (const QString& key, const QUrl& url)
{
    if (WebEngineSettings::self()->isNonPasswordStorableSite(url.host()))
        return;

    if (!WebEngineSettings::self()->askToSaveSitePassword())
        return;

    if (m_passwordBar && m_passwordBar->isVisible())
        return;

    if (!m_passwordBar) {
        m_passwordBar = new PasswordBar(widget());
        if (!m_wallet) {
            qCWarning(WEBENGINEPART_LOG) << "No m_wallet instance found! This should never happen!";
            return;
        }
        connect(m_passwordBar, &PasswordBar::saveFormDataAccepted,
                m_wallet, &WebEngineWallet::acceptSaveFormDataRequest);
        connect(m_passwordBar, &PasswordBar::saveFormDataRejected,
                m_wallet, &WebEngineWallet::rejectSaveFormDataRequest);
        connect(m_passwordBar, &PasswordBar::done, this, &WebEnginePart::slotSaveFormDataDone);
    }

    Q_ASSERT(m_passwordBar);

    m_passwordBar->setForms(m_wallet->pendingSaveData(key));
    m_passwordBar->setUrl(url);
    m_passwordBar->setRequestKey(key);
    m_passwordBar->setText(i18n("<html>Do you want %1 to remember the login "
                                "information for <b>%2</b>?</html>",
                                QCoreApplication::applicationName(),
                                url.host()));

    QBoxLayout* lay = qobject_cast<QBoxLayout*>(widget()->layout());
    if (lay) {
        lay->insertWidget(0, m_passwordBar);
    }

    m_passwordBar->animatedShow();
}

void WebEnginePart::slotSaveFormDataDone()
{
    if (!m_passwordBar)
        return;

    QBoxLayout* lay = qobject_cast<QBoxLayout*>(widget()->layout());
    if (lay)
        lay->removeWidget(m_passwordBar);
}

void WebEnginePart::updateWalletStatusBarIcon ()
{
    if (m_walletData.hasForms) {
        if (m_statusBarWalletLabel) {
            m_statusBarExtension->removeStatusBarItem(m_statusBarWalletLabel);
        } else {
            m_statusBarWalletLabel = new KUrlLabel(m_statusBarExtension->statusBar());
            m_statusBarWalletLabel->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum));
            m_statusBarWalletLabel->setUseCursor(false);
            connect(m_statusBarWalletLabel, QOverload<>::of(&KUrlLabel::leftClickedUrl), this, &WebEnginePart::slotLaunchWalletManager);
            connect(m_statusBarWalletLabel, QOverload<>::of(&KUrlLabel::rightClickedUrl), this, &WebEnginePart::slotShowWalletMenu);
        }
        QIcon icon = QIcon::fromTheme(m_walletData.hasCachedData ? QStringLiteral("wallet-open") : QStringLiteral("wallet-closed"));
        m_statusBarWalletLabel->setPixmap(icon.pixmap(QSize(16,16)));
        m_statusBarExtension->addStatusBarItem(m_statusBarWalletLabel, 0, false);
    } else if (m_statusBarWalletLabel) {
        deleteStatusBarWalletLabel();
    }
}

void WebEnginePart::slotFillFormRequestCompleted (bool ok)
{
    updateWalletData(WalletData::HasCachedData, ok);
}

void WebEnginePart::exitFullScreen()
{
    page()->triggerAction(QWebEnginePage::ExitFullScreen);
}

void WebEnginePart::walletFinishedFormDetection(const QUrl& url, bool found, bool autoFillableFound)
{
    if (page() && page()->url() == url) {
        updateWalletData({found, autoFillableFound});
        updateWalletActions();
        updateWalletStatusBarIcon();
    }
}

void WebEnginePart::slotWalletSavedForms(const QUrl& url, bool success)
{
    if (success && url == this->url()) {
        updateWalletData(WalletData::HasCachedData, true);
    }
}

void WebEnginePart::createWalletActions()
{
    QAction *a = new QAction(i18nc("Fill the Forms with Data from KWallet", "&Fill forms now"), this);
    actionCollection()->addAction("walletFillFormsNow", a);
    actionCollection()->setDefaultShortcut(a, QKeySequence("Ctrl+Shift+V"));
    connect(a, &QAction::triggered, this, [this]{if(page() && m_wallet){m_wallet->detectAndFillPageForms(page());}});

    a = new QAction(i18n("&Memorize Passwords in This Page Now"), this);
    actionCollection()->addAction("walletCacheFormsNow", a);
    connect(a, &QAction::triggered, this, [this]{if (page() && m_wallet){m_wallet->savePageDataNow(page());}});

    a = new QAction(i18n("&Customize Fields to Memorize for This Page..."), this);
    actionCollection()->addAction("walletCustomizeFields", a);
    connect(a, &QAction::triggered, this, [this](){if (m_wallet){m_wallet->customizeFieldsToCache(page(), view());}});

    a = new QAction(i18n("Remove Customized Memorization Settings for This Page"), this);
    actionCollection()->addAction("walletRemoveCustomization", a);
    connect(a, &QAction::triggered, this, [this](){m_wallet->removeCustomizationForPage(url());});

    KToggleAction *ta = new KToggleAction (i18n("&Allow Password Caching for This Site"), this);
    actionCollection()->addAction("walletDisablePasswordCaching", ta);
    connect(ta, &QAction::triggered, this, &WebEnginePart::togglePasswordStorableState);

    a = new QAction(i18n("Remove All Memorized Passwords for This Site"), this);
    actionCollection()->addAction("walletRemoveCachedData", a);
    connect(a, &QAction::triggered, this, &WebEnginePart::slotRemoveCachedPasswords);

    a = new QAction(i18n("&Launch Wallet Manager"), this);
    actionCollection()->addAction("walletShowManager", a);
    connect(a, &QAction::triggered, this, &WebEnginePart::slotLaunchWalletManager);

    a = new QAction(i18n("&Close Wallet"), this);
    actionCollection()->addAction("walletCloseWallet", a);
    connect(a, &QAction::triggered, this, &WebEnginePart::resetWallet);

    updateWalletActions();
}

void WebEnginePart::updateWalletActions()
{
    bool enableCaching = m_webView && !WebEngineSettings::self()->isNonPasswordStorableSite(m_webView->url().host());
    bool hasCustomForms = m_wallet && m_wallet->hasCustomizedCacheableForms(url());
    actionCollection()->action("walletFillFormsNow")->setEnabled(enableCaching && m_wallet && m_walletData.hasCachedData);
    actionCollection()->action("walletCacheFormsNow")->setEnabled(enableCaching && m_wallet && (m_walletData.hasAutoFillableForms || hasCustomForms));
    actionCollection()->action("walletCustomizeFields")->setEnabled(enableCaching && m_walletData.hasForms);
    actionCollection()->action("walletRemoveCustomization")->setEnabled(hasCustomForms);
    QAction *a = actionCollection()->action("walletDisablePasswordCaching");
    a->setChecked(enableCaching);
    a->setEnabled(m_walletData.hasForms);
    actionCollection()->action("walletRemoveCachedData")->setEnabled(m_walletData.hasCachedData);
    actionCollection()->action("walletCloseWallet")->setEnabled(m_wallet && m_wallet->isOpen());
}

void WebEnginePart::updateWalletData(WebEnginePart::WalletData::Member which, bool status)
{
    switch (which) {
        case WalletData::HasForms:
            m_walletData.hasForms = status;
            break;
        case WalletData::HasAutofillableForms:
            m_walletData.hasAutoFillableForms = status;
            break;
        case WalletData::HasCachedData:
            m_walletData.hasCachedData = status;
            break;
    }
    updateWalletActions();
    updateWalletStatusBarIcon();
}

void WebEnginePart::updateWalletData(std::initializer_list<bool> data)
{
    Q_ASSERT(data.size() > 0 && data.size() < 4);
    int size = data.size();
    auto it = data.begin();
    m_walletData.hasForms = it[0];
    if (size > 1) {
        m_walletData.hasAutoFillableForms = it[1];
    }
    if (size > 2) {
        m_walletData.hasAutoFillableForms = it[2];
    }
    updateWalletActions();
    updateWalletStatusBarIcon();
}

void WebEnginePart::setInspectedPart(KParts::ReadOnlyPart* part)
{
    WebEnginePart *wpart = qobject_cast<WebEnginePart*>(part);
    if (!wpart) {
        return;
    }
    page()->setInspectedPage(wpart->page());
    setUrl(page()->url());
}

void WebEnginePart::reloadAfterUAChange(const QString &)
{
    if (!page()) {
        return;
    }
    //NOTE: usually local files won't be affected by this, so don't automatically reload them
    if (!url().isLocalFile() && !url().isEmpty() && url().scheme() != "konq") {
        m_webView->triggerPageAction(QWebEnginePage::Reload);
    }
}
