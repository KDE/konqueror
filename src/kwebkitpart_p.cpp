/*
 * This file is part of the KDE project.
 *
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

#include "kwebkitpart_p.h"
#include "kwebkitpart_ext.h"
#include "kwebkitpart.h"
#include "webview.h"
#include "webpage.h"
#include "websslinfo.h"
#include "sslinfodialog_p.h"
#include "settings/webkitsettings.h"
#include "ui/passwordbar.h"
#include "ui/searchbar.h"

#include <kwebwallet.h>
#include <kdeversion.h>
#include <kcodecaction.h>
#include <KDE/KLocalizedString>
#include <KDE/KStringHandler>
#include <KDE/KMessageBox>
#include <KDE/KDebug>
#include <KDE/KFileItem>
#include <KDE/KConfigGroup>
#include <KDE/KAction>
#include <KDE/KActionCollection>
#include <KDE/KUrlLabel>
#include <KDE/KStatusBar>
#include <KDE/KToolInvocation>
#include <KDE/KMenu>
#include <KDE/KStandardDirs>
#include <KDE/KConfig>
#include <KDE/KAcceleratorManager>
#include <KParts/StatusBarExtension>

#include <QtCore/QFile>
#include <QtCore/QTextCodec>
#include <QtGui/QVBoxLayout>
#include <QtDBus/QDBusInterface>
#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebElement>
#include <QtWebKit/QWebHistoryItem>

#define QL1S(x) QLatin1String(x)
#define QL1C(x) QLatin1Char(x)


KWebKitPartPrivate::KWebKitPartPrivate(KWebKitPart *parent)
                   :QObject(),
                    emitOpenUrlNotify(true),
                    contentModified(false),
                    q(parent),
                    statusBarWalletLabel(0),
                    hasCachedFormData(false)
{
}

void KWebKitPartPrivate::init(QWidget *mainWidget)
{
    // Create the WebView...
    webView = new WebView (q, mainWidget);
    connect(webView, SIGNAL(titleChanged(const QString &)),
            q, SIGNAL(setWindowCaption(const QString &)));
    connect(webView, SIGNAL(loadFinished(bool)),
            this, SLOT(slotLoadFinished(bool)));
    connect(webView, SIGNAL(urlChanged(const QUrl &)),
            this, SLOT(slotUrlChanged(const QUrl &)));
    connect(webView, SIGNAL(linkMiddleOrCtrlClicked(const KUrl &)),
            this, SLOT(slotLinkMiddleOrCtrlClicked(const KUrl &)));
    connect(webView, SIGNAL(selectionClipboardUrlPasted(const KUrl &)),
            this, SLOT(slotSelectionClipboardUrlPasted(const KUrl &)));

    // Create the search bar...
    searchBar = new KDEPrivate::SearchBar;
    connect(searchBar, SIGNAL(searchTextChanged(const QString &, bool)),
            this, SLOT(slotSearchForText(const QString &, bool)));

    webPage = qobject_cast<WebPage*>(webView->page());
    Q_ASSERT(webPage);

    connect(webPage, SIGNAL(loadStarted()),
            this, SLOT(slotLoadStarted()));
    connect(webPage, SIGNAL(loadAborted(const KUrl &)),
            this, SLOT(slotLoadAborted(const KUrl &)));
    connect(webPage, SIGNAL(linkHovered(const QString &, const QString &, const QString &)),
            this, SLOT(slotLinkHovered(const QString &, const QString &, const QString &)));
    connect(webPage, SIGNAL(saveFrameStateRequested(QWebFrame *, QWebHistoryItem *)),
            this, SLOT(slotSaveFrameState(QWebFrame *, QWebHistoryItem *)));
    connect(webPage, SIGNAL(restoreFrameStateRequested(QWebFrame *)),
            this, SLOT(slotRestoreFrameState(QWebFrame *)));
    connect(webPage, SIGNAL(contentsChanged()), this, SLOT(slotContentsChanged()));
    connect(webPage, SIGNAL(jsStatusBarMessage(const QString &)),
            q, SIGNAL(setStatusBarText(const QString &)));
    connect(webView, SIGNAL(linkShiftClicked(const KUrl &)),
            webPage, SLOT(downloadUrl(const KUrl &)));
    connect(webView, SIGNAL(loadStarted()),
            searchBar, SLOT(hide()));
    connect(webView, SIGNAL(loadStarted()),
            searchBar, SLOT(clear()));

    browserExtension = new WebKitBrowserExtension(q);
    connect(webPage, SIGNAL(loadProgress(int)),
            browserExtension, SIGNAL(loadingProgress(int)));
    connect(webPage, SIGNAL(selectionChanged()),
            browserExtension, SLOT(updateEditActions()));
    connect(browserExtension, SIGNAL(saveUrl(const KUrl&)),
            webPage, SLOT(downloadUrl(const KUrl &)));

    // Add status bar extension...
    statusBarExtension = new KParts::StatusBarExtension(q);

    // Create and setup the password bar...
    KDEPrivate::PasswordBar *passwordBar = new KDEPrivate::PasswordBar(mainWidget);
    KWebWallet *webWallet = webPage->wallet();

    if (webWallet) {
        connect (webWallet, SIGNAL(saveFormDataRequested(const QString &, const QUrl &)),
                 passwordBar, SLOT(onSaveFormData(const QString &, const QUrl &)));
        connect(passwordBar, SIGNAL(saveFormDataAccepted(const QString &)),
                webWallet, SLOT(acceptSaveFormDataRequest(const QString &)));
        connect(passwordBar, SIGNAL(saveFormDataRejected(const QString &)),
                webWallet, SLOT(rejectSaveFormDataRequest(const QString &)));
        connect(webWallet, SIGNAL(walletClosed()), this, SLOT(slotWalletClosed()));
    }

    QVBoxLayout* lay = new QVBoxLayout(mainWidget);
    lay->setMargin(0);
    lay->setSpacing(0);
    lay->addWidget(passwordBar);
    lay->addWidget(webView);
    lay->addWidget(searchBar);

    mainWidget->setFocusProxy(webView);
}

void KWebKitPartPrivate::initActions()
{
    KAction *action = q->actionCollection()->addAction(KStandardAction::SaveAs, "saveDocument",
                                                       browserExtension, SLOT(slotSaveDocument()));

    action = new KAction(i18n("Save &Frame As..."), this);
    q->actionCollection()->addAction("saveFrame", action);
    connect(action, SIGNAL(triggered(bool)), browserExtension, SLOT(slotSaveFrame()));

    action = new KAction(KIcon("document-print-frame"), i18n("Print Frame..."), this);
    q->actionCollection()->addAction("printFrame", action);
    connect(action, SIGNAL(triggered(bool)), browserExtension, SLOT(printFrame()));

    action = new KAction(KIcon("zoom-in"), i18nc("zoom in action", "Zoom In"), this);
    q->actionCollection()->addAction("zoomIn", action);
    action->setShortcut(KShortcut("CTRL++; CTRL+="));
    connect(action, SIGNAL(triggered(bool)), browserExtension, SLOT(zoomIn()));

    action = new KAction(KIcon("zoom-out"), i18nc("zoom out action", "Zoom Out"), this);
    q->actionCollection()->addAction("zoomOut", action);
    action->setShortcut(KShortcut("CTRL+-; CTRL+_"));
    connect(action, SIGNAL(triggered(bool)), browserExtension, SLOT(zoomOut()));

    action = new KAction(KIcon("zoom-original"), i18nc("reset zoom action", "Actual Size"), this);
    q->actionCollection()->addAction("zoomNormal", action);
    action->setShortcut(KShortcut("CTRL+0"));
    connect(action, SIGNAL(triggered(bool)), browserExtension, SLOT(zoomNormal()));

    action = new KAction(i18n("Zoom Text Only"), this);
    action->setCheckable(true);
    KConfigGroup cgHtml(KGlobal::config(), "HTML Settings");
    bool zoomTextOnly = cgHtml.readEntry("ZoomTextOnly", false);
    action->setChecked(zoomTextOnly);
    q->actionCollection()->addAction("zoomTextOnly", action);
    connect(action, SIGNAL(triggered(bool)), browserExtension, SLOT(toogleZoomTextOnly()));

    action = q->actionCollection()->addAction(KStandardAction::SelectAll, "selectAll",
                                           browserExtension, SLOT(slotSelectAll()));
    action->setShortcutContext(Qt::WidgetShortcut);
    webView->addAction(action);

    KCodecAction *codecAction = new KCodecAction( KIcon("character-set"), i18n( "Set &Encoding" ), this, true );
    q->actionCollection()->addAction( "setEncoding", codecAction );
    connect(codecAction, SIGNAL(triggered(QTextCodec*)), SLOT(slotSetTextEncoding(QTextCodec*)));

    action = new KAction(i18n("View Do&cument Source"), this);
    q->actionCollection()->addAction("viewDocumentSource", action);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_U));
    connect(action, SIGNAL(triggered(bool)), browserExtension, SLOT(slotViewDocumentSource()));

    action = new KAction(i18nc("Secure Sockets Layer", "SSL"), this);
    q->actionCollection()->addAction("security", action);
    connect(action, SIGNAL(triggered(bool)), SLOT(slotShowSecurity()));

    action = q->actionCollection()->addAction(KStandardAction::Find, "find", this, SLOT(slotShowSearchBar()));
    action->setWhatsThis(i18nc("find action \"whats this\" text", "<h3>Find text</h3>"
                              "Shows a dialog that allows you to find text on the displayed page."));

    action = q->actionCollection()->addAction(KStandardAction::FindNext, "findnext",
                                              searchBar, SLOT(findNext()));
    action = q->actionCollection()->addAction(KStandardAction::FindPrev, "findprev",
                                              searchBar, SLOT(findPrevious()));
}

void KWebKitPartPrivate::slotLoadStarted()
{
    kDebug();
    emit q->started(0);
    slotWalletClosed();
    contentModified = false;
}

void KWebKitPartPrivate::slotContentsChanged()
{
    contentModified = true;
}

void KWebKitPartPrivate::slotLoadFinished(bool ok)
{
    emitOpenUrlNotify = true;

    if (ok) {
        QString linkStyle;
        QColor linkColor = WebKitSettings::self()->vLinkColor();

        if (linkColor.isValid())
            linkStyle += QString::fromLatin1("a:visited {color: rgb(%1,%2,%3);}\n")
                         .arg(linkColor.red()).arg(linkColor.green()).arg(linkColor.blue());

        linkColor = WebKitSettings::self()->linkColor();
        if (linkColor.isValid())
            linkStyle += QString::fromLatin1("a:active {color: rgb(%1,%2,%3);}\n")
                         .arg(linkColor.red()).arg(linkColor.green()).arg(linkColor.blue());

        if (WebKitSettings::self()->underlineLink())
            linkStyle += QL1S("a:link {text-decoration:underline;}\n");
        else if (WebKitSettings::self()->hoverLink())
            linkStyle += QL1S("a:hover {text-decoration:underline;}\n");

        if (!linkStyle.isEmpty())
            webPage->mainFrame()->documentElement().setAttribute(QL1S("style"), linkStyle);

        if (webView->title().trimmed().isEmpty()) {
            // If the document title is empty, then set it to the current url
            const QString caption = webView->url().toString((QUrl::RemoveQuery|QUrl::RemoveFragment));
            emit q->setWindowCaption(caption);

            // The urlChanged signal is emitted if and only if the main frame
            // receives the title of the page so we manually invoke the slot as
            // a work around here for pages that do not contain it, such as
            // text documents...
            slotUrlChanged(webView->url());
        }

        // Fill form data from wallet...
        KWebWallet *webWallet = webPage->wallet();
        if (webWallet) {
            webWallet->fillFormData(webPage->mainFrame());
            KWebWallet::WebFormList list = webWallet->formsWithCachedData(webPage->mainFrame());
            if (!list.isEmpty()) {
                if (!statusBarWalletLabel) {
                    statusBarWalletLabel = new KUrlLabel(statusBarExtension->statusBar());
                    statusBarWalletLabel->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum));
                    statusBarWalletLabel->setUseCursor(false);
                    statusBarWalletLabel->setPixmap(SmallIcon("wallet-open"));
                    connect(statusBarWalletLabel, SIGNAL(leftClickedUrl()), SLOT(slotLaunchWalletManager()));
                    connect(statusBarWalletLabel, SIGNAL(rightClickedUrl()), SLOT(slotShowWalletMenu()));
                }

                statusBarExtension->addStatusBarItem(statusBarWalletLabel, 0, false);
                hasCachedFormData = true;
            }
        }

        // Set the favicon specified through the <link> tag...
        const QWebElement element = webPage->mainFrame()->findFirstElement(QL1S("head>link[rel=icon]"));
        const QString href = element.attribute("href");
        if (!element.isNull()) {
            kDebug() << "Setting favicon to" << href;
            browserExtension->setIconUrl(KUrl(href));
        }
    }

    /*
      NOTE: Support for stopping meta data redirects is implemented in QtWebKit
      2.0 (Qt 4.7) or greater. See https://bugs.webkit.org/show_bug.cgi?id=29899.
    */
    bool pending = false;
#if QT_VERSION >= 0x040700
    if (webPage->mainFrame()->findAllElements(QL1S("head>meta[http-equiv=refresh]")).count()) {
        if (WebKitSettings::self()->autoPageRefresh())
            pending = true;
        else
            webPage->triggerAction(QWebPage::StopScheduledPageRefresh);
    }
#endif
    emit q->completed((ok && pending));
}

void KWebKitPartPrivate::slotLoadAborted(const KUrl & url)
{
    q->closeUrl();
    if (url.isValid())
      emit browserExtension->openUrlRequest(url);
    else
      q->setUrl(webView->url());
}

void KWebKitPartPrivate::slotUrlChanged(const QUrl& url)
{
    if (!url.isEmpty() && url.scheme() != QL1S("error") && url.toString() != QL1S("about:blank")) {
        q->setUrl(url);
        emit browserExtension->setLocationBarUrl(KUrl(url).prettyUrl());
    }
}

void KWebKitPartPrivate::slotShowSecurity()
{
    if (webPage->sslInfo().isValid()) {
        KSslInfoDialog *dlg = new KSslInfoDialog (q->widget());
        dlg->setSslInfo(webPage->sslInfo().certificateChain(),
                        webPage->sslInfo().peerAddress().toString(),
                        q->url().host(),
                        webPage->sslInfo().protocol(),
                        webPage->sslInfo().ciphers(),
                        webPage->sslInfo().usedChiperBits(),
                        webPage->sslInfo().supportedChiperBits(),
                        KSslInfoDialog::errorsFromString(webPage->sslInfo().certificateErrors()));

        dlg->open();
    } else {
        KMessageBox::information(0, i18n("The SSL information for this site "
                                         "appears to be corrupt."),
                                 i18nc("Secure Sockets Layer", "SSL"));
    }
}

void KWebKitPartPrivate::slotSaveFrameState(QWebFrame *frame, QWebHistoryItem *item)
{
    Q_UNUSED (item);
    if (!frame->parentFrame()) {
        kDebug() << "Update history ?" << emitOpenUrlNotify;
        if (emitOpenUrlNotify)
            emit browserExtension->openUrlNotify();

        // Save the SSL info as the history item meta-data...
        if (item && webPage->sslInfo().isValid())
            item->setUserData(webPage->sslInfo().toMetaData());
    }
}

void KWebKitPartPrivate::slotRestoreFrameState(QWebFrame *frame)
{
    if (!frame->parentFrame())
        emitOpenUrlNotify = true;
}

void KWebKitPartPrivate::slotLinkHovered(const QString &link, const QString &title, const QString &content)
{
    Q_UNUSED(title);
    Q_UNUSED(content);

    QString message;

    if (link.isEmpty()) {
        message = QL1S("");
        emit browserExtension->mouseOverInfo(KFileItem());
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
        } else if (linkUrl.scheme() == QL1S("javascript") &&
                   link.startsWith(QL1S("javascript:window.open"))) {
            message = KStringHandler::rsqueeze(link, 80);
            message += i18n(" (In new window)");
        } else {
            message = link;
            QWebElementCollection collection = webPage->mainFrame()->documentElement().findAll(QL1S("a[href]"));
            QListIterator<QWebFrame *> framesIt (webPage->mainFrame()->childFrames());
            while (framesIt.hasNext()) {
                collection += framesIt.next()->documentElement().findAll(QL1S("a[href]"));
            }

            Q_FOREACH(const QWebElement &element, collection) {
                //kDebug() << "link:" << link << "href" << element.attribute(QL1S("href"));
                if (element.hasAttribute(QL1S("target")) &&
                    link.contains(element.attribute(QL1S("href")), Qt::CaseInsensitive)) {
                    const QString target = element.attribute(QL1S("target")).toLower().simplified();
                    if (target == QL1S("top") || target == QL1S("_blank")) {
                        message += i18n(" (In new window)");
                        break;
                    }
                    else if (target == QL1S("_parent")) {
                        message += i18n(" (In parent frame)");
                        break;
                    }
                }
            }

            KFileItem item (linkUrl, QString(), KFileItem::Unknown);
            emit browserExtension->mouseOverInfo(item);
        }
    }

    emit q->setStatusBarText(message);
}

void KWebKitPartPrivate::slotSearchForText(const QString &text, bool backward)
{
    QWebPage::FindFlags flags = QWebPage::FindWrapsAroundDocument;

    if (backward)
        flags |= QWebPage::FindBackward;

    if (searchBar->caseSensitive())
        flags |= QWebPage::FindCaseSensitively;

    if (searchBar->highlightMatches())
        flags |= QWebPage::HighlightAllOccurrences;

    //kDebug() << "search for text:" << text << ", backward ?" << backward;
    searchBar->setFoundMatch(webView->page()->findText(text, flags));
}

void KWebKitPartPrivate::slotShowSearchBar()
{
    const QString text = webView->selectedText();
    searchBar->setSearchText(text.left(150));
}

void KWebKitPartPrivate::slotLinkMiddleOrCtrlClicked(const KUrl& linkUrl)
{
    KParts::OpenUrlArguments args;
    args.setActionRequestedByUser(true);
    emit browserExtension->createNewWindow(linkUrl, args);
}

void KWebKitPartPrivate::slotSelectionClipboardUrlPasted(const KUrl& selectedUrl)
{
    if (WebKitSettings::self()->isOpenMiddleClickEnabled())
        emit browserExtension->openUrlRequest(selectedUrl);
}

void KWebKitPartPrivate::slotWalletClosed()
{
    if (statusBarWalletLabel) {
        statusBarExtension->removeStatusBarItem(statusBarWalletLabel);
        delete statusBarWalletLabel;
        statusBarWalletLabel = 0;
        hasCachedFormData = false;
    }
}

void KWebKitPartPrivate::slotShowWalletMenu()
{
    KMenu *menu = new KMenu(0);

    if (webView && WebKitSettings::self()->isNonPasswordStorableSite(webView->url().host())) {
      menu->addAction(i18n("&Allow password caching for this site"), this, SLOT(slotDeleteNonPasswordStorableSite()));
    }

    if (webPage && hasCachedFormData) {
      menu->addAction(i18n("Remove all cached passwords for this site"), this, SLOT(slotRemoveCachedPasswords()));
    }

    menu->addSeparator();
    menu->addAction(i18n("&Close Wallet"), this, SLOT(slotWalletClosed()));

    KAcceleratorManager::manage(menu);
    menu->popup(QCursor::pos());
}

void KWebKitPartPrivate::slotLaunchWalletManager()
{
    QDBusInterface r("org.kde.kwalletmanager", "/kwalletmanager/MainWindow_1");
    if (r.isValid()) {
        r.call(QDBus::NoBlock, "show");
    } else {
        KToolInvocation::startServiceByDesktopName("kwalletmanager_show");
    }
}

void KWebKitPartPrivate::slotDeleteNonPasswordStorableSite()
{
    if (webView)
        WebKitSettings::self()->removeNonPasswordStorableSite(webView->url().host());
}

void KWebKitPartPrivate::slotRemoveCachedPasswords()
{
    if (webPage && webPage->wallet()) {
        webPage->wallet()->removeFormData(webPage->mainFrame(), true);
        hasCachedFormData = false;
    }
}

void KWebKitPartPrivate::slotSetTextEncoding(QTextCodec * codec)
{
    if (webPage) {
        QWebSettings *localSettings = webPage->settings();
        if (localSettings) {
            kDebug() << codec->name();
            localSettings->setDefaultTextEncoding(codec->name());
            q->openUrl(q->url());
            //webPage->triggerAction(QWebPage::Reload);
        }
    }
}

#include "kwebkitpart_p.moc"
