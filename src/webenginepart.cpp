/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
 * Copyright (C) 2008 - 2010 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
 * Copyright (C) 2009 Dawit Alemayehu <adawit@kde.org>
 * Copyright (C) 2013 Allan Sandfeld Jensen <sandfeld@kde.org>
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

#include "webenginepart.h"

//#include <QWebHistoryItem>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QUrlQuery>

#include "webenginepart_ext.h"
#include "sslinfodialog_p.h"
#include "webview.h"
#include "webpage.h"
#include "websslinfo.h"
#include "webhistoryinterface.h"

#include "ui/searchbar.h"
#include "ui/passwordbar.h"
#include "ui/featurepermissionbar.h"
#include "settings/webenginesettings.h"

#include <kcodecaction.h>
#include <kio/global.h>

#include <KActionCollection>
#include <KAboutData>
#include <KUrlLabel>
#include <KMessageBox>
#include <KStringHandler>
#include <KToolInvocation>
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

#include <QUrl>
#include <QFile>
#include <QTextCodec>
#include <QCoreApplication>
#include <QVBoxLayout>
#include <QDBusInterface>
#include <QMenu>
#include <QStatusBar>

#define QL1S(x)  QLatin1String(x)
#define QL1C(x)  QLatin1Char(x)

Q_GLOBAL_STATIC_WITH_ARGS(QUrl, globalBlankUrl, ("about:blank"))

WebEnginePart::WebEnginePart(QWidget *parentWidget, QObject *parent,
                         const QByteArray& cachedHistory, const QStringList& /*args*/)
            :KParts::ReadOnlyPart(parent),
             m_emitOpenUrlNotify(true),
             m_hasCachedFormData(false),
             m_doLoadFinishedActions(false),
             m_statusBarWalletLabel(0),
             m_searchBar(0),
             m_passwordBar(0),
             m_featurePermissionBar(0)
{
    KAboutData about = KAboutData("webenginepart",
                                  i18nc("Program Name", "WebEnginePart"),
                                  /*version*/ "1.3.0",
                                  i18nc("Short Description", "QtWebEngine Browser Engine Component"),
                                  KAboutLicense::LGPL,
                                  i18n("(C) 2009-2010 Dawit Alemayehu\n"
                                        "(C) 2008-2010 Urs Wolfer\n"
                                        "(C) 2007 Trolltech ASA"));

    about.addAuthor(i18n("Sune Vuorela"), i18n("Maintainer, Developer"), "sune@kde.org");
    about.addAuthor(i18n("Dawit Alemayehu"), i18n("Developer"), "adawit@kde.org");
    about.addAuthor(i18n("Urs Wolfer"), i18n("Maintainer, Developer"), "uwolfer@kde.org");
    about.addAuthor(i18n("Michael Howell"), i18n("Developer"), "mhowell123@gmail.com");
    about.addAuthor(i18n("Laurent Montel"), i18n("Developer"), "montel@kde.org");
    about.addAuthor(i18n("Dirk Mueller"), i18n("Developer"), "mueller@kde.org");
    about.setProductName("webenginepart/general");
//    KComponentData componentData(&about);
    setComponentData(about, false /*don't load plugins yet*/);

#if 0
    // NOTE: If the application does not set its version number, we automatically
    // set it to KDE's version number so that the default user-agent string contains
    // proper application version number information. See QWebPage::userAgentForUrl...
    if (QCoreApplication::applicationVersion().isEmpty())
        QCoreApplication::setApplicationVersion(QString("%1.%2.%3")
                                                .arg(KDE::versionMajor())
                                                .arg(KDE::versionMinor())
                                                .arg(KDE::versionRelease()));
#endif
    setXMLFile(QL1S("webenginepart.rc"));

    // Create this KPart's widget
    QWidget *mainWidget = new QWidget (parentWidget);
    mainWidget->setObjectName("webenginepart");

    // Create the WebView...
    m_webView = new WebView (this, parentWidget);

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
    new WebEngineScriptableExtension(this);


    // Layout the GUI...
    QVBoxLayout* l = new QVBoxLayout(mainWidget);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);
    l->addWidget(m_webView);
    mainWidget->setLayout(l);

    // Set the part's widget
    setWidget(mainWidget);

    // Set the web view as the the focus object
    mainWidget->setFocusProxy(m_webView);

    // Connect the signals from the webview
    connect(m_webView, SIGNAL(titleChanged(QString)),
            this, SIGNAL(setWindowCaption(QString)));
    connect(m_webView, SIGNAL(urlChanged(QUrl)),
            this, SLOT(slotUrlChanged(QUrl)));
//    connect(m_webView, SIGNAL(linkMiddleOrCtrlClicked(QUrl)),
//            this, SLOT(slotLinkMiddleOrCtrlClicked(QUrl)));
//    connect(m_webView, SIGNAL(selectionClipboardUrlPasted(QUrl,QString)),
//            this, SLOT(slotSelectionClipboardUrlPasted(QUrl,QString)));
    connect(m_webView, SIGNAL(loadFinished(bool)),
            this, SLOT(slotLoadFinished(bool)));

    // Connect the signals from the page...
    connectWebPageSignals(page());

    // Init the QAction we are going to use...
    initActions();

    // Load plugins once we are fully ready
    loadPlugins();
}

WebEnginePart::~WebEnginePart()
{
}

WebPage* WebEnginePart::page()
{
    if (m_webView)
        return qobject_cast<WebPage*>(m_webView->page());
    return 0;
}

const WebPage* WebEnginePart::page() const
{
    if (m_webView)
        return qobject_cast<const WebPage*>(m_webView->page());
    return 0;
}

void WebEnginePart::initActions()
{
    actionCollection()->addAction(KStandardAction::SaveAs, "saveDocument",
                                  m_browserExtension, SLOT(slotSaveDocument()));

    QAction* action = new QAction(i18n("Save &Frame As..."), this);
    actionCollection()->addAction("saveFrame", action);
    connect(action, SIGNAL(triggered(bool)), m_browserExtension, SLOT(slotSaveFrame()));

    action = new QAction(QIcon::fromTheme("document-print-preview"), i18n("Print Preview"), this);
    actionCollection()->addAction("printPreview", action);
    connect(action, SIGNAL(triggered(bool)), m_browserExtension, SLOT(slotPrintPreview()));

    action = new QAction(QIcon::fromTheme("zoom-in"), i18nc("zoom in action", "Zoom In"), this);
    actionCollection()->addAction("zoomIn", action);
    action->setShortcut(QKeySequence("CTRL++; CTRL+="));
    connect(action, SIGNAL(triggered(bool)), m_browserExtension, SLOT(zoomIn()));

    action = new QAction(QIcon::fromTheme("zoom-out"), i18nc("zoom out action", "Zoom Out"), this);
    actionCollection()->addAction("zoomOut", action);
    action->setShortcut(QKeySequence("CTRL+-; CTRL+_"));
    connect(action, SIGNAL(triggered(bool)), m_browserExtension, SLOT(zoomOut()));

    action = new QAction(QIcon::fromTheme("zoom-original"), i18nc("reset zoom action", "Actual Size"), this);
    actionCollection()->addAction("zoomNormal", action);
    action->setShortcut(QKeySequence("CTRL+0"));
    connect(action, SIGNAL(triggered(bool)), m_browserExtension, SLOT(zoomNormal()));

    action = new QAction(i18n("Zoom Text Only"), this);
    action->setCheckable(true);
    KConfigGroup cgHtml(KSharedConfig::openConfig(), "HTML Settings");
    bool zoomTextOnly = cgHtml.readEntry("ZoomTextOnly", false);
    action->setChecked(zoomTextOnly);
    actionCollection()->addAction("zoomTextOnly", action);
    connect(action, SIGNAL(triggered(bool)), m_browserExtension, SLOT(toogleZoomTextOnly()));

    action = new QAction(i18n("Zoom To DPI"), this);
    action->setCheckable(true);
    bool zoomToDPI = cgHtml.readEntry("ZoomToDPI", false);
    action->setChecked(zoomToDPI);
    actionCollection()->addAction("zoomToDPI", action);
    connect(action, SIGNAL(triggered(bool)), m_browserExtension, SLOT(toogleZoomToDPI()));

    action = actionCollection()->addAction(KStandardAction::SelectAll, "selectAll",
                                           m_browserExtension, SLOT(slotSelectAll()));
    action->setShortcutContext(Qt::WidgetShortcut);
    m_webView->addAction(action);

    KCodecAction *codecAction = new KCodecAction( QIcon::fromTheme("character-set"), i18n( "Set &Encoding" ), this, true );
    actionCollection()->addAction( "setEncoding", codecAction );
    connect(codecAction, SIGNAL(triggered(QTextCodec*)), SLOT(slotSetTextEncoding(QTextCodec*)));

    action = new QAction(i18n("View Do&cument Source"), this);
    actionCollection()->addAction("viewDocumentSource", action);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_U));
    connect(action, SIGNAL(triggered(bool)), m_browserExtension, SLOT(slotViewDocumentSource()));

    action = new QAction(i18nc("Secure Sockets Layer", "SSL"), this);
    actionCollection()->addAction("security", action);
    connect(action, SIGNAL(triggered(bool)), SLOT(slotShowSecurity()));

    action = actionCollection()->addAction(KStandardAction::Find, "find", this, SLOT(slotShowSearchBar()));
    action->setWhatsThis(i18nc("find action \"whats this\" text", "<h3>Find text</h3>"
                              "Shows a dialog that allows you to find text on the displayed page."));
}

void WebEnginePart::updateActions()
{
    m_browserExtension->updateActions();

    QAction* action = actionCollection()->action(QL1S("saveDocument"));
    if (action) {
        const QString protocol (url().scheme());
        action->setEnabled(protocol != QL1S("about") && protocol != QL1S("error"));
    }

    action = actionCollection()->action(QL1S("printPreview"));
    if (action) {
        action->setEnabled(m_browserExtension->isActionEnabled("print"));
    }

}

void WebEnginePart::connectWebPageSignals(WebPage* page)
{
    if (!page)
        return;

    connect(page, SIGNAL(loadStarted()),
            this, SLOT(slotLoadStarted()));
    connect(page, SIGNAL(loadAborted(QUrl)),
            this, SLOT(slotLoadAborted(QUrl)));
    connect(page, &QWebEnginePage::linkHovered,
            this, &WebEnginePart::slotLinkHovered);
//    connect(page, SIGNAL(saveFrameStateRequested(QWebFrame*,QWebHistoryItem*)),
//            this, SLOT(slotSaveFrameState(QWebFrame*,QWebHistoryItem*)));
//    connect(page, SIGNAL(restoreFrameStateRequested(QWebFrame*)),
//            this, SLOT(slotRestoreFrameState(QWebFrame*)));
//    connect(page, SIGNAL(statusBarMessage(QString)),
//            this, SLOT(slotSetStatusBarText(QString)));
    connect(page, SIGNAL(windowCloseRequested()),
            this, SLOT(slotWindowCloseRequested()));
//    connect(page, SIGNAL(printRequested(QWebFrame*)),
//            m_browserExtension, SLOT(slotPrintRequested(QWebFrame*)));
 //   connect(page, SIGNAL(frameCreated(QWebFrame*)),
 //           this, SLOT(slotFrameCreated(QWebFrame*)));

//    connect(m_webView, SIGNAL(linkShiftClicked(QUrl)),
//            page, SLOT(downloadUrl(QUrl)));

    connect(page, SIGNAL(loadProgress(int)),
            m_browserExtension, SIGNAL(loadingProgress(int)));
    connect(page, SIGNAL(selectionChanged()),
            m_browserExtension, SLOT(updateEditActions()));
//    connect(m_browserExtension, SIGNAL(saveUrl(QUrl)),
//            page, SLOT(downloadUrl(QUrl)));

//    connect(page->mainFrame(), SIGNAL(loadFinished(bool)),
//            this, SLOT(slotMainFrameLoadFinished(bool)));


    connect(page, &QWebEnginePage::iconUrlChanged, [page, this](const QUrl& url) {
        if (WebEngineSettings::self()->favIconsEnabled()
            && !page->profile()->isOffTheRecord()){
                m_browserExtension->setIconUrl(url);
        }
    });

#if 0
    KWebWallet *wallet = page->wallet();
    if (wallet) {
        connect(wallet, SIGNAL(saveFormDataRequested(QString,QUrl)),
                this, SLOT(slotSaveFormDataRequested(QString,QUrl)));
        connect(wallet, SIGNAL(fillFormRequestCompleted(bool)),
                this, SLOT(slotFillFormRequestCompleted(bool)));
        connect(wallet, SIGNAL(walletClosed()), this, SLOT(slotWalletClosed()));
    }
#endif
}

bool WebEnginePart::openUrl(const QUrl &_u)
{
    QUrl u (_u);

    qDebug() << u;

    // Ignore empty requests...
    if (u.isEmpty())
        return false;

    // If the URL given is a supported local protocol, e.g. "bookmark" but lacks
    // a path component, we set the path to "/" here so that the security context
    // will properly allow access to local resources.
    if (u.host().isEmpty() && u.path().isEmpty()
        && KProtocolInfo::protocolClass(u.scheme()) == QL1S(":local")) {
        u.setPath(QL1S("/"));
    }

    // Do not emit update history when url is typed in since the embedding part
    // should handle that automatically itself. At least Konqueror does that.
    m_emitOpenUrlNotify = false;

    // Pointer to the page object...
    WebPage* p = page();
    Q_ASSERT(p);

    KParts::BrowserArguments bargs (m_browserExtension->browserArguments());
    KParts::OpenUrlArguments args (arguments());

    if (u != *globalBlankUrl) {
        // Get the SSL information sent, if any...
        if (args.metaData().contains(QL1S("ssl_in_use"))) {
            WebSslInfo sslInfo;
            sslInfo.restoreFrom(KIO::MetaData(args.metaData()).toVariant());
            sslInfo.setUrl(u);
            p->setSslInfo(sslInfo);
        }
    }

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

QWebEngineView* WebEnginePart::view()
{
    return m_webView;
}

bool WebEnginePart::isModified() const
{
    //return m_webView->isModified();
    return false;
}

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
    emit started(0);
    updateActions();
}

void WebEnginePart::slotMainFrameLoadFinished (bool ok)
{
    if (!ok || !m_doLoadFinishedActions)
        return;

    m_doLoadFinishedActions = false;

    if (!m_emitOpenUrlNotify) {
        m_emitOpenUrlNotify = true; // Save history once page loading is done.
    }

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
    if (url() != *globalBlankUrl) {
        m_hasCachedFormData = false;

        if (WebEngineSettings::self()->isNonPasswordStorableSite(url().host())) {
            addWalletStatusBarIcon();
        } else {
// Attempt to fill the web form...
//          KWebWallet *webWallet = page() ? page()->wallet() : 0;
//          if (webWallet) {
//              webWallet->fillFormData(frame, false);
//          }
        }
    }
}

void WebEnginePart::slotLoadFinished(bool ok)
{
    bool pending = false;

    if (m_doLoadFinishedActions) {
        updateActions();
       // QWebFrame* frame = (page() ? page()->currentFrame() : 0);
       // if (ok &&
       //     frame == page()->mainFrame() &&
       //     !frame->findFirstElement(QL1S("head>meta[http-equiv=refresh]")).isNull()) {
       //     if (WebEngineSettings::self()->autoPageRefresh()) {
       //         pending = true;
       //     } else {
       //         frame->page()->triggerAction(QWebEnginePage::Stop);
       //     }
       // }
    }

    emit completed ((ok && pending));
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
    if (url.scheme().compare(QL1S("error"), Qt::CaseInsensitive) == 0)
        return;

    const QUrl u (url);

    // Ignore if url has not changed!
    if (this->url() == u)
      return;

    m_doLoadFinishedActions = true;
    setUrl(u);

    // Do not update the location bar with about:blank
    if (url != *globalBlankUrl) {
        //kDebug() << "Setting location bar to" << u.prettyUrl() << "current URL:" << this->url();
        emit m_browserExtension->setLocationBarUrl(u.toDisplayString());
    }
}

void WebEnginePart::slotShowSecurity()
{
    if (!page())
        return;

    const WebSslInfo& sslInfo = page()->sslInfo();
    if (!sslInfo.isValid()) {
        KMessageBox::information(0, i18n("The SSL information for this site "
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
                    KSslInfoDialog::errorsFromString(sslInfo.certificateErrors()));

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
        slotWalletClosed();

        // If "NoEmitOpenUrlNotification" property is set to true, do not
        // emit the open url notification. Property is set by this part's
        // extension to prevent openUrl notification being sent when
        // handling history navigation requests.
        const bool doNotEmitOpenUrl = property("NoEmitOpenUrlNotification").toBool();
        if (doNotEmitOpenUrl) {
            setProperty("NoEmitOpenUrlNotification", QVariant());
        }

        // Only emit open url notify for the main frame. Do not
        if (m_emitOpenUrlNotify && !doNotEmitOpenUrl) {
            // kDebug() << "***** EMITTING openUrlNotify" << item->url();
            emit m_browserExtension->openUrlNotify();
        }
    }

    // For some reason, QtWebEngine PORTING_TODO does not restore scroll position when
    // QWebHistory is restored from persistent storage. Therefore, we
    // preserve that information and restore it as needed. See
    // slotRestoreFrameState.
    const QPoint scrollPos (frame->scrollPosition());
    if (!scrollPos.isNull()) {
        // kDebug() << "Saving scroll position:" << scrollPos;
        item->setUserData(scrollPos);
    }
}
#endif

#if 0
void WebEnginePart::slotRestoreFrameState(QWebFrame *frame)
{
    QWebPage* page = (frame ? frame->page() : 0);
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

        if (QString::compare(scheme, QL1S("mailto"), Qt::CaseInsensitive) == 0) {
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
                //kDebug() << "query: " << queryItem.first << queryItem.second;
                if (queryItem.first.contains(QL1C('@')) && queryItem.second.isEmpty())
                    fields["to"] << queryItem.first;
                if (QString::compare(queryItem.first, QL1S("to"), Qt::CaseInsensitive) == 0)
                    fields["to"] << queryItem.second;
                if (QString::compare(queryItem.first, QL1S("cc"), Qt::CaseInsensitive) == 0)
                    fields["cc"] << queryItem.second;
                if (QString::compare(queryItem.first, QL1S("bcc"), Qt::CaseInsensitive) == 0)
                    fields["bcc"] << queryItem.second;
                if (QString::compare(queryItem.first, QL1S("subject"), Qt::CaseInsensitive) == 0)
                    fields["subject"] << queryItem.second;
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

    //kDebug() << "search for text:" << text << ", backward ?" << backward;
    page()->findText(text, flags, [this](bool found) {
        m_searchBar->setFoundMatch(found);
    });
}

void WebEnginePart::slotShowSearchBar()
{
    if (!m_searchBar) {
        // Create the search bar...
        m_searchBar = new SearchBar(widget());
        connect(m_searchBar, SIGNAL(searchTextChanged(QString,bool)),
                this, SLOT(slotSearchForText(QString,bool)));

        actionCollection()->addAction(KStandardAction::FindNext, "findnext",
                                      m_searchBar, SLOT(findNext()));
        actionCollection()->addAction(KStandardAction::FindPrev, "findprev",
                                      m_searchBar, SLOT(findPrevious()));

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
        KMessageBox::questionYesNo(m_webView,
                                   i18n("<qt>Do you want to search for <b>%1</b>?</qt>", searchText),
                                   i18n("Internet Search"), KGuiItem(i18n("&Search"), "edit-find"),
                                   KStandardGuiItem::cancel(), "MiddleClickSearch") != KMessageBox::Yes)
        return;

    emit m_browserExtension->openUrlRequest(selectedUrl);
}

void WebEnginePart::slotWalletClosed()
{
    if (!m_statusBarWalletLabel)
       return;

    m_statusBarExtension->removeStatusBarItem(m_statusBarWalletLabel);
    delete m_statusBarWalletLabel;
    m_statusBarWalletLabel = 0;
    m_hasCachedFormData = false;
}

void WebEnginePart::slotShowWalletMenu()
{
    QMenu *menu = new QMenu(0);

    if (m_webView && WebEngineSettings::self()->isNonPasswordStorableSite(m_webView->url().host()))
      menu->addAction(i18n("&Allow password caching for this site"), this, SLOT(slotDeleteNonPasswordStorableSite()));

    if (m_hasCachedFormData)
      menu->addAction(i18n("Remove all cached passwords for this site"), this, SLOT(slotRemoveCachedPasswords()));

    menu->addSeparator();
    menu->addAction(i18n("&Close Wallet"), this, SLOT(slotWalletClosed()));

    KAcceleratorManager::manage(menu);
    menu->popup(QCursor::pos());
}

void WebEnginePart::slotLaunchWalletManager()
{
    QDBusInterface r("org.kde.kwalletmanager", "/kwalletmanager/MainWindow_1");
    if (r.isValid())
        r.call(QDBus::NoBlock, "show");
    else
        KToolInvocation::startServiceByDesktopName("kwalletmanager_show");
}

void WebEnginePart::slotDeleteNonPasswordStorableSite()
{
    if (m_webView)
        WebEngineSettings::self()->removeNonPasswordStorableSite(m_webView->url().host());
}

void WebEnginePart::slotRemoveCachedPasswords()
{
    if (!page()) // || !page()->wallet())
        return;

//    page()->wallet()->removeFormData(page()->mainFrame(), true);
    m_hasCachedFormData = false;
}

void WebEnginePart::slotSetTextEncoding(QTextCodec * codec)
{
    // FIXME: The code below that sets the text encoding has been reported not to work.
    if (!page())
        return;

    QWebEngineSettings *localSettings = page()->settings();
    if (!localSettings)
        return;

    qDebug() << "Encoding: new=>" << localSettings->defaultTextEncoding() << ", old=>" << codec->name();

    localSettings->setDefaultTextEncoding(codec->name());
    page()->triggerAction(QWebEnginePage::Reload);
}

void WebEnginePart::slotSetStatusBarText(const QString& text)
{
    const QString host (page() ? page()->url().host() : QString());
    if (WebEngineSettings::self()->windowStatusPolicy(host) == KParts::HtmlSettingsInterface::JSWindowStatusAllow)
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

void WebEnginePart::slotShowFeaturePermissionBar(QWebEnginePage::Feature feature)
{
    // FIXME: Allow multiple concurrent feature permission requests.
    if (m_featurePermissionBar && m_featurePermissionBar->isVisible())
        return;

    if (!m_featurePermissionBar) {
        m_featurePermissionBar = new FeaturePermissionBar(widget());

        connect(m_featurePermissionBar, SIGNAL(permissionGranted(QWebPage::Feature)),
                this, SLOT(slotFeaturePermissionGranted(QWebPage::Feature)));
        connect(m_featurePermissionBar, SIGNAL(permissionDenied(QWebPage::Feature)),
                this, SLOT(slotFeaturePermissionDenied(QWebPage::Feature)));
//         connect(m_passwordBar, SIGNAL(done()),
//                 this, SLOT(slotSaveFormDataDone()));
        QBoxLayout* lay = qobject_cast<QBoxLayout*>(widget()->layout());
        if (lay)
            lay->insertWidget(0, m_featurePermissionBar);
    }
    m_featurePermissionBar->setFeature(feature);
//     m_featurePermissionBar->setText(i18n("<html>Do you want to grant the site <b>%1</b> "
//                                     "access to information about your current physical location?",
//                                     url.host()));
    m_featurePermissionBar->setText(i18n("<html>Do you want to grant the site "
                                    "access to information about your current physical location?"));
    m_featurePermissionBar->animatedShow();
}

void WebEnginePart::slotFeaturePermissionGranted(QWebEnginePage::Feature feature)
{
    Q_ASSERT(m_featurePermissionBar && m_featurePermissionBar->feature() == feature);
    page()->setFeaturePermission(page()->url(), feature, QWebEnginePage::PermissionGrantedByUser);
}

void WebEnginePart::slotFeaturePermissionDenied(QWebEnginePage::Feature feature)
{
    Q_ASSERT(m_featurePermissionBar && m_featurePermissionBar->feature() == feature);
    page()->setFeaturePermission(page()->url(), feature, QWebEnginePage::PermissionDeniedByUser);
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
#if 0
        KWebWallet* wallet = page()->wallet();
        if (!wallet) {
            kWarning() << "No wallet instance found! This should never happen!";
            return;
        }
        connect(m_passwordBar, SIGNAL(saveFormDataAccepted(QString)),
                wallet, SLOT(acceptSaveFormDataRequest(QString)));
        connect(m_passwordBar, SIGNAL(saveFormDataRejected(QString)),
                wallet, SLOT(rejectSaveFormDataRequest(QString)));
        connect(m_passwordBar, SIGNAL(done()),
                this, SLOT(slotSaveFormDataDone()));
#endif
    }

    Q_ASSERT(m_passwordBar);

    m_passwordBar->setUrl(url);
    m_passwordBar->setRequestKey(key);
    m_passwordBar->setText(i18n("<html>Do you want %1 to remember the login "
                                "information for <b>%2</b>?</html>",
                                QCoreApplication::applicationName(),
                                url.host()));

    QBoxLayout* lay = qobject_cast<QBoxLayout*>(widget()->layout());
    if (lay)
        lay->insertWidget(0, m_passwordBar);

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

void WebEnginePart::addWalletStatusBarIcon ()
{
    if (m_statusBarWalletLabel) {
        m_statusBarExtension->removeStatusBarItem(m_statusBarWalletLabel);
    } else {
        m_statusBarWalletLabel = new KUrlLabel(m_statusBarExtension->statusBar());
        m_statusBarWalletLabel->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum));
        m_statusBarWalletLabel->setUseCursor(false);
        m_statusBarWalletLabel->setPixmap(QIcon::fromTheme("wallet-open").pixmap(QSize(16,16)));
        connect(m_statusBarWalletLabel, SIGNAL(leftClickedUrl()), SLOT(slotLaunchWalletManager()));
        connect(m_statusBarWalletLabel, SIGNAL(rightClickedUrl()), SLOT(slotShowWalletMenu()));
    }
    m_statusBarExtension->addStatusBarItem(m_statusBarWalletLabel, 0, false);
}

void WebEnginePart::slotFillFormRequestCompleted (bool ok)
{
    if ((m_hasCachedFormData = ok))
        addWalletStatusBarIcon();
}

#include "webenginepart.moc"
