/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
 * Copyright (C) 2008 - 2010 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
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

#include "webengineview.h"
#include "webenginepage.h"
#include "webenginepart.h"
#include "settings/webenginesettings.h"

#include <KIO/Global>
#include <KAboutData>
#include <KActionCollection>
#include <KConfigGroup>
#include <KService>
#include <KUriFilter>
#include <KActionMenu>
#include <KIO/AccessManager>
#include <KStringHandler>
#include <KLocalizedString>
#include <KToolInvocation>

#include <QTimer>
#include <QMimeData>
#include <QDropEvent>
#include <QLabel>
#include <QNetworkRequest>
#include <QToolTip>
#include <QCoreApplication>
#include <QMimeType>
#include <QMimeDatabase>

#define QL1S(x)   QLatin1String(x)
#define QL1C(x)   QLatin1Char(x)

#define ALTERNATE_DEFAULT_WEB_SHORTCUT    QL1S("google")
#define ALTERNATE_WEB_SHORTCUTS           QStringList() << QL1S("google") << QL1S("wikipedia") << QL1S("webster") << QL1S("dmoz")

WebEngineView::WebEngineView(WebEnginePart* part, QWidget* parent)
        :QWebEngineView(parent),
         m_actionCollection(new KActionCollection(this)),
         m_part(part),
         m_autoScrollTimerId(-1),
         m_verticalAutoScrollSpeed(0),
         m_horizontalAutoScrollSpeed(0)
{
    setAcceptDrops(true);

    // Create the custom page...
    setPage(new WebEnginePage(part, this));

    connect(this, SIGNAL(loadStarted()), this, SLOT(slotStopAutoScroll()));
    
    if (WebEngineSettings::self()->zoomToDPI())
        setZoomFactor(logicalDpiY() / 96.0f);

#ifndef HAVE_WEBENGINECONTEXTMENUDATA
    m_result = 0;
#endif
}

WebEngineView::~WebEngineView()
{
    //kDebug();
}

void WebEngineView::loadUrl(const QUrl& url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& bargs)
{
    page()->setProperty("NavigationTypeUrlEntered", true);

    if (args.reload() && url == this->url()) {
      reload();
      return;
    }

    QNetworkRequest request(url);
    if (args.reload()) {
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    }

    if (bargs.postData.isEmpty()) {
        QWebEngineView::load(url);
    } else {
     //   QWebEngineView::load(url, QNetworkAccessManager::PostOperation, bargs.postData);
    }
}

QWebEngineContextMenuData WebEngineView::contextMenuResult() const
{
    return m_result;
}

static void extractMimeTypeFor(const QUrl& url, QString& mimeType)
{
    const QString fname(url.fileName());
 
    if (fname.isEmpty() || url.hasFragment() || url.hasQuery())
        return;
 
    QMimeType pmt = QMimeDatabase().mimeTypeForFile(fname);

    // Further check for mime types guessed from the extension which,
    // on a web page, are more likely to be a script delivering content
    // of undecidable type. If the mime type from the extension is one
    // of these, don't use it.  Retain the original type 'text/html'.
    if (pmt.isDefault() ||
        pmt.inherits(QL1S("application/x-perl")) ||
        pmt.inherits(QL1S("application/x-perl-module")) ||
        pmt.inherits(QL1S("application/x-php")) ||
        pmt.inherits(QL1S("application/x-python-bytecode")) ||
        pmt.inherits(QL1S("application/x-python")) ||
        pmt.inherits(QL1S("application/x-shellscript")))
        return;

    mimeType = pmt.name();
}

void WebEngineView::contextMenuEvent(QContextMenuEvent* e)
{
#ifdef HAVE_WEBENGINECONTEXTMENUDATA
    m_result = page()->contextMenuData();

    // Clear the previous collection entries first...
    m_actionCollection->clear();

    KParts::BrowserExtension::PopupFlags flags = KParts::BrowserExtension::DefaultPopupItems;
    KParts::BrowserExtension::ActionGroupMap mapAction;
    QString mimeType (QL1S("text/html"));
    bool forcesNewWindow = false;

    QUrl emitUrl;

    if (m_result.isContentEditable()) {
        flags |= KParts::BrowserExtension::ShowTextSelectionItems;
        editableContentActionPopupMenu(mapAction);
    } else if (m_result.mediaType() == QWebEngineContextMenuData::MediaTypeVideo || m_result.mediaType() == QWebEngineContextMenuData::MediaTypeAudio) {
        multimediaActionPopupMenu(mapAction);
    } else if (!m_result.linkUrl().isValid()) {
        if (m_result.mediaType() == QWebEngineContextMenuData::MediaTypeImage) {
            emitUrl = m_result.mediaUrl();
            extractMimeTypeFor(emitUrl, mimeType);
        } else {
            flags |= KParts::BrowserExtension::ShowBookmark;
            emitUrl = m_part->url();

            if (!m_result.selectedText().isEmpty()) {
                flags |= KParts::BrowserExtension::ShowTextSelectionItems;
                selectActionPopupMenu(mapAction);
            }
        }
        partActionPopupMenu(mapAction);
    } else {
        flags |= KParts::BrowserExtension::ShowBookmark;
        flags |= KParts::BrowserExtension::IsLink;
        emitUrl = m_result.linkUrl();
        linkActionPopupMenu(mapAction);
        if (emitUrl.isLocalFile())
            mimeType = QMimeDatabase().mimeTypeForUrl(emitUrl).name();
        else
            extractMimeTypeFor(emitUrl, mimeType);
        partActionPopupMenu(mapAction);

        // Show the OpenInThisWindow context menu item
//        forcesNewWindow = (page()->currentFrame() != m_result.linkTargetFrame());
    }

    if (!mapAction.isEmpty()) {
        KParts::OpenUrlArguments args;
        KParts::BrowserArguments bargs;
        args.setMimeType(mimeType);
        bargs.setForcesNewWindow(forcesNewWindow);
        e->accept();
        emit m_part->browserExtension()->popupMenu(e->globalPos(), emitUrl, static_cast<mode_t>(-1), args, bargs, flags, mapAction);
        return;
    }

#endif
    QWebEngineView::contextMenuEvent(e);
}

void WebEngineView::keyPressEvent(QKeyEvent* e)
{
    if (e && hasFocus()) {
        const int key = e->key();
        if (e->modifiers() & Qt::ShiftModifier) {
            switch (key) {
            case Qt::Key_Up:
               /* if (!isEditableElement(page()))*/ {
                    m_verticalAutoScrollSpeed--;
                    if (m_autoScrollTimerId == -1)
                        m_autoScrollTimerId = startTimer(100);
                    e->accept();
                    return;
                }
                break;
            case Qt::Key_Down:
                /*if (!isEditableElement(page()))*/ {
                    m_verticalAutoScrollSpeed++;
                    if (m_autoScrollTimerId == -1)
                        m_autoScrollTimerId = startTimer(100);
                    e->accept();
                    return;
                }
                break;
            case Qt::Key_Left:
                /*if (!isEditableElement(page()))*/ {
                    m_horizontalAutoScrollSpeed--;
                    if (m_autoScrollTimerId == -1)
                        m_autoScrollTimerId = startTimer(100);
                    e->accept();
                    return;
                }
                break;
            case Qt::Key_Right:
                /*if (!isEditableElement(page()))*/ {
                    m_horizontalAutoScrollSpeed--;
                    if (m_autoScrollTimerId == -1)
                        m_autoScrollTimerId = startTimer(100);
                    e->accept();
                    return;
                }
                break;
            default:
                break;
            }
        } else if (m_autoScrollTimerId != -1) {
            // kDebug() << "scroll timer id:" << m_autoScrollTimerId;
            slotStopAutoScroll();
            e->accept();
            return;
        }
    }
    QWebEngineView::keyPressEvent(e);
}

void WebEngineView::keyReleaseEvent(QKeyEvent *e)
{
    QWebEngineView::keyReleaseEvent(e);
}

void WebEngineView::mouseReleaseEvent(QMouseEvent* e)
{
    QWebEngineView::mouseReleaseEvent(e);
}

void WebEngineView::wheelEvent (QWheelEvent* e)
{
    QWebEngineView::wheelEvent(e);
}


void WebEngineView::timerEvent(QTimerEvent* e)
{
#if 0
    if (e && e->timerId() == m_autoScrollTimerId) {
        // do the scrolling
        scroll(m_horizontalAutoScrollSpeed, m_verticalAutoScrollSpeed);
        // check if we reached the end
        const int y = page()->scrollPosition().y();
        if (y == page()->currentFrame()->scrollBarMinimum(Qt::Vertical) ||
            y == page()->currentFrame()->scrollBarMaximum(Qt::Vertical)) {
            m_verticalAutoScrollSpeed = 0;
        }

        const int x = page()->scrollPosition().x();
        if (x == page()->currentFrame()->scrollBarMinimum(Qt::Horizontal) ||
            x == page()->currentFrame()->scrollBarMaximum(Qt::Horizontal)) {
            m_horizontalAutoScrollSpeed = 0;
        }

        // Kill the timer once the max/min scroll limit is reached.
        if (m_horizontalAutoScrollSpeed == 0  && m_verticalAutoScrollSpeed == 0) {
            killTimer(m_autoScrollTimerId);
            m_autoScrollTimerId = -1;
        }
        e->accept();
        return;
    }
#endif
    QWebEngineView::timerEvent(e);
}

void WebEngineView::editableContentActionPopupMenu(KParts::BrowserExtension::ActionGroupMap& partGroupMap)
{
    QList<QAction*> editableContentActions;

    QActionGroup* group = new QActionGroup(this);
    group->setExclusive(true);

    QAction* action = new QAction(m_actionCollection);
    action->setSeparator(true);
    editableContentActions.append(action);

    action = m_actionCollection->addAction(KStandardAction::Copy, QL1S("copy"),  m_part->browserExtension(), SLOT(copy()));
    action->setEnabled(pageAction(QWebEnginePage::Copy)->isEnabled());
    editableContentActions.append(action);

    action = m_actionCollection->addAction(KStandardAction::Cut, QL1S("cut"),  m_part->browserExtension(), SLOT(cut()));
    action->setEnabled(pageAction(QWebEnginePage::Cut)->isEnabled());
    editableContentActions.append(action);

    action = m_actionCollection->addAction(KStandardAction::Paste, QL1S("paste"),  m_part->browserExtension(), SLOT(paste()));
    action->setEnabled(pageAction(QWebEnginePage::Paste)->isEnabled());
    editableContentActions.append(action);

    action = new QAction(m_actionCollection);
    action->setSeparator(true);
    editableContentActions.append(action);

    editableContentActions.append(pageAction(QWebEnginePage::SelectAll));
    editableContentActions.append(pageAction(QWebEnginePage::InspectElement));

    partGroupMap.insert(QStringLiteral("editactions") , editableContentActions);
}


void WebEngineView::partActionPopupMenu(KParts::BrowserExtension::ActionGroupMap& partGroupMap)
{
    QList<QAction*> partActions;

#ifdef HAVE_WEBENGINECONTEXTMENUDATA
    if (m_result.mediaUrl().isValid()) {
        QAction *action;
        action = new QAction(i18n("Save Image As..."), this);
        m_actionCollection->addAction(QL1S("saveimageas"), action);
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotSaveImageAs()));
        partActions.append(action);

        action = new QAction(i18n("Send Image..."), this);
        m_actionCollection->addAction(QL1S("sendimage"), action);
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotSendImage()));
        partActions.append(action);

        action = new QAction(i18n("Copy Image URL"), this);
        m_actionCollection->addAction(QL1S("copyimageurl"), action);
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotCopyImageURL()));
        partActions.append(action);

#if 0
        action = new QAction(i18n("Copy Image"), this);
        m_actionCollection->addAction(QL1S("copyimage"), action);
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotCopyImage()));
        action->setEnabled(!m_result.pixmap().isNull());
        partActions.append(action);
#endif

        action = new QAction(i18n("View Image (%1)", QUrl(m_result.mediaUrl()).fileName()), this);
        m_actionCollection->addAction(QL1S("viewimage"), action);
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotViewImage()));
        partActions.append(action);

        if (WebEngineSettings::self()->isAdFilterEnabled()) {
            action = new QAction(i18n("Block Image..."), this);
            m_actionCollection->addAction(QL1S("blockimage"), action);
            connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotBlockImage()));
            partActions.append(action);

            if (!m_result.mediaUrl().host().isEmpty() &&
                !m_result.mediaUrl().scheme().isEmpty())
            {
                action = new QAction(i18n("Block Images From %1" , m_result.mediaUrl().host()), this);
                m_actionCollection->addAction(QL1S("blockhost"), action);
                connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotBlockHost()));
                partActions.append(action);
            }
        }
    }
#endif

    {
        QAction *separatorAction = new QAction(m_actionCollection);
        separatorAction->setSeparator(true);
        partActions.append(separatorAction);
    }

    partActions.append(m_part->actionCollection()->action(QStringLiteral("viewDocumentSource")));

    partActions.append(pageAction(QWebEnginePage::InspectElement));

    partGroupMap.insert(QStringLiteral("partactions"), partActions);
}

void WebEngineView::selectActionPopupMenu(KParts::BrowserExtension::ActionGroupMap& selectGroupMap)
{
    QList<QAction*> selectActions;

    QAction* copyAction = m_actionCollection->addAction(KStandardAction::Copy, QL1S("copy"),  m_part->browserExtension(), SLOT(copy()));
    copyAction->setText(i18n("&Copy Text"));
    copyAction->setEnabled(m_part->browserExtension()->isActionEnabled("copy"));
    selectActions.append(copyAction);

    addSearchActions(selectActions, this);

    KUriFilterData data (selectedText().simplified().left(256));
    data.setCheckForExecutables(false);
    if (KUriFilter::self()->filterUri(data, QStringList() << QStringLiteral("kshorturifilter") << QStringLiteral("fixhosturifilter")) &&
        data.uri().isValid() && data.uriType() == KUriFilterData::NetProtocol) {
        QAction *action = new QAction(QIcon::fromTheme(QStringLiteral("window-new")), i18nc("open selected url", "Open '%1'",
                                            KStringHandler::rsqueeze(data.uri().url(), 18)), this);
        m_actionCollection->addAction(QL1S("openSelection"), action);
        action->setData(QUrl(data.uri()));
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotOpenSelection()));
        selectActions.append(action);
    }

    selectGroupMap.insert(QStringLiteral("editactions"), selectActions);
}

void WebEngineView::linkActionPopupMenu(KParts::BrowserExtension::ActionGroupMap& linkGroupMap)
{
#ifdef HAVE_WEBENGINECONTEXTMENUDATA
    Q_ASSERT(!m_result.linkUrl().isEmpty());

    const QUrl url(m_result.linkUrl());
#else
    const QUrl url;
#endif

    QList<QAction*> linkActions;

    QAction* action;

#ifdef HAVE_WEBENGINECONTEXTMENUDATA
    if (!m_result.selectedText().isEmpty()) {
        action = m_actionCollection->addAction(KStandardAction::Copy, QL1S("copy"),  m_part->browserExtension(), SLOT(copy()));
        action->setText(i18n("&Copy Text"));
        action->setEnabled(m_part->browserExtension()->isActionEnabled("copy"));
        linkActions.append(action);
    }
#endif

    if (url.scheme() == QLatin1String("mailto")) {
#ifdef HAVE_WEBENGINECONTEXTMENUDATA
        action = new QAction(i18n("&Copy Email Address"), this);
        m_actionCollection->addAction(QL1S("copylinklocation"), action);
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotCopyEmailAddress()));
        linkActions.append(action);
#endif
    } else {
#ifdef HAVE_WEBENGINECONTEXTMENUDATA
        if (!m_result.linkText().isEmpty()) {
            action = new QAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy Link &Text"), this);
            m_actionCollection->addAction(QL1S("copylinktext"), action);
            connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotCopyLinkText()));
            linkActions.append(action);
        }
#endif

        action = new QAction(i18n("Copy Link &URL"), this);
        m_actionCollection->addAction(QL1S("copylinkurl"), action);
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotCopyLinkURL()));
        linkActions.append(action);

        action = new QAction(i18n("&Save Link As..."), this);
        m_actionCollection->addAction(QL1S("savelinkas"), action);
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotSaveLinkAs()));
        linkActions.append(action);
    }

    linkGroupMap.insert(QStringLiteral("linkactions"), linkActions);
}

void WebEngineView::multimediaActionPopupMenu(KParts::BrowserExtension::ActionGroupMap& mmGroupMap)
{
#ifdef HAVE_WEBENGINECONTEXTMENUDATA
    QList<QAction*> multimediaActions;

    const bool isVideoElement = m_result.mediaType() == QWebEngineContextMenuData::MediaTypeVideo;
    const bool isAudioElement = m_result.mediaType() == QWebEngineContextMenuData::MediaTypeAudio;

    QAction* action = new QAction(i18n("&Play/Pause"), this);
    m_actionCollection->addAction(QL1S("playmultimedia"), action);
    connect(action, SIGNAL(triggered()), m_part->browserExtension(), SLOT(slotPlayMedia()));
    multimediaActions.append(action);

    action = new QAction(i18n("Un&mute/&Mute"), this);
    m_actionCollection->addAction(QL1S("mutemultimedia"), action);
    connect(action, SIGNAL(triggered()), m_part->browserExtension(), SLOT(slotMuteMedia()));
    multimediaActions.append(action);

    action = new QAction(i18n("Toggle &Loop"), this);
    m_actionCollection->addAction(QL1S("loopmultimedia"), action);
    connect(action, SIGNAL(triggered()), m_part->browserExtension(), SLOT(slotLoopMedia()));
    multimediaActions.append(action);

    action = new QAction(i18n("Toggle &Controls"), this);
    m_actionCollection->addAction(QL1S("showmultimediacontrols"), action);
    connect(action, SIGNAL(triggered()), m_part->browserExtension(), SLOT(slotShowMediaControls()));
    multimediaActions.append(action);

    action = new QAction(m_actionCollection);
    action->setSeparator(true);
    multimediaActions.append(action);

    QString saveMediaText, copyMediaText;
    if (isVideoElement) {
        saveMediaText = i18n("Sa&ve Video As...");
        copyMediaText = i18n("C&opy Video URL");
    } else if (isAudioElement) {
        saveMediaText = i18n("Sa&ve Audio As...");
        copyMediaText = i18n("C&opy Audio URL");
    } else {
        saveMediaText = i18n("Sa&ve Media As...");
        copyMediaText = i18n("C&opy Media URL");
    }

    action = new QAction(saveMediaText, this);
    m_actionCollection->addAction(QL1S("savemultimedia"), action);
    connect(action, SIGNAL(triggered()), m_part->browserExtension(), SLOT(slotSaveMedia()));
    multimediaActions.append(action);

    action = new QAction(copyMediaText, this);
    m_actionCollection->addAction(QL1S("copymultimediaurl"), action);
    connect(action, SIGNAL(triggered()), m_part->browserExtension(), SLOT(slotCopyMedia()));
    multimediaActions.append(action);

    mmGroupMap.insert(QStringLiteral("partactions"), multimediaActions);
#endif
}

void WebEngineView::slotConfigureWebShortcuts()
{
    KToolInvocation::kdeinitExec(QStringLiteral("kcmshell5"),
                                 QStringList() << QStringLiteral("webshortcuts"));
}

void WebEngineView::slotStopAutoScroll()
{
    if (m_autoScrollTimerId == -1) {
        return;
    }

    killTimer(m_autoScrollTimerId);
    m_autoScrollTimerId = -1;
    m_verticalAutoScrollSpeed = 0;
    m_horizontalAutoScrollSpeed = 0;

}

void WebEngineView::addSearchActions(QList<QAction*>& selectActions, QWebEngineView* view)
{
   // search text
    const QString selectedText = view->selectedText().simplified();
    if (selectedText.isEmpty())
        return;

    KUriFilterData data;
    data.setData(selectedText);
    data.setAlternateDefaultSearchProvider(ALTERNATE_DEFAULT_WEB_SHORTCUT);
    data.setAlternateSearchProviders(ALTERNATE_WEB_SHORTCUTS);

    if (KUriFilter::self()->filterSearchUri(data, KUriFilter::NormalTextFilter)) {
        const QString squeezedText = KStringHandler::rsqueeze(selectedText, 20);
        QAction *action = new QAction(QIcon::fromTheme(data.iconName()),
                                      i18nc("Search \"search provider\" for \"text\"", "Search %1 for '%2'",
                                            data.searchProvider(), squeezedText), view);
        action->setData(QUrl(data.uri()));
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(searchProvider()));
        m_actionCollection->addAction(QL1S("defaultSearchProvider"), action);
        selectActions.append(action);


        const QStringList preferredSearchProviders = data.preferredSearchProviders();
        if (!preferredSearchProviders.isEmpty()) {
            KActionMenu* providerList = new KActionMenu(i18nc("Search for \"text\" with",
                                                              "Search for '%1' with", squeezedText), view);
            Q_FOREACH(const QString &searchProvider, preferredSearchProviders) {
                if (searchProvider == data.searchProvider())
                    continue;

                QAction *action = new QAction(QIcon::fromTheme(data.iconNameForPreferredSearchProvider(searchProvider)), searchProvider, view);
                action->setData(data.queryForPreferredSearchProvider(searchProvider));
                m_actionCollection->addAction(searchProvider, action);
                connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(searchProvider()));

                providerList->addAction(action);
            }

            QAction *action = new QAction(i18n("Configure Web Shortcuts..."), view);
            action->setIcon(QIcon::fromTheme(QStringLiteral("configure")));
            connect(action, &QAction::triggered, this, &WebEngineView::slotConfigureWebShortcuts);
            providerList->addAction(action);

            m_actionCollection->addAction(QL1S("searchProviderList"), providerList);
            selectActions.append(providerList);
        }
    }
}
