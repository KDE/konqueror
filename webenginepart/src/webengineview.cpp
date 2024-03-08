/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2007 Trolltech ASA
    SPDX-FileCopyrightText: 2008-2010 Urs Wolfer <uwolfer @ kde.org>
    SPDX-FileCopyrightText: 2008 Laurent Montel <montel@kde.org>
    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "webengineview.h"
#include "webenginepage.h"
#include "webenginepart.h"
#include "webenginepart_ext.h"
#include "settings/webenginesettings.h"
#include "webenginepart_ext.h"
#include "spellcheckermanager.h"
#include "browserinterface.h"

#include <KIO/Global>
#include <KAboutData>
#include <KActionCollection>
#include <KConfigGroup>
#include <KService>
#include <KUriFilter>
#include <KActionMenu>
#include <KStringHandler>
#include <KLocalizedString>
#include <KIO/CommandLauncherJob>

#include <QTimer>
#include <QMimeData>
#include <QDropEvent>
#include <QLabel>
#include <QNetworkRequest>
#include <QToolTip>
#include <QCoreApplication>
#include <QMimeType>
#include <QMimeDatabase>
#include <QMenu>
#include <QActionGroup>

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
         m_horizontalAutoScrollSpeed(0),
         m_spellCheckMenu(nullptr)
{
    setAcceptDrops(true);

    // Create the custom page...
    setPage(new WebEnginePage(part, this));

    connect(this, &QWebEngineView::loadStarted, this, &WebEngineView::slotStopAutoScroll);
    
    if (WebEngineSettings::self()->zoomToDPI())
        setZoomFactor(logicalDpiY() / 96.0f);
}

WebEngineView::~WebEngineView()
{
    //qCDebug(WEBENGINEPART_LOG);
}

void WebEngineView::loadUrl(const QUrl& url, const KParts::OpenUrlArguments& args, const BrowserArguments& bargs)
{
    WebEnginePage *pg = qobject_cast<WebEnginePage*>(page());
    if (!pg) {
        return;
    }

    page()->setProperty("NavigationTypeUrlEntered", true);

    if (args.reload() && url == this->url()) {
      reload();
      return;
    }

//     QNetworkRequest request(url);
//     if (args.reload()) {
//         request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
//     }

    if (bargs.postData.isEmpty()) {
        QWebEngineView::load(url);
    } else {
     //   QWebEngineView::load(url, QNetworkAccessManager::PostOperation, bargs.postData);
    }
}

const QWebEngineContextMenuRequest* WebEngineView::contextMenuResult() const
{
    return result();
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
#if QT_VERSION_MAJOR < 6
    m_result = page()->contextMenuData();
#else
    m_result = lastContextMenuRequest();
#endif

    // Clear the previous collection entries first...
    m_actionCollection->clear();

    KParts::NavigationExtension::PopupFlags flags = KParts::NavigationExtension::DefaultPopupItems;
    KParts::NavigationExtension::ActionGroupMap mapAction;
    QString mimeType (QL1S("text/html"));
    bool forcesNewWindow = false;

    QUrl emitUrl;

    if (result()->isContentEditable()) {
        flags |= KParts::NavigationExtension::ShowTextSelectionItems;
        editableContentActionPopupMenu(mapAction);
    } else if (result()->mediaType() == QWebEngineContextMenuRequest::MediaTypeVideo || result()->mediaType() == QWebEngineContextMenuRequest::MediaTypeAudio) {
        multimediaActionPopupMenu(mapAction);
    } else if (!result()->linkUrl().isValid()) {
        if (result()->mediaType() == QWebEngineContextMenuRequest::MediaTypeImage) {
            emitUrl = result()->mediaUrl();
            extractMimeTypeFor(emitUrl, mimeType);
        } else {
            flags |= KParts::NavigationExtension::ShowBookmark;
            emitUrl = m_part->url();

            if (!result()->selectedText().isEmpty()) {
                flags |= KParts::NavigationExtension::ShowTextSelectionItems;
                selectActionPopupMenu(mapAction);
            }
        }
        partActionPopupMenu(mapAction);
    } else {
        flags |= KParts::NavigationExtension::ShowBookmark;
        flags |= KParts::NavigationExtension::IsLink;
        emitUrl = result()->linkUrl();
        linkActionPopupMenu(mapAction);
        if (emitUrl.isLocalFile())
            mimeType = QMimeDatabase().mimeTypeForUrl(emitUrl).name();
        else
            extractMimeTypeFor(emitUrl, mimeType);
        partActionPopupMenu(mapAction);

        // Show the OpenInThisWindow context menu item
//        forcesNewWindow = (page()->currentFrame() != result()->linkTargetFrame());
    }

    if (!mapAction.isEmpty()) {
        KParts::OpenUrlArguments args;
        BrowserArguments bargs;
        args.setMimeType(mimeType);
        bargs.setForcesNewWindow(forcesNewWindow);
        e->accept();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        emit m_part->browserExtension()->popupMenu(e->globalPos(), emitUrl, static_cast<mode_t>(-1), args, bargs, flags, mapAction);
#else
        emit m_part->browserExtension()->browserPopupMenuFromUrl(e->globalPos(), emitUrl, static_cast<mode_t>(-1), args, bargs, flags, mapAction);
#endif
        return;
    }

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
            // qCDebug(WEBENGINEPART_LOG) << "scroll timer id:" << m_autoScrollTimerId;
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

void WebEngineView::editableContentActionPopupMenu(KParts::NavigationExtension::ActionGroupMap& partGroupMap)
{
    if (m_spellCheckMenu) {
        m_spellCheckMenu->deleteLater();
        m_spellCheckMenu = nullptr;
    }
    QList<QAction*> editableContentActions;

    QActionGroup* group = new QActionGroup(this);
    group->setExclusive(true);

    QAction* action = new QAction(m_actionCollection);
    action->setSeparator(true);
    editableContentActions.append(action);

    WebEngineNavigationExtension *ext = qobject_cast<WebEngineNavigationExtension *>(m_part->navigationExtension());
    Q_ASSERT(ext!=nullptr);

    action = KStandardAction::create(KStandardAction::Copy, ext, &WebEngineNavigationExtension::copy, m_actionCollection);
    action->setEnabled(pageAction(QWebEnginePage::Copy)->isEnabled());
    editableContentActions.append(action);

    action = KStandardAction::create(KStandardAction::Cut, ext, &WebEngineNavigationExtension::cut, m_actionCollection);
    action->setEnabled(pageAction(QWebEnginePage::Cut)->isEnabled());
    editableContentActions.append(action);

    action = KStandardAction::create(KStandardAction::Paste, ext, &WebEngineNavigationExtension::paste, m_actionCollection);
    action->setEnabled(pageAction(QWebEnginePage::Paste)->isEnabled());
    editableContentActions.append(action);

    action = new QAction(m_actionCollection);
    action->setSeparator(true);
    editableContentActions.append(action);

    editableContentActions.append(pageAction(QWebEnginePage::SelectAll));
    editableContentActions.append(pageAction(QWebEnginePage::InspectElement));


#if QT_VERSION_MAJOR < 6
    const QWebEngineContextMenuRequest *req = &page()->contextMenuData();
#else
    QWebEngineContextMenuRequest *req = lastContextMenuRequest();
#endif
    SpellCheckerManager *manager = m_part->spellCheckerManager();
    m_spellCheckMenu = manager->spellCheckingMenu(req->spellCheckerSuggestions(), m_actionCollection, dynamic_cast<WebEnginePage*>(page()));
    if (m_spellCheckMenu) {
        editableContentActions.append(m_spellCheckMenu->menuAction());
    }

    partGroupMap.insert(QStringLiteral("editactions") , editableContentActions);
}

void WebEngineView::partActionPopupMenu(KParts::NavigationExtension::ActionGroupMap& partGroupMap)
{
    QList<QAction*> partActions;

    WebEngineNavigationExtension *ext = qobject_cast<WebEngineNavigationExtension *>(m_part->navigationExtension());
    Q_ASSERT(ext!=nullptr);
    if (result()->mediaUrl().isValid()) {
        QAction *action;
        action = new QAction(i18n("Save Image As..."), this);
        m_actionCollection->addAction(QL1S("saveimageas"), action);
        connect(action, &QAction::triggered, ext, &WebEngineNavigationExtension::slotSaveImageAs);
        partActions.append(action);

        action = new QAction(i18n("Send Image..."), this);
        m_actionCollection->addAction(QL1S("sendimage"), action);
        connect(action, &QAction::triggered, ext, &WebEngineNavigationExtension::slotSendImage);
        partActions.append(action);

        action = new QAction(i18n("Copy Image URL"), this);
        m_actionCollection->addAction(QL1S("copyimageurl"), action);
        connect(action, &QAction::triggered, ext, &WebEngineNavigationExtension::slotCopyImageURL);
        partActions.append(action);

#if 0
        action = new QAction(i18n("Copy Image"), this);
        m_actionCollection->addAction(QL1S("copyimage"), action);
        connect(action, &QAction::triggered, ext, &WebEngineNavigationExtension::slotCopyImage);
        action->setEnabled(!result()->pixmap().isNull());
        partActions.append(action);
#endif

        action = new QAction(i18n("View Image (%1)", QUrl(result()->mediaUrl()).fileName()), this);
        m_actionCollection->addAction(QL1S("viewimage"), action);
        connect(action, &QAction::triggered, ext, &WebEngineNavigationExtension::slotViewImage);
        partActions.append(action);

        if (WebEngineSettings::self()->isAdFilterEnabled()) {
            action = new QAction(i18n("Block Image..."), this);
            m_actionCollection->addAction(QL1S("blockimage"), action);
            connect(action, &QAction::triggered, ext, &WebEngineNavigationExtension::slotBlockImage);
            partActions.append(action);

            if (!result()->mediaUrl().host().isEmpty() &&
                !result()->mediaUrl().scheme().isEmpty())
            {
                action = new QAction(i18n("Block Images From %1" , result()->mediaUrl().host()), this);
                m_actionCollection->addAction(QL1S("blockhost"), action);
                connect(action, &QAction::triggered, ext, &WebEngineNavigationExtension::slotBlockHost);
                partActions.append(action);
            }
        }
    }

    {
        QAction *separatorAction = new QAction(m_actionCollection);
        separatorAction->setSeparator(true);
        partActions.append(separatorAction);
    }

    partActions.append(m_part->actionCollection()->action(QStringLiteral("viewDocumentSource")));

    partActions.append(pageAction(QWebEnginePage::InspectElement));

    partGroupMap.insert(QStringLiteral("partactions"), partActions);
}

void WebEngineView::selectActionPopupMenu(KParts::NavigationExtension::ActionGroupMap& selectGroupMap)
{
    QList<QAction*> selectActions;

    WebEngineNavigationExtension *ext = qobject_cast<WebEngineNavigationExtension *>(m_part->navigationExtension());
    Q_ASSERT(ext!=nullptr);

    QAction* copyAction = KStandardAction::create(KStandardAction::Copy,
                                                  ext, &WebEngineNavigationExtension::copy, m_actionCollection);
    copyAction->setText(i18n("&Copy Text"));
    copyAction->setEnabled(ext->isActionEnabled("copy"));
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
        connect(action, &QAction::triggered, ext, &WebEngineNavigationExtension::slotOpenSelection);
        selectActions.append(action);
    }

    selectGroupMap.insert(QStringLiteral("editactions"), selectActions);
}

void WebEngineView::linkActionPopupMenu(KParts::NavigationExtension::ActionGroupMap& linkGroupMap)
{
    Q_ASSERT(!result()->linkUrl().isEmpty());

    const QUrl url(result()->linkUrl());

    QList<QAction*> linkActions;

    QAction* action;

    WebEngineNavigationExtension *ext = qobject_cast<WebEngineNavigationExtension *>(m_part->navigationExtension());
    Q_ASSERT(ext!=nullptr);

    if (!result()->selectedText().isEmpty()) {
        action = KStandardAction::create(KStandardAction::Copy, ext, &WebEngineNavigationExtension::copy,
                                         m_actionCollection);
        action->setText(i18n("&Copy Text"));
        action->setEnabled(ext->isActionEnabled("copy"));
        linkActions.append(action);
    }

    if (url.scheme() == QLatin1String("mailto")) {
        action = new QAction(i18n("&Copy Email Address"), this);
        m_actionCollection->addAction(QL1S("copylinklocation"), action);
        connect(action, &QAction::triggered, ext, &WebEngineNavigationExtension::slotCopyEmailAddress);
        linkActions.append(action);
    } else {
        if (!result()->linkText().isEmpty()) {
            action = new QAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy Link &Text"), this);
            m_actionCollection->addAction(QL1S("copylinktext"), action);
            connect(action, &QAction::triggered, ext, &WebEngineNavigationExtension::slotCopyLinkText);
            linkActions.append(action);
        }

        action = new QAction(i18n("Copy Link &URL"), this);
        m_actionCollection->addAction(QL1S("copylinkurl"), action);
        connect(action, &QAction::triggered, ext, &WebEngineNavigationExtension::slotCopyLinkURL);
        linkActions.append(action);

        action = new QAction(i18n("&Save Link As..."), this);
        m_actionCollection->addAction(QL1S("savelinkas"), action);
        auto saveLinkAsLambda = [this, url](bool){qobject_cast<WebEngineNavigationExtension*>(m_part->navigationExtension())->slotSaveLinkAs(url);};
        connect(action, &QAction::triggered, m_part->navigationExtension(), saveLinkAsLambda);
        linkActions.append(action);
    }

    linkGroupMap.insert(QStringLiteral("linkactions"), linkActions);
}

void WebEngineView::multimediaActionPopupMenu(KParts::NavigationExtension::ActionGroupMap& mmGroupMap)
{
    QList<QAction*> multimediaActions;

    const bool isVideoElement = result()->mediaType() == QWebEngineContextMenuRequest::MediaTypeVideo;
    const bool isAudioElement = result()->mediaType() == QWebEngineContextMenuRequest::MediaTypeAudio;

    WebEngineNavigationExtension *ext = qobject_cast<WebEngineNavigationExtension *>(m_part->navigationExtension());
    Q_ASSERT(ext!=nullptr);

    QAction* action = new QAction(i18n("&Play/Pause"), this);
    m_actionCollection->addAction(QL1S("playmultimedia"), action);
    connect(action, &QAction::triggered, ext, &WebEngineNavigationExtension::slotPlayMedia);
    multimediaActions.append(action);

    action = new QAction(i18n("Un&mute/&Mute"), this);
    m_actionCollection->addAction(QL1S("mutemultimedia"), action);
    connect(action, &QAction::triggered, ext, &WebEngineNavigationExtension::slotMuteMedia);
    multimediaActions.append(action);

    action = new QAction(i18n("Toggle &Loop"), this);
    m_actionCollection->addAction(QL1S("loopmultimedia"), action);
    connect(action, &QAction::triggered, ext, &WebEngineNavigationExtension::slotLoopMedia);
    multimediaActions.append(action);

    action = new QAction(i18n("Toggle &Controls"), this);
    m_actionCollection->addAction(QL1S("showmultimediacontrols"), action);
    connect(action, &QAction::triggered, ext, &WebEngineNavigationExtension::slotShowMediaControls);
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
    connect(action, &QAction::triggered, ext, &WebEngineNavigationExtension::slotSaveMedia);
    multimediaActions.append(action);

    action = new QAction(copyMediaText, this);
    m_actionCollection->addAction(QL1S("copymultimediaurl"), action);
    connect(action, &QAction::triggered, ext, &WebEngineNavigationExtension::slotCopyMedia);
    multimediaActions.append(action);

    mmGroupMap.insert(QStringLiteral("partactions"), multimediaActions);
}

void WebEngineView::slotConfigureWebShortcuts()
{
    auto job = new KIO::CommandLauncherJob(QStringLiteral("kcmshell%1").arg(KI18N_VERSION_MAJOR), {QStringLiteral("webshortcuts")});
    job->start();
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

    WebEngineNavigationExtension *ext = qobject_cast<WebEngineNavigationExtension *>(m_part->navigationExtension());
    Q_ASSERT(ext!=nullptr);

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
        connect(action, &QAction::triggered, ext, &WebEngineNavigationExtension::searchProvider);
        m_actionCollection->addAction(QL1S("defaultSearchProvider"), action);
        selectActions.append(action);


        const QStringList preferredSearchProviders = data.preferredSearchProviders();
        if (!preferredSearchProviders.isEmpty()) {
            KActionMenu* providerList = new KActionMenu(i18nc("Search for \"text\" with",
                                                              "Search for '%1' with", squeezedText), view);
            for (const QString &searchProvider: preferredSearchProviders) {
                if (searchProvider == data.searchProvider())
                    continue;

                QAction *action = new QAction(QIcon::fromTheme(data.iconNameForPreferredSearchProvider(searchProvider)), searchProvider, view);
                action->setData(data.queryForPreferredSearchProvider(searchProvider));
                m_actionCollection->addAction(searchProvider, action);
                connect(action, &QAction::triggered, ext, &WebEngineNavigationExtension::searchProvider);

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

#ifndef REMOTE_DND_NOT_HANDLED_BY_WEBENGINE
void WebEngineView::dropEvent(QDropEvent* e)
{
    WebEnginePage *pg = qobject_cast<WebEnginePage*>(page());
    if (pg) {
        pg->setDropOperationStarted();
    }
    QWebEngineView::dropEvent(e);
}

#else
void WebEngineView::dropEvent(QDropEvent* e)
{
    QWebEngineView::dropEvent(e);
    //Unlike in acceptProposedDragEventIfPossible, we don't check !e->isAccepted because it seems that it's always true
    //(if the move event was accepted, this is accepted automatically; if the move event was rejected, this function
    //isn't called at all)
    if (!m_dragAndDropHandledBySuperclass && e->mimeData()->hasUrls()) {
        m_dragAndDropHandledBySuperclass = true;
        emit m_part->navigationExtension()->openUrlRequest(e->mimeData()->urls().first());
        e->acceptProposedAction();
    }
}

void WebEngineView::acceptDragMoveEventIfPossible(QDragMoveEvent* e)
{
    if (!e->isAccepted() && e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
        m_dragAndDropHandledBySuperclass = false;
    } else {
        m_dragAndDropHandledBySuperclass = true;
    }
}

void WebEngineView::dragEnterEvent(QDragEnterEvent* e)
{
    QWebEngineView::dragEnterEvent(e);
    acceptDragMoveEventIfPossible(e);
}

void WebEngineView::dragMoveEvent(QDragMoveEvent* e)
{
    QWebEngineView::dragMoveEvent(e);
    acceptDragMoveEventIfPossible(e);
}
#endif

QWebEngineContextMenuRequest* WebEngineView::result()
{
#if QT_VERSION_MAJOR < 6
    return &m_result;
#else
    return m_result;
#endif
}

const QWebEngineContextMenuRequest* WebEngineView::result() const
{
#if QT_VERSION_MAJOR < 6
    return &m_result;
#else
    return m_result;
#endif
}
