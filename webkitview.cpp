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

#include "webkitview.h"
#include "webkitpart.h"
#include "webkitpage.h"

#include <KDE/KParts/GenericFactory>
#include <KDE/KAboutData>
#include <KDE/KAction>
#include <KDE/KActionCollection>
#include <KDE/KConfigGroup>
#include <KMimeType>
#include <KService>
#include <KUriFilterData>
#include <KStandardDirs>
#include <KActionMenu>

#include <QtNetwork/QHttpRequestHeader>
#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebHitTestResult>
#include <QtWebKit/QWebPage>

class WebView::WebViewPrivate
{
public:
    KActionCollection* m_actionCollection;
    QWebHitTestResult result;
};


WebView::WebView(WebKitPart *wpart, QWidget *parent)
    : QWebView(parent), part(wpart), d(new WebView::WebViewPrivate)
{
    setPage(new WebPage(wpart, parent));
    d->m_actionCollection = new KActionCollection(this);
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
    if (!selectedText().isEmpty()) {
        flags |= KParts::BrowserExtension::ShowTextSelectionItems;
        editActionPopupMenu(mapAction);
    }
    emit part->browserExtension()->popupMenu(/*guiclient */
        e->globalPos(), part->url(), 0, KParts::OpenUrlArguments(), KParts::BrowserArguments(),
        flags, mapAction);
}

void WebView::partActionPopupMenu(KParts::BrowserExtension::ActionGroupMap &partGroupMap)
{
    QList<QAction *>partActions;
    KAction *action = new KAction(i18n("Save Image As..."), this);
    d->m_actionCollection->addAction("saveimageas", action);
    connect(action, SIGNAL(triggered(bool)), part->browserExtension(), SLOT(slotSaveImageAs()));
    partActions.append(action);

    action = new KAction(i18n("Send Image..."), this);
    d->m_actionCollection->addAction("sendimage", action);
    connect(action, SIGNAL(triggered(bool)), part->browserExtension(), SLOT(slotSendImage()));
    partActions.append(action);

    action = new KAction(i18n("Copy Image"), this);
    d->m_actionCollection->addAction("copyimage", action);
    action->setEnabled(!d->result.pixmap().isNull());
    connect(action, SIGNAL(triggered(bool)), part->browserExtension(), SLOT(slotCopyImage()));
    partActions.append(action);

    action = new KAction(i18n("View Frame Source"), this);
    d->m_actionCollection->addAction("viewFrameSource", action);
    connect(action, SIGNAL(triggered(bool)), part->browserExtension(), SLOT(slotViewDocumentSource()));
    partActions.append(action);


    partGroupMap.insert("partactions", partActions);
}

void WebView::linkActionPopupMenu(KParts::BrowserExtension::ActionGroupMap &linkGroupMap)
{
    QList<QAction *>linkActions;

    KAction *action = new KAction(i18n("Open in New &Window"), this);
    d->m_actionCollection->addAction("frameinwindow", action);
    action->setIcon(KIcon("window-new"));
    connect(action, SIGNAL(triggered(bool)), part->browserExtension(), SLOT(slotFrameInWindow()));
    linkActions.append(action);

    action = new KAction(i18n("Open in &This Window"), this);
    d->m_actionCollection->addAction("frameintop", action);
    connect(action, SIGNAL(triggered(bool)), part->browserExtension(), SLOT(slotFrameInTop()));
    linkActions.append(action);

    action = new KAction(i18n("Open in &New Tab"), this);
    d->m_actionCollection->addAction("frameintab", action);
    action->setIcon(KIcon("tab-new"));
    connect(action, SIGNAL(triggered(bool)), part->browserExtension(), SLOT(slotFrameInTab()));
    linkActions.append(action);

    linkGroupMap.insert("linkactions", linkActions);
}

void WebView::editActionPopupMenu(KParts::BrowserExtension::ActionGroupMap &editGroupMap)
{
    QList<QAction *>editActions;


    QAction* copyAction = d->m_actionCollection->addAction(KStandardAction::Copy, "copy",  part->browserExtension(), SLOT(copy()));
    copyAction->setText(i18n("&Copy Text"));
    copyAction->setEnabled(part->browserExtension()->isActionEnabled("copy"));
    editActions.append(copyAction);


    // Fill search provider entries
    KConfig config("kuriikwsfilterrc");
    KConfigGroup cg = config.group("General");
    const QString defaultEngine = cg.readEntry("DefaultSearchEngine", "google");
    const char keywordDelimiter = cg.readEntry("KeywordDelimiter", static_cast<int>(':'));

    // search text
    QString selectedText = this->selectedText();
    selectedText.replace("&", "&&");
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

    KAction *action = new KAction(i18n("Search for '%1' with %2", selectedText, name), this);
    d->m_actionCollection->addAction("searchProvider", action);
    editActions.append(action);
    action->setIcon(icon);
    connect(action, SIGNAL(triggered(bool)), part->browserExtension(), SLOT(searchProvider()));

    // favorite search providers
    QStringList favoriteEngines;
    favoriteEngines << "google" << "google_groups" << "google_news" << "webster" << "dmoz" << "wikipedia";
    favoriteEngines = cg.readEntry("FavoriteSearchEngines", favoriteEngines);

    if (!favoriteEngines.isEmpty()) {
        KActionMenu* providerList = new KActionMenu(i18n("Search for '%1' with",  selectedText), this);
        d->m_actionCollection->addAction("searchProviderList", providerList);
        editActions.append(providerList);

        QStringList::ConstIterator it = favoriteEngines.begin();
        for (; it != favoriteEngines.end(); ++it) {
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

                KAction *action = new KAction(name, this);
                d->m_actionCollection->addAction(QString("searchProvider" + searchProviderPrefix).toLatin1().constData(), action);
                action->setIcon(icon);
                connect(action, SIGNAL(triggered(bool)), part->browserExtension(), SLOT(searchProvider()));

                providerList->addAction(action);
            }
        }
    }

    if (selectedText.contains("://") && KUrl(selectedText).isValid()) {
        KAction *action = new KAction(i18n("Open '%1'", selectedText), this);
        d->m_actionCollection->addAction("openSelection", action);
        action->setIcon(KIcon("window-new"));
        connect(action, SIGNAL(triggered(bool)), part->browserExtension(), SLOT(openSelection()));
        editActions.append(action);
    }

    editGroupMap.insert("editactions", editActions);
}
