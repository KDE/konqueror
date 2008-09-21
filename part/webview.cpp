/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
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
#include "webkitpart.h"
#include "webpage.h"

#include <KParts/GenericFactory>
#include <KAboutData>
#include <KAction>
#include <KActionCollection>
#include <KConfigGroup>
#include <KMimeType>
#include <KService>
#include <KUriFilterData>
#include <KStandardDirs>
#include <KActionMenu>

#include <QtNetwork/QHttpRequestHeader>
#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebHitTestResult>

class WebView::WebViewPrivate
{
public:
    WebViewPrivate(WebView *webView)
    : webView(webView)
    {}

    void addSearchActions(QList<QAction *>& selectActions);
    QString selectedTextAsOneLine() const;

    /**
    * Returns selectedText without any leading or trailing whitespace,
    * and with non-breaking-spaces turned into normal spaces.
    *
    * Note that hasSelection can return true and yet simplifiedSelectedText can be empty,
    * e.g. when selecting a single space.
    */
    QString simplifiedSelectedText() const;

    WebView *webView;
    KActionCollection* actionCollection;
    QWebHitTestResult result;
    WebKitPart *part;
};


WebView::WebView(WebKitPart *wpart, QWidget *parent)
    : KWebView(parent), d(new WebViewPrivate(this))
{
    d->part = wpart;
    setPage(new WebPage(wpart, parent));
    d->actionCollection = new KActionCollection(this);
    setAcceptDrops(true);
}

WebView::~WebView()
{
    delete d;
}

QWebHitTestResult WebView::contextMenuResult() const
{
    return d->result;
}

void WebView::contextMenuEvent(QContextMenuEvent *e)
{
    KWebView::contextMenuEvent(e);
    return; // FIXME: remove these two lines as soon as a stable and useful impl. has been done

    d->result = page()->mainFrame()->hitTestContent(e->pos());
    KParts::BrowserExtension::PopupFlags flags = KParts::BrowserExtension::DefaultPopupItems;
    flags |= KParts::BrowserExtension::ShowReload;
    flags |= KParts::BrowserExtension::ShowBookmark;
    flags |= KParts::BrowserExtension::ShowNavigationItems;
    flags |= KParts::BrowserExtension::ShowUrlOperations;

    KParts::BrowserExtension::ActionGroupMap mapAction;
    if (!d->result.linkUrl().isEmpty()) {
        flags |= KParts::BrowserExtension::IsLink;
        linkActionPopupMenu(mapAction);
    }
    if (!d->result.imageUrl().isEmpty()) {
        partActionPopupMenu(mapAction);
    }
    if (d->result.isContentEditable()) {
        KWebView::contextMenuEvent(e); // TODO: better KDE integration if possible
        return;
    }
    if (d->result.isContentSelected()) {
        flags |= KParts::BrowserExtension::ShowTextSelectionItems;
        selectActionPopupMenu(mapAction);
    }

    emit d->part->browserExtension()->popupMenu(/*guiclient */
        e->globalPos(), d->part->url(), 0, KParts::OpenUrlArguments(), KParts::BrowserArguments(),
        flags, mapAction);
}

void WebView::partActionPopupMenu(KParts::BrowserExtension::ActionGroupMap &partGroupMap)
{
    QList<QAction *>partActions;
    KAction *action = new KAction(i18n("Save Image As..."), this);
    d->actionCollection->addAction("saveimageas", action);
    connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(slotSaveImageAs()));
    partActions.append(action);

    action = new KAction(i18n("Send Image..."), this);
    d->actionCollection->addAction("sendimage", action);
    connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(slotSendImage()));
    partActions.append(action);

    action = new KAction(i18n("Copy Image"), this);
    d->actionCollection->addAction("copyimage", action);
    action->setEnabled(!d->result.pixmap().isNull());
    connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(slotCopyImage()));
    partActions.append(action);

    action = new KAction(i18n("View Frame Source"), this);
    d->actionCollection->addAction("viewFrameSource", action);
    connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(slotViewDocumentSource()));
    partActions.append(action);

    partGroupMap.insert("partactions", partActions);
}

void WebView::linkActionPopupMenu(KParts::BrowserExtension::ActionGroupMap &linkGroupMap)
{
    QList<QAction *>linkActions;

    KAction *action = new KAction(i18n("Open in New &Window"), this);
    d->actionCollection->addAction("frameinwindow", action);
    action->setIcon(KIcon("window-new"));
    connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(slotFrameInWindow()));
    linkActions.append(action);

    action = new KAction(i18n("Open in &This Window"), this);
    d->actionCollection->addAction("frameintop", action);
    connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(slotFrameInTop()));
    linkActions.append(action);

    action = new KAction(i18n("Open in &New Tab"), this);
    d->actionCollection->addAction("frameintab", action);
    action->setIcon(KIcon("tab-new"));
    connect(action, SIGNAL(triggered(bool)), d->part->browserExtension(), SLOT(slotFrameInTab()));
    linkActions.append(action);

    linkGroupMap.insert("linkactions", linkActions);
}

void WebView::selectActionPopupMenu(KParts::BrowserExtension::ActionGroupMap &selectGroupMap)
{
    QList<QAction *>selectActions;

    QAction* copyAction = d->actionCollection->addAction(KStandardAction::Copy, "copy",  d->part->browserExtension(), SLOT(copy()));
    copyAction->setText(i18n("&Copy Text"));
    copyAction->setEnabled(d->part->browserExtension()->isActionEnabled("copy"));
    selectActions.append(copyAction);

    d->addSearchActions(selectActions);

    QString selectedTextURL = d->selectedTextAsOneLine();
    if (selectedTextURL.contains("://") && KUrl(selectedTextURL).isValid()) {
        if (selectedTextURL.length() > 18) {
            selectedTextURL.truncate(15);
            selectedTextURL += "...";
        }
        KAction *action = new KAction(i18n("Open '%1'", selectedTextURL), this);
        d->actionCollection->addAction("openSelection", action);
        action->setIcon(KIcon("window-new"));
        connect(action, SIGNAL(triggered(bool)), this, SLOT(openSelection()));
        selectActions.append(action);
    }

    selectGroupMap.insert("editactions", selectActions);
}

void WebView::WebViewPrivate::addSearchActions(QList<QAction *>& selectActions)
{
    // Fill search provider entries
    KConfig config("kuriikwsfilterrc");
    KConfigGroup cg = config.group("General");
    const QString defaultEngine = cg.readEntry("DefaultSearchEngine", "google");
    const char keywordDelimiter = cg.readEntry("KeywordDelimiter", static_cast<int>(':'));

    // search text
    QString selectedText = simplifiedSelectedText();
    if (selectedText.isEmpty())
        return;

    selectedText.replace('&', "&&");
    if (selectedText.length() > 18) {
        selectedText.truncate(15);
        selectedText += "...";
    }

    // default search provider
    KService::Ptr service = KService::serviceByDesktopPath(QString("searchproviders/%1.desktop").arg(defaultEngine));

    // search provider icon
    KIcon icon;
    KUriFilterData data;
    QStringList list;
    data.setData(QString("some keyword"));
    list << "kurisearchfilter" << "kuriikwsfilter";

    QString name;
    if (KUriFilter::self()->filterUri(data, list)) {
        QString iconPath = KStandardDirs::locate("cache", KMimeType::favIconForUrl(data.uri()) + ".png");
        if (iconPath.isEmpty())
            icon = KIcon("edit-find");
        else
            icon = KIcon(QPixmap(iconPath));
        name = service->name();
    } else {
        icon = KIcon("google");
        name = "Google";
    }

    KAction *action = new KAction(i18n("Search for '%1' with %2", selectedText, name), webView);
    actionCollection->addAction("searchProvider", action);
    selectActions.append(action);
    action->setIcon(icon);
    connect(action, SIGNAL(triggered(bool)), part->browserExtension(), SLOT(searchProvider()));

    // favorite search providers
    QStringList favoriteEngines;
    favoriteEngines << "google" << "google_groups" << "google_news" << "webster" << "dmoz" << "wikipedia";
    favoriteEngines = cg.readEntry("FavoriteSearchEngines", favoriteEngines);

    if (!favoriteEngines.isEmpty()) {
        KActionMenu* providerList = new KActionMenu(i18n("Search for '%1' with",  selectedText), webView);
        actionCollection->addAction("searchProviderList", providerList);
        selectActions.append(providerList);

        QStringList::ConstIterator it = favoriteEngines.constBegin();
        for (; it != favoriteEngines.constEnd(); ++it) {
            if (*it == defaultEngine)
                continue;
            service = KService::serviceByDesktopPath(QString("searchproviders/%1.desktop").arg(*it));
            if (!service)
                continue;
            const QString searchProviderPrefix = *(service->property("Keys").toStringList().begin()) + keywordDelimiter;
            data.setData(searchProviderPrefix + "some keyword");

            if (KUriFilter::self()->filterUri(data, list)) {
                QString iconPath = KStandardDirs::locate("cache", KMimeType::favIconForUrl(data.uri()) + ".png");
                if (iconPath.isEmpty())
                    icon = KIcon("edit-find");
                else
                    icon = KIcon(QPixmap(iconPath));
                name = service->name();

                KAction *action = new KAction(name, webView);
                actionCollection->addAction(QString("searchProvider" + searchProviderPrefix).toLatin1().constData(), action);
                action->setIcon(icon);
                connect(action, SIGNAL(triggered(bool)), part->browserExtension(), SLOT(searchProvider()));

                providerList->addAction(action);
            }
        }
    }
}

QString WebView::WebViewPrivate::simplifiedSelectedText() const
{
    QString text = webView->selectedText();
    text.replace(QChar(0xa0), ' ');
    // remove leading and trailing whitespace
    while (!text.isEmpty() && text[0].isSpace())
        text = text.mid(1);
    while (!text.isEmpty() && text[text.length()-1].isSpace())
        text.truncate(text.length()-1);
    return text;
}

QString WebView::WebViewPrivate::selectedTextAsOneLine() const
{
    QString text = this->simplifiedSelectedText();
    // in addition to what simplifiedSelectedText does,
    // remove linefeeds and any whitespace surrounding it (#113177),
    // to get it all in a single line.
    text.remove(QRegExp("[\\s]*\\n+[\\s]*"));
    return text;
}

void WebView::openSelection()
{
    KParts::BrowserArguments browserArgs;
    browserArgs.frameName = "_blank";

    emit d->part->browserExtension()->openUrlRequest(d->selectedTextAsOneLine(), KParts::OpenUrlArguments(), browserArgs);
}
