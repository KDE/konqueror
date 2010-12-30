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

#include "webview.h"
#include "webpage.h"
#include "kwebkitpart.h"
#include "settings/webkitsettings.h"

#include <kio/global.h>
#include <KDE/KParts/GenericFactory>
#include <KDE/KAboutData>
#include <KDE/KAction>
#include <KDE/KActionCollection>
#include <KDE/KConfigGroup>
#include <KDE/KMimeType>
#include <KDE/KService>
#include <KDE/KUriFilter>
#include <KDE/KStandardDirs>
#include <KDE/KActionMenu>
#include <KDE/KIO/AccessManager>
#include <KDE/KStringHandler>

#include <QtNetwork/QHttpRequestHeader>
#include <QtNetwork/QNetworkRequest>
#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebHitTestResult>

#define QL1S(x)   QLatin1String(x)
#define ALTERNATE_DEFAULT_PROVIDER    QL1S("google")
#define ALTERNATE_SEARCH_PROVIDERS    QStringList() << QL1S("google") << QL1S("wikipedia") << QL1S("webster") << QL1S("dmoz")

class WebView::WebViewPrivate
{
public:
    WebViewPrivate() {}
    void addSearchActions(QList<QAction *>& selectActions, QWebView*);

    KActionCollection* actionCollection;
    QWebHitTestResult result;
    QPointer<KWebKitPart> part;
};


WebView::WebView(KWebKitPart *wpart, QWidget *parent)
        :KWebView(parent, false), d(new WebViewPrivate())
{
    d->part = wpart;
    d->actionCollection = new KActionCollection(this);
    setAcceptDrops(true);

    // Create the custom page...
    setPage(new WebPage(wpart, this));
}

WebView::~WebView()
{
    delete d;
}

void WebView::loadUrl(const KUrl &url, const KParts::OpenUrlArguments &args, const KParts::BrowserArguments &bargs)
{
    page()->setProperty("NavigationTypeUrlEntered", true);
    
    if (args.reload()) {
      pageAction(KWebPage::Reload)->trigger();
      return;
    }

    if (bargs.postData.isEmpty()) {
        KWebView::load(QNetworkRequest(url));
    } else {
        KWebView::load(QNetworkRequest(url), QNetworkAccessManager::PostOperation, bargs.postData);
    }
}

QWebHitTestResult WebView::contextMenuResult() const
{
    return d->result;
}

void WebView::contextMenuEvent(QContextMenuEvent *e)
{
    d->result = page()->mainFrame()->hitTestContent(e->pos());
    if (d->result.isContentEditable()) {
        KWebView::contextMenuEvent(e); // TODO: better KDE integration if possible
        return;
    }

    // Clear the previous collection entries first...
    d->actionCollection->clear();    

    KParts::BrowserExtension::PopupFlags flags = KParts::BrowserExtension::DefaultPopupItems;
    flags |= KParts::BrowserExtension::ShowBookmark;
    flags |= KParts::BrowserExtension::ShowReload;
    KParts::BrowserExtension::ActionGroupMap mapAction;
    QString mimeType (QL1S("text/html"));

    KUrl emitUrl;
    if (d->result.linkUrl().isEmpty()) {
        emitUrl = d->part->url();
        if (d->result.isContentSelected()) {
            flags |= KParts::BrowserExtension::ShowTextSelectionItems;
            selectActionPopupMenu(mapAction);
        } else {
            flags |= KParts::BrowserExtension::ShowNavigationItems;          
        }      
    } else {
        flags |= KParts::BrowserExtension::IsLink;
        emitUrl = d->result.linkUrl();
        linkActionPopupMenu(mapAction);
        if (emitUrl.isLocalFile()) {
            mimeType = KMimeType::findByUrl(emitUrl, 0, true, false)->name();
        } else {
            const QString fname(emitUrl.fileName(KUrl::ObeyTrailingSlash));
            if (!fname.isEmpty() && !emitUrl.hasRef() && emitUrl.query().isEmpty())
            {
                KMimeType::Ptr pmt = KMimeType::findByPath(fname, 0, true);

                // Further check for mime types guessed from the extension which,
                // on a web page, are more likely to be a script delivering content
                // of undecidable type. If the mime type from the extension is one
                // of these, don't use it.  Retain the original type 'text/html'.
                if (pmt->name() != KMimeType::defaultMimeType() &&
                    !pmt->is("application/x-perl") &&
                    !pmt->is("application/x-perl-module") &&
                    !pmt->is("application/x-php") &&
                    !pmt->is("application/x-python-bytecode") &&
                    !pmt->is("application/x-python") &&
                    !pmt->is("application/x-shellscript"))
                    mimeType = pmt->name();
            }
        }
    }

    partActionPopupMenu(mapAction);
    KParts::OpenUrlArguments args;
    KParts::BrowserArguments bargs;
    args.setMimeType(mimeType);
    emit d->part->browserExtension()->popupMenu(e->globalPos(), emitUrl, 0, args, bargs, flags, mapAction);
}

void WebView::partActionPopupMenu(KParts::BrowserExtension::ActionGroupMap &partGroupMap)
{
    QList<QAction *>partActions;

    if (d->result.frame()->parentFrame()) {
        KActionMenu * menu = new KActionMenu(i18nc("@title:menu HTML frame/iframe", "Frame"), this);

        KAction * action = new KAction(i18n("Open in New &Window"), this);
        d->actionCollection->addAction("frameinwindow", action);
        action->setIcon(KIcon("window-new"));
        connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(slotFrameInWindow()));
        menu->addAction(action);

        action = new KAction(i18n("Open in &This Window"), this);
        d->actionCollection->addAction("frameintop", action);
        connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(slotFrameInTop()));
        menu->addAction(action);

        action = new KAction(i18n("Open in &New Tab"), this);
        d->actionCollection->addAction("frameintab", action);
        action->setIcon(KIcon("tab-new"));
        connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(slotFrameInTab()));
        menu->addAction(action);

        action = new KAction(d->actionCollection);
        action->setSeparator(true);
        menu->addAction(action);

        action = new KAction(i18n("Reload Frame"), this);
        d->actionCollection->addAction("reloadframe", action);
        connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(slotReloadFrame()));
        menu->addAction(action);

        action = new KAction(i18n("Print Frame..."), this);
        d->actionCollection->addAction("printFrame", action);
        action->setIcon(KIcon("document-print-frame"));
        connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(print()));
        menu->addAction(action);

        action = new KAction(i18n("Save &Frame As..."), this);
        d->actionCollection->addAction("saveFrame", action);
        connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(slotSaveFrame()));
        menu->addAction(action);

        action = new KAction(i18n("View Frame Source"), this);
        d->actionCollection->addAction("viewFrameSource", action);
        connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(slotViewFrameSource()));
        menu->addAction(action);
///TODO Slot not implemented yet
//         action = new KAction(i18n("View Frame Information"), this);
//         d->actionCollection->addAction("viewFrameInfo", action);
//         connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(slotViewPageInfo()));

        action = new KAction(d->actionCollection);
        action->setSeparator(true);
        menu->addAction(action);

        partActions.append(menu);
    }

    if (d->result.imageUrl().isValid()) {
        QAction *action;
        if (!d->actionCollection->action("saveimageas")) {
            action = new KAction(i18n("Save Image As..."), this);
            d->actionCollection->addAction("saveimageas", action);
            connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(slotSaveImageAs()));
        }
        partActions.append(d->actionCollection->action("saveimageas"));

        if (!d->actionCollection->action("sendimage")) {
            action = new KAction(i18n("Send Image..."), this);
            d->actionCollection->addAction("sendimage", action);
            connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(slotSendImage()));
        }
        partActions.append(d->actionCollection->action("sendimage"));

        if (!d->actionCollection->action("copyimage")) {
            action = new KAction(i18n("Copy Image"), this);
            d->actionCollection->addAction("copyimage", action);
            connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(slotCopyImage()));
        }
        action = d->actionCollection->action("copyimage");
        action->setEnabled(!d->result.pixmap().isNull());
        partActions.append(action);

        if (!d->actionCollection->action("viewimage")) {
            action = new KAction(i18n("View Image (%1)", KUrl(d->result.imageUrl()).fileName()), this);
            d->actionCollection->addAction("viewimage", action);
            connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(slotViewImage()));
        }
        partActions.append(d->actionCollection->action("viewimage"));

        if (WebKitSettings::self()->isAdFilterEnabled()) {
            action = new KAction( i18n( "Block Image..." ), this );
            d->actionCollection->addAction( "blockimage", action );
            connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(slotBlockImage()));
            partActions.append(action);

            if (!d->result.imageUrl().host().isEmpty() &&
                !d->result.imageUrl().scheme().isEmpty())
            {
                action = new KAction( i18n( "Block Images From %1" , d->result.imageUrl().host()), this );
                d->actionCollection->addAction( "blockhost", action );
                connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(slotBlockHost()));
                partActions.append(action);
            }
        }
    }

    const bool showDocSourceAction = (d->result.linkUrl().isEmpty() && !d->result.isContentSelected());
    const bool showInspectorAction = settings()->testAttribute(QWebSettings::DeveloperExtrasEnabled);
    
    if (showDocSourceAction || showInspectorAction) {
        QAction *separatorAction = new QAction(this);
        separatorAction->setSeparator(true);
        partActions.append(separatorAction);
    }

    if (showDocSourceAction) {
        QAction* action = d->part->actionCollection()->action("viewDocumentSource");
        partActions.append(action);
    }

    if (showInspectorAction)
        partActions.append(pageAction(QWebPage::InspectElement));

    partGroupMap.insert("partactions", partActions);
}

void WebView::selectActionPopupMenu(KParts::BrowserExtension::ActionGroupMap &selectGroupMap)
{
    QList<QAction *>selectActions;
    if (!d->actionCollection->action("copy")) {
        KAction* copyAction = d->actionCollection->addAction(KStandardAction::Copy, "copy",  d->part->browserExtension(), SLOT(copy()));
        copyAction->setText(i18n("&Copy Text"));
        copyAction->setEnabled(d->part->browserExtension()->isActionEnabled("copy"));
    }
    selectActions.append(d->actionCollection->action("copy"));

    d->addSearchActions(selectActions, this);

    KUriFilterData data (selectedText().simplified().left(256));
    data.setCheckForExecutables(false);
    if (KUriFilter::self()->filterUri(data, QStringList() << "kshorturifilter" << "fixhosturifilter") &&
        data.uri().isValid() && data.uriType() == KUriFilterData::NetProtocol) {
        KAction *action = new KAction(i18nc("open selected url", "Open '%1'",
                                            KStringHandler::rsqueeze(data.uri().url(), 18)), this);
        d->actionCollection->addAction("openSelection", action);
        action->setIcon(KIcon("window-new"));
        action->setData(QUrl(data.uri()));
        connect(action, SIGNAL(triggered(bool)), this, SLOT(openSelection()));
        selectActions.append(action);
    }

    selectGroupMap.insert("editactions", selectActions);
}

void WebView::linkActionPopupMenu(KParts::BrowserExtension::ActionGroupMap &linkGroupMap)
{
    Q_ASSERT(!d->result.linkUrl().isEmpty());

    const KUrl url(d->result.linkUrl());

    QList<QAction *>linkActions;

    KAction *action;
    if (!d->actionCollection->action("copy")) {
        action = d->actionCollection->addAction(KStandardAction::Copy, "copy",  d->part->browserExtension(), SLOT(copy()));
        action->setText(i18n("&Copy Text"));
        action->setEnabled(d->part->browserExtension()->isActionEnabled("copy"));
    }
    linkActions.append(d->actionCollection->action("copy"));

    if (url.protocol() == "mailto") {
        if (!d->actionCollection->action("copylinklocation")) {
            action = new KAction(i18n("&Copy Email Address"), this);
            d->actionCollection->addAction("copylinklocation", action);
            connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(slotCopyLinkLocation()));
        }
        linkActions.append(d->actionCollection->action("copylinklocation"));
    } else {
        if (!d->actionCollection->action("copylinklocation")) {
            action = new KAction(i18n("&Copy Link Address"), this);
            d->actionCollection->addAction("copylinklocation", action);
            connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(slotCopyLinkLocation()));
        }
        linkActions.append(d->actionCollection->action("copylinklocation"));

        if (!d->actionCollection->action("savelinkas")) {
            action = new KAction(i18n("&Save Link As..."), this);
            d->actionCollection->addAction("savelinkas", action);
            connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(slotSaveLinkAs()));
        }
        linkActions.append(d->actionCollection->action("savelinkas"));
    }
    linkGroupMap.insert("linkactions", linkActions);
}

void WebView::WebViewPrivate::addSearchActions(QList<QAction *>& selectActions, QWebView *view)
{
   // search text
    const QString selectedText = view->selectedText().simplified();
    if (selectedText.isEmpty())
        return;

    KUriFilterData data;
    data.setData(selectedText);
    data.setAlternateDefaultSearchProvider(ALTERNATE_DEFAULT_PROVIDER);
    data.setAlternateSearchProviders(ALTERNATE_SEARCH_PROVIDERS);

    if (KUriFilter::self()->filterSearchUri(data, KUriFilter::NormalTextFilter)) {
        const QString squeezedText = KStringHandler::rsqueeze(selectedText, 20);
        KAction *action = new KAction(i18nc("Search \"search provider\" for \"text\"", "Search %1 for '%2'",
                                            data.searchProvider(), squeezedText), view);
        action->setData(QUrl(data.uri()));
        action->setIcon(KIcon(data.iconName()));
        connect(action, SIGNAL(triggered(bool)), part->browserExtension(), SLOT(searchProvider()));
        actionCollection->addAction(QL1S("defaultSearchProvider"), action);
        selectActions.append(action);


        const QStringList preferredSearchProviders = data.preferredSearchProviders();
        if (!preferredSearchProviders.isEmpty()) {
            KActionMenu* providerList = new KActionMenu(i18nc("Search for \"text\" with",
                                                              "Search for '%1' with", squeezedText), view);
            Q_FOREACH(const QString &searchProvider, preferredSearchProviders) {
                if (searchProvider == data.searchProvider())
                    continue;

                KAction *action = new KAction(searchProvider, view);
                action->setData(data.queryForPreferredSearchProvider(searchProvider));
                actionCollection->addAction(searchProvider, action);
                action->setIcon(KIcon(data.iconNameForPreferredSearchProvider(searchProvider)));
                connect(action, SIGNAL(triggered(bool)), part->browserExtension(), SLOT(searchProvider()));

                providerList->addAction(action);
            }
            actionCollection->addAction("searchProviderList", providerList);
            selectActions.append(providerList);
        }
    }
}

void WebView::openSelection()
{
    QAction *action = qobject_cast<KAction*>(sender());
    if (action) {
        KParts::BrowserArguments browserArgs;
        browserArgs.frameName = "_blank";
        emit d->part->browserExtension()->openUrlRequest(action->data().toUrl(), KParts::OpenUrlArguments(), browserArgs);
    }
}
