/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
 * Copyright (C) 2008 - 2010 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
 * Copyright (C) 2009 Dawit Alemayehu <adawit@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "kwebkitpart.h"

#include "kwebkitpart_ext.h"
#include "sslinfodialog_p.h"
#include "webview.h"
#include "webpage.h"
#include "websslinfo.h"
#include "webhistoryinterface.h"

#include "ui/searchbar.h"
#include "ui/passwordbar.h"
#include "settings/webkitsettings.h"

#include <kdeversion.h>
#include <kcodecaction.h>
#include <kio/global.h>

#include <KDE/KActionCollection>
#include <KDE/KAboutData>
#include <KDE/KComponentData>
#include <KDE/KDebug>
#include <KDE/KUrlLabel>
#include <KDE/KMessageBox>
#include <KDE/KStringHandler>
#include <KDE/KMenu>
#include <KDE/KWebWallet>
#include <KDE/KToolInvocation>
#include <KDE/KAcceleratorManager>
#include <KDE/KStatusBar>
#include <KDE/KFileItem>
#include <KParts/StatusBarExtension>

#include <QtCore/QUrl>
#include <QtCore/QRect>
#include <QtCore/QFile>
#include <QtCore/QTextCodec>
#include <QtGui/QApplication>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPrintPreviewDialog>
#include <QtDBus/QDBusInterface>
#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebElement>
#include <QtWebKit/QWebHistoryItem>

#define QL1S(x)  QLatin1String(x)
#define QL1C(x)  QLatin1Char(x)

static const QUrl sAboutBlankUrl (QL1S("about:blank"));

static inline int convertStr2Int(const QString& value)
{
   bool ok;
   const int tempValue = value.toInt(&ok);

   if (ok)
     return tempValue;

   return 0;
}

KWebKitPart::KWebKitPart(QWidget *parentWidget, QObject *parent,
                         const QStringList& args)
            :KParts::ReadOnlyPart(parent),
             m_emitOpenUrlNotify(true),
             m_pageRestored(false),
             m_hasCachedFormData(false),
             m_statusBarWalletLabel(0)
{
    KAboutData about = KAboutData("kwebkitpart", 0,
                                  ki18nc("Program Name", "KWebKitPart"),
                                  /*version*/ "1.2.0",
                                  ki18nc("Short Description", "QtWebKit Browser Engine Component"),
                                  KAboutData::License_LGPL,
                                  ki18n("(C) 2009-2010 Dawit Alemayehu\n"
                                        "(C) 2008-2010 Urs Wolfer\n"
                                        "(C) 2007 Trolltech ASA"));

    about.addAuthor(ki18n("Dawit Alemayehu"), ki18n("Maintainer, Developer"), "adawit@kde.org");
    about.addAuthor(ki18n("Urs Wolfer"), ki18n("Maintainer, Developer"), "uwolfer@kde.org");
    about.addAuthor(ki18n("Michael Howell"), ki18n("Developer"), "mhowell123@gmail.com");
    about.addAuthor(ki18n("Laurent Montel"), ki18n("Developer"), "montel@kde.org");
    about.addAuthor(ki18n("Dirk Mueller"), ki18n("Developer"), "mueller@kde.org");
    about.setProductName("kwebkitpart/general");
    KComponentData componentData(&about);
    setComponentData(componentData, false /*don't load plugins yet*/);

    // NOTE: If the application does not set its version number, we automatically
    // set it to KDE's version number so that the default user-agent string contains
    // proper application version number information. See QWebPage::userAgentForUrl...
    if (QCoreApplication::applicationVersion().isEmpty())
        QCoreApplication::setApplicationVersion(QString("%1.%2.%3")
                                                .arg(KDE::versionMajor())
                                                .arg(KDE::versionMinor())
                                                .arg(KDE::versionRelease()));

    QWidget *mainWidget = new QWidget (parentWidget);
    mainWidget->setObjectName("kwebkitpart");
    setWidget(mainWidget);

    // Create the WebView...
    m_webView = new WebView (this, mainWidget);

    // Create the browser extension. The first item of 'args' is used to pass
    // the session history filename.
    m_browserExtension = new WebKitBrowserExtension(this, args.at(0));

    // Add status bar extension...
    m_statusBarExtension = new KParts::StatusBarExtension(this);

    // Add a web history interface for storing visited links.
    if (!QWebHistoryInterface::defaultInterface())
        QWebHistoryInterface::setDefaultInterface(new WebHistoryInterface(this));

    // Add text and html extensions...
    new KWebKitTextExtension(this);
    new KWebKitHtmlExtension(this);

    // Create and setup the password bar...
    m_passwordBar = new KDEPrivate::PasswordBar(mainWidget);

    // Create the search bar...
    m_searchBar = new KDEPrivate::SearchBar(mainWidget);
    connect(m_searchBar, SIGNAL(searchTextChanged(const QString &, bool)),
            this, SLOT(slotSearchForText(const QString &, bool)));

    // Connect the signals/slots from the webview...
    connect(m_webView, SIGNAL(titleChanged(const QString &)),
            this, SIGNAL(setWindowCaption(const QString &)));
    connect(m_webView, SIGNAL(loadFinished(bool)),
            this, SLOT(slotLoadFinished(bool)));
    connect(m_webView, SIGNAL(urlChanged(const QUrl &)),
            this, SLOT(slotUrlChanged(const QUrl &)));
    connect(m_webView, SIGNAL(linkMiddleOrCtrlClicked(const KUrl &)),
            this, SLOT(slotLinkMiddleOrCtrlClicked(const KUrl &)));
    connect(m_webView, SIGNAL(selectionClipboardUrlPasted(const KUrl &, const QString &)),
            this, SLOT(slotSelectionClipboardUrlPasted(const KUrl &, const QString &)));

    // Connect the signals from the page...
    connectWebPageSignals(page());

    // Layout the GUI...
    QVBoxLayout* lay = new QVBoxLayout(mainWidget);
    lay->setMargin(0);
    lay->setSpacing(0);
    lay->addWidget(m_passwordBar);
    lay->addWidget(m_webView);
    lay->addWidget(m_searchBar);

    // Set the web view as the the focus object...
    mainWidget->setFocusProxy(m_webView);

    setXMLFile("kwebkitpart.rc");

    // Init the QAction we are going to use...
    initActions();

    // Load plugins once we are fully ready
    loadPlugins();
}

KWebKitPart::~KWebKitPart()
{
    m_browserExtension->saveHistoryState();
}


WebPage* KWebKitPart::page()
{
    if (m_webView)
        return qobject_cast<WebPage*>(m_webView->page());
    return 0;
}

void KWebKitPart::initActions()
{
    KAction *action = actionCollection()->addAction(KStandardAction::SaveAs, "saveDocument",
                                                    m_browserExtension, SLOT(slotSaveDocument()));

    action = new KAction(i18n("Save &Frame As..."), this);
    actionCollection()->addAction("saveFrame", action);
    connect(action, SIGNAL(triggered(bool)), m_browserExtension, SLOT(slotSaveFrame()));

    action = new KAction(KIcon("document-print-frame"), i18n("Print Frame..."), this);
    actionCollection()->addAction("printFrame", action);
    connect(action, SIGNAL(triggered(bool)), m_browserExtension, SLOT(printFrame()));

    action = new KAction(KIcon("zoom-in"), i18nc("zoom in action", "Zoom In"), this);
    actionCollection()->addAction("zoomIn", action);
    action->setShortcut(KShortcut("CTRL++; CTRL+="));
    connect(action, SIGNAL(triggered(bool)), m_browserExtension, SLOT(zoomIn()));

    action = new KAction(KIcon("zoom-out"), i18nc("zoom out action", "Zoom Out"), this);
    actionCollection()->addAction("zoomOut", action);
    action->setShortcut(KShortcut("CTRL+-; CTRL+_"));
    connect(action, SIGNAL(triggered(bool)), m_browserExtension, SLOT(zoomOut()));

    action = new KAction(KIcon("zoom-original"), i18nc("reset zoom action", "Actual Size"), this);
    actionCollection()->addAction("zoomNormal", action);
    action->setShortcut(KShortcut("CTRL+0"));
    connect(action, SIGNAL(triggered(bool)), m_browserExtension, SLOT(zoomNormal()));

    action = new KAction(i18n("Zoom Text Only"), this);
    action->setCheckable(true);
    KConfigGroup cgHtml(KGlobal::config(), "HTML Settings");
    bool zoomTextOnly = cgHtml.readEntry("ZoomTextOnly", false);
    action->setChecked(zoomTextOnly);
    actionCollection()->addAction("zoomTextOnly", action);
    connect(action, SIGNAL(triggered(bool)), m_browserExtension, SLOT(toogleZoomTextOnly()));

    action = actionCollection()->addAction(KStandardAction::SelectAll, "selectAll",
                                           m_browserExtension, SLOT(slotSelectAll()));
    action->setShortcutContext(Qt::WidgetShortcut);
    m_webView->addAction(action);

    KCodecAction *codecAction = new KCodecAction( KIcon("character-set"), i18n( "Set &Encoding" ), this, true );
    actionCollection()->addAction( "setEncoding", codecAction );
    connect(codecAction, SIGNAL(triggered(QTextCodec*)), SLOT(slotSetTextEncoding(QTextCodec*)));

    action = new KAction(i18n("View Do&cument Source"), this);
    actionCollection()->addAction("viewDocumentSource", action);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_U));
    connect(action, SIGNAL(triggered(bool)), m_browserExtension, SLOT(slotViewDocumentSource()));

    action = new KAction(i18nc("Secure Sockets Layer", "SSL"), this);
    actionCollection()->addAction("security", action);
    connect(action, SIGNAL(triggered(bool)), SLOT(slotShowSecurity()));

    action = actionCollection()->addAction(KStandardAction::Find, "find", this, SLOT(slotShowSearchBar()));
    action->setWhatsThis(i18nc("find action \"whats this\" text", "<h3>Find text</h3>"
                              "Shows a dialog that allows you to find text on the displayed page."));

    action = actionCollection()->addAction(KStandardAction::FindNext, "findnext",
                                           m_searchBar, SLOT(findNext()));
    action = actionCollection()->addAction(KStandardAction::FindPrev, "findprev",
                                           m_searchBar, SLOT(findPrevious()));
}

void KWebKitPart::connectWebPageSignals(WebPage* page)
{
    if (!page)
        return;

    connect(page, SIGNAL(loadStarted()),
            this, SLOT(slotLoadStarted()));
    connect(page, SIGNAL(loadAborted(const KUrl &)),
            this, SLOT(slotLoadAborted(const KUrl &)));
    connect(page, SIGNAL(linkHovered(const QString &, const QString &, const QString &)),
            this, SLOT(slotLinkHovered(const QString &, const QString &, const QString &)));
    connect(page, SIGNAL(saveFrameStateRequested(QWebFrame *, QWebHistoryItem *)),
            this, SLOT(slotSaveFrameState(QWebFrame *, QWebHistoryItem *)));
    connect(page, SIGNAL(restoreFrameStateRequested(QWebFrame *)),
            this, SLOT(slotRestoreFrameState(QWebFrame *)));
    connect(page, SIGNAL(statusBarMessage(const QString&)),
            this, SLOT(slotSetStatusBarText(const QString &)));
    connect(page, SIGNAL(windowCloseRequested()),
            this, SLOT(slotWindowCloseRequested()));
    connect(page, SIGNAL(printRequested(QWebFrame*)),
            this, SLOT(slotPrintRequested(QWebFrame*)));

    connect(page, SIGNAL(loadStarted()), m_searchBar, SLOT(clear()));
    connect(page, SIGNAL(loadStarted()), m_searchBar, SLOT(hide()));

    connect(m_webView, SIGNAL(linkShiftClicked(const KUrl &)),
            page, SLOT(downloadUrl(const KUrl &)));

    connect(page, SIGNAL(loadProgress(int)),
            m_browserExtension, SIGNAL(loadingProgress(int)));
    connect(page, SIGNAL(selectionChanged()),
            m_browserExtension, SLOT(updateEditActions()));
    connect(m_browserExtension, SIGNAL(saveUrl(const KUrl&)),
            page, SLOT(downloadUrl(const KUrl &)));

    KWebWallet *wallet = page->wallet();
    if (wallet) {
        connect(wallet, SIGNAL(saveFormDataRequested(const QString &, const QUrl &)),
                m_passwordBar, SLOT(onSaveFormData(const QString &, const QUrl &)));
        connect(m_passwordBar, SIGNAL(saveFormDataAccepted(const QString &)),
                wallet, SLOT(acceptSaveFormDataRequest(const QString &)));
        connect(m_passwordBar, SIGNAL(saveFormDataRejected(const QString &)),
                wallet, SLOT(rejectSaveFormDataRequest(const QString &)));
        connect(wallet, SIGNAL(walletClosed()), this, SLOT(slotWalletClosed()));
    }
}

bool KWebKitPart::openUrl(const KUrl &u)
{
    kDebug() << u;

    // Ignore empty requests...
    if (u.isEmpty())
        return false;

    // Do not update history when url is typed in since konqueror
    // automatically does that itself.
    m_emitOpenUrlNotify = false;

    // Handle error conditions...
    if (u.protocol().compare(QL1S("error"), Qt::CaseInsensitive) == 0 && u.hasSubUrl()) {
        /**
         * The format of the error url is that two variables are passed in the query:
         * error = int kio error code, errText = QString error text from kio
         * and the URL where the error happened is passed as a sub URL.
         */
        KUrl::List urls = KUrl::split(u);

        if ( urls.count() > 1 ) {
            KUrl mainURL = urls.first();
            int error = convertStr2Int(mainURL.queryItem("error"));

            // error=0 isn't a valid error code, so 0 means it's missing from the URL
            if ( error == 0 )
                error = KIO::ERR_UNKNOWN;

            const QString errorText = mainURL.queryItem( "errText" );
            urls.pop_front();
            KUrl reqUrl = KUrl::join( urls );
            emit m_browserExtension->setLocationBarUrl(reqUrl.prettyUrl());
            if (page()) {
                m_webView->setHtml(page()->errorPage(error, errorText, reqUrl));
                return true;
            }
        }

        return false;
    }

    KParts::BrowserArguments bargs (m_browserExtension->browserArguments());
    KParts::OpenUrlArguments args (arguments());
    const bool isAboutBlank = (sAboutBlankUrl == u);

    if (!isAboutBlank && page()) {
        // Check if this is a restore state request, i.e. a history navigation
        // or a session restore. If it is, fulfill the request using QWebHistory.
        if (args.metaData().contains(QL1S("kwebkitpart-restore-state"))) {
            m_pageRestored = true;
            const int historyCount = page()->history()->count();
            const int historyIndex = args.metaData().take(QL1S("kwebkitpart-restore-state")).toInt();
            if (historyCount > 0 && historyIndex > -1 && historyIndex < historyCount ) {
                QWebHistoryItem historyItem = page()->history()->itemAt(historyIndex);
                const bool restoreScrollPos = args.metaData().contains(QL1S("kwebkitpart-restore-scrollx"));
                if (restoreScrollPos) {
                    QMap<QString, QVariant> data;
                    data.insert(QL1S("scrollx"), args.metaData().take(QL1S("kwebkitpart-restore-scrollx")).toInt());
                    data.insert(QL1S("scrolly"), args.metaData().take(QL1S("kwebkitpart-restore-scrolly")).toInt());
                    historyItem.setUserData(data);
                }

                if (historyItem.isValid()) {
                    setUrl(historyItem.url());
                    // Set any cached ssl information...
                    if (historyItem.userData().isValid()) {
                        WebSslInfo sslInfo;
                        sslInfo.restoreFrom(historyItem.userData());
                        page()->setSslInfo(sslInfo);
                    }
                    page()->history()->goToItem(historyItem);
                    // Update the part's OpenUrlArguments after removing all of the
                    // 'kwebkitpart-restore-x' metadata entries...
                    setArguments(args);
                    return true;
                }
            }
        }

        // Get the SSL information sent, if any...
        if (args.metaData().contains(QL1S("ssl_in_use"))) {
            WebSslInfo sslInfo;
            sslInfo.restoreFrom(KIO::MetaData(args.metaData()).toVariant());
            sslInfo.setUrl(u);
            page()->setSslInfo(sslInfo);
        }

        // Update the part's OpenUrlArguments after removing all of the
        // 'kwebkitpart-restore-x' metadata entries...
        setArguments(args);
    }

    // Set URL in KParts before emitting started; konq plugins rely on that.
    setUrl(u);
    if (!isAboutBlank) {
        m_webView->loadUrl(u, args, bargs);
    }
    return true;
}

bool KWebKitPart::closeUrl()
{
#if QT_VERSION >= 0x040700
    m_webView->triggerPageAction(QWebPage::StopScheduledPageRefresh);
#endif
    m_webView->stop();
    return true;
}

QWebView* KWebKitPart::view()
{
    return m_webView;
}

bool KWebKitPart::isModified() const
{
    return m_webView->isModified();
}

void KWebKitPart::guiActivateEvent(KParts::GUIActivateEvent *event)
{
    Q_UNUSED(event);
    // just overwrite, but do nothing for the moment
}

bool KWebKitPart::openFile()
{
    // never reached
    return false;
}


/// slots...

void KWebKitPart::slotLoadStarted()
{
    emit started(0);
    slotWalletClosed();
}

void KWebKitPart::slotLoadFinished(bool ok)
{
    m_emitOpenUrlNotify = true;

    if (ok) {
        if (m_webView->title().trimmed().isEmpty()) {
            // If the document title is empty, then set it to the current url
            const QString caption = m_webView->url().toString((QUrl::RemoveQuery|QUrl::RemoveFragment));
            emit setWindowCaption(caption);

            // The urlChanged signal is emitted if and only if the main frame
            // receives the title of the page so we manually invoke the slot as
            // a work around here for pages that do not contain it, such as
            // text documents...
            slotUrlChanged(m_webView->url());
        }

        const QUrl mainUrl = m_webView->url();
        if (mainUrl != sAboutBlankUrl && page()) {
            // Fill form data from wallet...
            KWebWallet *webWallet = page()->wallet();
            if (webWallet) {
                webWallet->fillFormData(page()->mainFrame());
                KWebWallet::WebFormList list = webWallet->formsWithCachedData(page()->mainFrame());
                if (!list.isEmpty()) {
                    if (!m_statusBarWalletLabel) {
                        m_statusBarWalletLabel = new KUrlLabel(m_statusBarExtension->statusBar());
                        m_statusBarWalletLabel->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum));
                        m_statusBarWalletLabel->setUseCursor(false);
                        m_statusBarWalletLabel->setPixmap(SmallIcon("wallet-open"));
                        connect(m_statusBarWalletLabel, SIGNAL(leftClickedUrl()), SLOT(slotLaunchWalletManager()));
                        connect(m_statusBarWalletLabel, SIGNAL(rightClickedUrl()), SLOT(slotShowWalletMenu()));
                    }

                    m_statusBarExtension->addStatusBarItem(m_statusBarWalletLabel, 0, false);
                    m_hasCachedFormData = true;
                }
            }

            // Set the favicon specified through the <link> tag...
            if (WebKitSettings::self()->favIconsEnabled() &&
                mainUrl.scheme().startsWith(QL1S("http"), Qt::CaseInsensitive)) {
                const QWebElement element = page()->mainFrame()->findFirstElement(QL1S("head>link[rel=icon], "
                                                                                        "head>link[rel=\"shortcut icon\"]"));
                KUrl shortcutIconUrl;
                if (element.isNull()) {
                    shortcutIconUrl = page()->mainFrame()->baseUrl();
                    QString urlPath = shortcutIconUrl.path();
                    const int index = urlPath.indexOf(QL1C('/'));
                    if (index > -1)
                      urlPath.truncate(index);
                    urlPath += QL1S("/favicon.ico");
                    shortcutIconUrl.setPath(urlPath);
                } else {
                    shortcutIconUrl = KUrl (page()->mainFrame()->baseUrl(), element.attribute("href"));
                }

                kDebug() << "setting favicon to" << shortcutIconUrl;
                m_browserExtension->setIconUrl(shortcutIconUrl);
            }
        }
    }

    // Set page restored to false, if the page was restored...
    if (m_pageRestored) {
        m_pageRestored = false;
        // Restore the scroll postions if present...
        KParts::OpenUrlArguments args = arguments();
        if (args.metaData().contains(QL1S("kwebkitpart-restore-scrollx"))) {
            const int scrollPosX = args.metaData().take(QL1S("kwebkitpart-restore-scrollx")).toInt();
            const int scrollPosY = args.metaData().take(QL1S("kwebkitpart-restore-scrolly")).toInt();
            if (page()) {
                page()->mainFrame()->setScrollPosition(QPoint(scrollPosX, scrollPosY));
                setArguments(args);
            }
        }
    }

    /*
      NOTE: Support for stopping meta data redirects is implemented in QtWebKit
      2.0 (Qt 4.7) or greater. See https://bugs.webkit.org/show_bug.cgi?id=29899.
    */
    bool pending = false;
#if QT_VERSION >= 0x040700
    if (page() && page()->mainFrame()->findAllElements(QL1S("head>meta[http-equiv=refresh]")).count()) {
        if (WebKitSettings::self()->autoPageRefresh())
            pending = true;
        else
            page()->triggerAction(QWebPage::StopScheduledPageRefresh);
    }
#endif
    emit completed((ok && pending));
}

void KWebKitPart::slotLoadAborted(const KUrl & url)
{
    closeUrl();
    if (url.isValid())
        emit m_browserExtension->openUrlRequest(url);
    else
        setUrl(m_webView->url());
}

void KWebKitPart::slotUrlChanged(const QUrl& url)
{
    // Ignore if empty
    if (url.isEmpty())
        return;

    // Ignore if error url
    if (url.scheme().compare(QL1S("error"), Qt::CaseInsensitive) == 0)
        return;

    const KUrl u (url);
    setUrl(u);

    // Do not update the location bar with about:blank
    if (url == sAboutBlankUrl)
        return;

    kDebug() << "Setting location bar to" << u.prettyUrl();
    emit m_browserExtension->setLocationBarUrl(u.prettyUrl());
}

void KWebKitPart::slotShowSecurity()
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

void KWebKitPart::slotSaveFrameState(QWebFrame *frame, QWebHistoryItem *item)
{
    if (!page() || frame != page()->mainFrame())
        return;

    //kDebug() << "Update history ?" << m_emitOpenUrlNotify;
    if (m_emitOpenUrlNotify)
        emit m_browserExtension->openUrlNotify();

    // Save the SSL info as the history item meta-data...
    if (!item)
        return;

    QMap<QString, QVariant> data;
    const QVariant v = item->userData();

    if (v.isValid() && v.type() == QVariant::Map)
        data = v.toMap();

    if (page()->sslInfo().saveTo(data))
        item->setUserData(data);
}

void KWebKitPart::slotRestoreFrameState(QWebFrame *frame)
{
    //kDebug() << "Restore state for" << frame;
    if (!page() || frame != page()->mainFrame())
        return;

    m_emitOpenUrlNotify = true;

    if (!m_pageRestored)
        return;

    const QWebHistoryItem item = frame->page()->history()->currentItem();
    QVariant v = item.userData();
    if (!v.isValid() || v.type() != QVariant::Map)
        return;

    QMap<QString, QVariant> data = v.toMap();
    if (data.contains(QL1S("scrollx")) && data.contains(QL1S("scrolly")))
        frame->setScrollPosition(QPoint(data.value(QL1S("scrollx")).toInt(),
                                        data.value(QL1S("scrolly")).toInt()));
}

void KWebKitPart::slotLinkHovered(const QString &link, const QString &title, const QString &content)
{
    Q_UNUSED(title);
    Q_UNUSED(content);

    QString message;

    if (link.isEmpty()) {
        message = QL1S("");
        emit m_browserExtension->mouseOverInfo(KFileItem());
    } else {
        QUrl linkUrl (link);
        const QString scheme = linkUrl.scheme();

        if (QString::compare(scheme, QL1S("mailto"), Qt::CaseInsensitive) == 0) {
            message += i18nc("status bar text when hovering email links; looks like \"Email: xy@kde.org - CC: z@kde.org -BCC: x@kde.org - Subject: Hi translator\"", "Email: ");

            // Workaround: for QUrl's parsing deficiencies of "mailto:foo@bar.com".
            if (!linkUrl.hasQuery())
              linkUrl = QUrl(scheme + '?' + linkUrl.path());

            QMap<QString, QStringList> fields;
            QPair<QString, QString> queryItem;

            Q_FOREACH (queryItem, linkUrl.queryItems()) {
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
        } else if (linkUrl.scheme() == QL1S("javascript")) {
            message = KStringHandler::rsqueeze(link, 80);
            if (link.startsWith(QL1S("javascript:window.open")))
                message += i18n(" (In new window)");
        } else {
            message = link;
            if (page()) {
                QWebFrame* frame = page()->currentFrame();
                if (frame) {
                    const QString query = QL1S("a[href*=\"") + link + QL1S("\"]");
                    const QWebElement element = frame->findFirstElement(query);
                    const QString target = element.attribute(QL1S("target"));
                    if (target.compare(QL1S("_blank"), Qt::CaseInsensitive) == 0 ||
                        target.compare(QL1S("top"), Qt::CaseInsensitive) == 0) {
                        message += i18n(" (In new window)");
                    } else if (target.compare(QL1S("_parent"), Qt::CaseInsensitive) == 0) {
                        message += i18n(" (In parent frame)");
                    }
                }
            }
            kDebug() << "link:" << link << "title:" << title << "content:" << content;
            KFileItem item (linkUrl, QString(), KFileItem::Unknown);
            emit m_browserExtension->mouseOverInfo(item);
        }
    }

    emit setStatusBarText(message);
}

void KWebKitPart::slotSearchForText(const QString &text, bool backward)
{
    QWebPage::FindFlags flags = QWebPage::FindWrapsAroundDocument;

    if (backward)
        flags |= QWebPage::FindBackward;

    if (m_searchBar->caseSensitive())
        flags |= QWebPage::FindCaseSensitively;

    if (m_searchBar->highlightMatches())
        flags |= QWebPage::HighlightAllOccurrences;

    //kDebug() << "search for text:" << text << ", backward ?" << backward;
    m_searchBar->setFoundMatch(page()->findText(text, flags));
}

void KWebKitPart::slotShowSearchBar()
{
    const QString text = m_webView->selectedText();
    m_searchBar->setSearchText(text.left(150));
}

void KWebKitPart::slotLinkMiddleOrCtrlClicked(const KUrl& linkUrl)
{
    KParts::OpenUrlArguments args;
    args.setActionRequestedByUser(true);
    emit m_browserExtension->createNewWindow(linkUrl, args);
}

void KWebKitPart::slotSelectionClipboardUrlPasted(const KUrl& selectedUrl, const QString& searchText)
{
    if (!WebKitSettings::self()->isOpenMiddleClickEnabled())
        return;

    if (!searchText.isEmpty() &&
        KMessageBox::questionYesNo(m_webView,
                                   i18n("<qt>Do you want to search for <b>%1</b>?</qt>", searchText),
                                   i18n("Internet Search"), KGuiItem(i18n("&Search"), "edit-find"),
                                   KStandardGuiItem::cancel(), "MiddleClickSearch") != KMessageBox::Yes)
        return;

    emit m_browserExtension->openUrlRequest(selectedUrl);
}

void KWebKitPart::slotWalletClosed()
{
    if (!m_statusBarWalletLabel)
       return;

    m_statusBarExtension->removeStatusBarItem(m_statusBarWalletLabel);
    delete m_statusBarWalletLabel;
    m_statusBarWalletLabel = 0;
    m_hasCachedFormData = false;
}

void KWebKitPart::slotShowWalletMenu()
{
    KMenu *menu = new KMenu(0);

    if (m_webView && WebKitSettings::self()->isNonPasswordStorableSite(m_webView->url().host()))
      menu->addAction(i18n("&Allow password caching for this site"), this, SLOT(slotDeleteNonPasswordStorableSite()));

    if (page() && m_hasCachedFormData)
      menu->addAction(i18n("Remove all cached passwords for this site"), this, SLOT(slotRemoveCachedPasswords()));

    menu->addSeparator();
    menu->addAction(i18n("&Close Wallet"), this, SLOT(slotWalletClosed()));

    KAcceleratorManager::manage(menu);
    menu->popup(QCursor::pos());
}

void KWebKitPart::slotLaunchWalletManager()
{
    QDBusInterface r("org.kde.kwalletmanager", "/kwalletmanager/MainWindow_1");
    if (r.isValid())
        r.call(QDBus::NoBlock, "show");
    else
        KToolInvocation::startServiceByDesktopName("kwalletmanager_show");
}

void KWebKitPart::slotDeleteNonPasswordStorableSite()
{
    if (m_webView)
        WebKitSettings::self()->removeNonPasswordStorableSite(m_webView->url().host());
}

void KWebKitPart::slotRemoveCachedPasswords()
{
    if (!page() || !page()->wallet())
        return;

    page()->wallet()->removeFormData(page()->mainFrame(), true);
    m_hasCachedFormData = false;
}

void KWebKitPart::slotSetTextEncoding(QTextCodec * codec)
{
    // FIXME: The code below that sets the text encoding has been reported not to work.
    if (!page())
        return;

    QWebSettings *localSettings = page()->settings();
    if (!localSettings)
        return;

    kDebug() << codec->name();

    localSettings->setDefaultTextEncoding(codec->name());
    openUrl(url());
}

void KWebKitPart::slotSetStatusBarText(const QString& text)
{
    const QString host = page() ? page()->mainFrame()->url().host() : QString();
    if (WebKitSettings::self()->windowStatusPolicy(host) == WebKitSettings::KJSWindowStatusAllow)
        emit setStatusBarText(text);
}

void KWebKitPart::slotWindowCloseRequested()
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

void KWebKitPart::slotPrintRequested(QWebFrame* frame)
{
    if (!frame)
        return;

    QPrintPreviewDialog dlg(m_webView);
    connect(&dlg, SIGNAL(paintRequested(QPrinter *)),
            frame, SLOT(print(QPrinter *)));
    dlg.exec();
}
