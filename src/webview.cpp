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

#include "webview.h"
#include "webpage.h"
#include "kwebkitpart.h"
#include "settings/webkitsettings.h"

#include <kio/global.h>
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
#include <KDE/KDebug>

#include <QTimer>
#include <QMimeData>
#include <QDropEvent>
#include <QLabel>
#include <QNetworkRequest>
#include <QWebFrame>
#include <QWebElement>
#include <QWebHitTestResult>
#include <QWebInspector>
#include <QToolTip>
#include <QCoreApplication>
#include <unistd.h>

#define QL1S(x)   QLatin1String(x)
#define QL1C(x)   QLatin1Char(x)

#define ALTERNATE_DEFAULT_WEB_SHORTCUT    QL1S("google")
#define ALTERNATE_WEB_SHORTCUTS           QStringList() << QL1S("google") << QL1S("wikipedia") << QL1S("webster") << QL1S("dmoz")

WebView::WebView(KWebKitPart* part, QWidget* parent)
        :KWebView(parent, false),
         m_actionCollection(new KActionCollection(this)),
         m_part(part),
         m_webInspector(0),
         m_autoScrollTimerId(-1),
         m_verticalAutoScrollSpeed(0),
         m_horizontalAutoScrollSpeed(0),
         m_accessKeyActivated(NotActivated)
{
    setAcceptDrops(true);

    // Create the custom page...
    setPage(new WebPage(part, this));

    connect(this, SIGNAL(loadStarted()), this, SLOT(slotStopAutoScroll()));
    connect(this, SIGNAL(loadStarted()), this, SLOT(hideAccessKeys()));
    connect(page(), SIGNAL(scrollRequested(int,int,QRect)), this, SLOT(hideAccessKeys()));
}

WebView::~WebView()
{
    //kDebug();
}

void WebView::loadUrl(const KUrl& url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& bargs)
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
        KWebView::load(request);
    } else {
        KWebView::load(request, QNetworkAccessManager::PostOperation, bargs.postData);
    }
}

QWebHitTestResult WebView::contextMenuResult() const
{
    return m_result;
}

static bool isMultimediaElement(const QWebElement& element)
{
    if (element.tagName().compare(QL1S("video"), Qt::CaseInsensitive) == 0)
        return true;

    if (element.tagName().compare(QL1S("audio"), Qt::CaseInsensitive) == 0)
        return true;

    return false;
}

static void extractMimeTypeFor(const KUrl& url, QString& mimeType)
{
    const QString fname(url.fileName(KUrl::ObeyTrailingSlash));
 
    if (fname.isEmpty() || url.hasRef() || url.hasQuery())
        return;
 
    KMimeType::Ptr pmt = KMimeType::findByPath(fname, 0, true);

    // Further check for mime types guessed from the extension which,
    // on a web page, are more likely to be a script delivering content
    // of undecidable type. If the mime type from the extension is one
    // of these, don't use it.  Retain the original type 'text/html'.
    if (pmt->name() == KMimeType::defaultMimeType() ||
        pmt->is(QL1S("application/x-perl")) ||
        pmt->is(QL1S("application/x-perl-module")) ||
        pmt->is(QL1S("application/x-php")) ||
        pmt->is(QL1S("application/x-python-bytecode")) ||
        pmt->is(QL1S("application/x-python")) ||
        pmt->is(QL1S("application/x-shellscript")))
        return;

    mimeType = pmt->name();
}

void WebView::dropEvent(QDropEvent* ev)
{
#if (QTWEBKIT_VERSION < QTWEBKIT_VERSION_CHECK(2, 2, 0))
    // TODO: Workaround that should be removed one the bug in QtWebKit is fixed.
    // See https://bugs.webkit.org/show_bug.cgi?id=53320.
    if (ev) {
        QWebHitTestResult hitResult = page()->currentFrame()->hitTestContent(ev->pos());
        if (hitResult.isNull() || hitResult.element().tagName() != QL1S("input")) {
            const QMimeData* mimeData = ev ? ev->mimeData() : 0;
            if (mimeData && mimeData->hasUrls()) {
                KUrl url (mimeData->urls().first());
                emit m_part->browserExtension()->openUrlRequest(url);
                ev->accept();
                return;
            }
        }
    }
#else
    QWebView::dropEvent(ev);
#endif
}

void WebView::contextMenuEvent(QContextMenuEvent* e)
{
    m_result = page()->mainFrame()->hitTestContent(e->pos());

    // Clear the previous collection entries first...
    m_actionCollection->clear();

    KParts::BrowserExtension::PopupFlags flags = KParts::BrowserExtension::DefaultPopupItems;
    KParts::BrowserExtension::ActionGroupMap mapAction;
    QString mimeType (QL1S("text/html"));
    bool forcesNewWindow = false;

    KUrl emitUrl;
    if (m_result.isContentEditable()) {
        if (m_result.element().hasAttribute(QL1S("disabled"))) {
            e->accept();
            return;
        }
        flags |= KParts::BrowserExtension::ShowTextSelectionItems;
        editableContentActionPopupMenu(mapAction);
    } else if (isMultimediaElement(m_result.element())) {
        multimediaActionPopupMenu(mapAction);
    } else if (!m_result.linkUrl().isValid()) {
        if (m_result.imageUrl().isValid()) {
            emitUrl = m_result.imageUrl();
            extractMimeTypeFor(emitUrl, mimeType);
        } else {
            flags |= KParts::BrowserExtension::ShowBookmark;
            flags |= KParts::BrowserExtension::ShowReload;
            emitUrl = m_part->url();

            if (m_result.isContentSelected()) {
                flags |= KParts::BrowserExtension::ShowTextSelectionItems;
                selectActionPopupMenu(mapAction);
            } else {
                flags |= KParts::BrowserExtension::ShowNavigationItems;
            }
        }
        partActionPopupMenu(mapAction);
    } else {
        flags |= KParts::BrowserExtension::ShowBookmark;
        flags |= KParts::BrowserExtension::ShowReload;
        flags |= KParts::BrowserExtension::IsLink;
        emitUrl = m_result.linkUrl();
        linkActionPopupMenu(mapAction);
        if (emitUrl.isLocalFile())
            mimeType = KMimeType::findByUrl(emitUrl, 0, true, false)->name();
        else
            extractMimeTypeFor(emitUrl, mimeType);
        partActionPopupMenu(mapAction);

        // Show the OpenInThisWindow context menu item
        forcesNewWindow = (page()->currentFrame() != m_result.linkTargetFrame());
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

    KWebView::contextMenuEvent(e);
}

static bool isEditableElement(QWebPage* page)
{
    const QWebFrame* frame = (page ? page->currentFrame() : 0);
    QWebElement element = (frame ? frame->findFirstElement(QL1S(":focus")) : QWebElement());
    if (!element.isNull()) {
        const QString tagName(element.tagName());
        if (tagName.compare(QL1S("textarea"), Qt::CaseInsensitive) == 0) {
            return true;
        }
        const QString type(element.attribute(QL1S("type")).toLower());
        if (tagName.compare(QL1S("input"), Qt::CaseInsensitive) == 0
            && (type.isEmpty() || type == QL1S("text") || type == QL1S("password"))) {
            return true;
        }
        if (element.evaluateJavaScript("this.isContentEditable").toBool()) {
            return true;
        }
    }
    return false;
}

void WebView::keyPressEvent(QKeyEvent* e)
{
    if (e && hasFocus()) {
        const int key = e->key();
        if (WebKitSettings::self()->accessKeysEnabled()) {
            if (m_accessKeyActivated == Activated) {
                if (checkForAccessKey(e)) {
                    hideAccessKeys();
                    e->accept();
                    return;
                }
                hideAccessKeys();
            } else if (e->key() == Qt::Key_Control && e->modifiers() == Qt::ControlModifier && !isEditableElement(page())) {
                m_accessKeyActivated = PreActivated; // Only preactive here, it will be actually activated in key release.
            }
        }

        if (e->modifiers() & Qt::ShiftModifier) {
            switch (key) {
            case Qt::Key_Up:
                if (!isEditableElement(page())) {
                    m_verticalAutoScrollSpeed--;
                    if (m_autoScrollTimerId == -1)
                        m_autoScrollTimerId = startTimer(100);
                    e->accept();
                    return;
                }
                break;
            case Qt::Key_Down:
                if (!isEditableElement(page())) {
                    m_verticalAutoScrollSpeed++;
                    if (m_autoScrollTimerId == -1)
                        m_autoScrollTimerId = startTimer(100);
                    e->accept();
                    return;
                }
                break;
            case Qt::Key_Left:
                if (!isEditableElement(page())) {
                    m_horizontalAutoScrollSpeed--;
                    if (m_autoScrollTimerId == -1)
                        m_autoScrollTimerId = startTimer(100);
                    e->accept();
                    return;
                }
                break;
            case Qt::Key_Right:
                if (!isEditableElement(page())) {
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
    KWebView::keyPressEvent(e);
}

void WebView::keyReleaseEvent(QKeyEvent *e)
{
    if (WebKitSettings::self()->accessKeysEnabled() && m_accessKeyActivated == PreActivated) {
        // Activate only when the CTRL key is pressed and released by itself.
        if (e->key() == Qt::Key_Control && e->modifiers() == Qt::NoModifier) {
            showAccessKeys();
            emit statusBarMessage(i18n("Access keys activated"));
            m_accessKeyActivated = Activated;
        } else {
            m_accessKeyActivated = NotActivated;
        }
    }
    KWebView::keyReleaseEvent(e);
}

void WebView::mouseReleaseEvent(QMouseEvent* e)
{
    if (WebKitSettings::self()->accessKeysEnabled() && m_accessKeyActivated == PreActivated
        && e->button() != Qt::NoButton && (e->modifiers() & Qt::ControlModifier)) {
        m_accessKeyActivated = NotActivated;
    }
    KWebView::mouseReleaseEvent(e);
}

void WebView::wheelEvent (QWheelEvent* e)
{
    if (WebKitSettings::self()->accessKeysEnabled() &&
        m_accessKeyActivated == PreActivated &&
        (e->modifiers() & Qt::ControlModifier)) {
        m_accessKeyActivated = NotActivated;
    }
    KWebView::wheelEvent(e);
}


void WebView::timerEvent(QTimerEvent* e)
{
    if (e && e->timerId() == m_autoScrollTimerId) {
        // do the scrolling
        page()->currentFrame()->scroll(m_horizontalAutoScrollSpeed, m_verticalAutoScrollSpeed);

        // check if we reached the end
        const int y = page()->currentFrame()->scrollPosition().y();
        if (y == page()->currentFrame()->scrollBarMinimum(Qt::Vertical) ||
            y == page()->currentFrame()->scrollBarMaximum(Qt::Vertical)) {
            m_verticalAutoScrollSpeed = 0;
        }

        const int x = page()->currentFrame()->scrollPosition().x();
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
    KWebView::timerEvent(e);
}

static bool showSpellCheckAction(const QWebElement& element)
{
    if (element.hasAttribute(QL1S("readonly")))
        return false;

    if (element.attribute(QL1S("spellcheck"), QL1S("true")).compare(QL1S("false"), Qt::CaseInsensitive) == 0)
        return false;

    if (element.hasAttribute(QL1S("type"))
        && element.attribute(QL1S("type")).compare(QL1S("text"), Qt::CaseInsensitive) != 0)
        return false;

    return true;
}

void WebView::editableContentActionPopupMenu(KParts::BrowserExtension::ActionGroupMap& partGroupMap)
{
    QList<QAction*> editableContentActions;

    KActionMenu* menu = new KActionMenu(i18nc("Text direction", "Direction"), this);
    QActionGroup* group = new QActionGroup(this);
    group->setExclusive(true);

    KAction *action = m_actionCollection->addAction(QL1S("text-direction-default"),  m_part->browserExtension(), SLOT(slotTextDirectionChanged()));
    action->setText(i18n("Default"));
    action->setCheckable(true);
    action->setData(QWebPage::SetTextDirectionDefault);
    action->setEnabled(pageAction(QWebPage::SetTextDirectionDefault)->isEnabled());
    action->setCheckable(pageAction(QWebPage::SetTextDirectionDefault)->isChecked());
    action->setActionGroup(group);
    menu->addAction(action);

    action = m_actionCollection->addAction(QL1S("text-direction-left-to-right"),  m_part->browserExtension(), SLOT(slotTextDirectionChanged()));
    action->setText(i18n("Left to right"));
    action->setCheckable(true);
    action->setData(QWebPage::SetTextDirectionLeftToRight);
    action->setEnabled(pageAction(QWebPage::SetTextDirectionLeftToRight)->isEnabled());
    action->setChecked(pageAction(QWebPage::SetTextDirectionLeftToRight)->isChecked());
    action->setActionGroup(group);
    menu->addAction(action);

    action = m_actionCollection->addAction(QL1S("text-direction-right-to-left"),  m_part->browserExtension(), SLOT(slotTextDirectionChanged()));
    action->setText(i18n("Right to left"));
    action->setCheckable(true);
    action->setData(QWebPage::SetTextDirectionRightToLeft);
    action->setEnabled(pageAction(QWebPage::SetTextDirectionRightToLeft)->isEnabled());
    action->setChecked(pageAction(QWebPage::SetTextDirectionRightToLeft)->isChecked());
    action->setActionGroup(group);
    menu->addAction(action);

    editableContentActions.append(menu);

    action = new KAction(m_actionCollection);
    action->setSeparator(true);
    editableContentActions.append(action);

    action = m_actionCollection->addAction(KStandardAction::Copy, QL1S("copy"),  m_part->browserExtension(), SLOT(copy()));
    action->setEnabled(pageAction(QWebPage::Copy)->isEnabled());
    editableContentActions.append(action);

    action = m_actionCollection->addAction(KStandardAction::Cut, QL1S("cut"),  m_part->browserExtension(), SLOT(cut()));
    action->setEnabled(pageAction(QWebPage::Cut)->isEnabled());
    editableContentActions.append(action);

    action = m_actionCollection->addAction(KStandardAction::Paste, QL1S("paste"),  m_part->browserExtension(), SLOT(paste()));
    action->setEnabled(pageAction(QWebPage::Paste)->isEnabled());
    editableContentActions.append(action);

    action = new KAction(m_actionCollection);
    action->setSeparator(true);
    editableContentActions.append(action);

    const bool hasContent = (!m_result.element().evaluateJavaScript(QL1S("this.value")).toString().isEmpty());
    action = m_actionCollection->addAction(KStandardAction::SelectAll, QL1S("selectall"),  m_part->browserExtension(), SLOT(slotSelectAll()));
    action->setEnabled((pageAction(QWebPage::SelectAll)->isEnabled() && hasContent));
    editableContentActions.append(action);

    if (showSpellCheckAction(m_result.element())) {
        action = new KAction(m_actionCollection);
        action->setSeparator(true);
        editableContentActions.append(action);
        action = m_actionCollection->addAction(KStandardAction::Spelling, QL1S("spelling"), m_part->browserExtension(), SLOT(slotCheckSpelling()));
        action->setText(i18n("Check Spelling..."));
        action->setEnabled(hasContent);
        editableContentActions.append(action);

        const bool hasSelection = (hasContent && m_result.isContentSelected());
        action = m_actionCollection->addAction(KStandardAction::Spelling, QL1S("spellcheckSelection"), m_part->browserExtension(), SLOT(slotSpellCheckSelection()));
        action->setText(i18n("Spellcheck selection..."));
        action->setEnabled(hasSelection);
        editableContentActions.append(action);
    }

    if (settings()->testAttribute(QWebSettings::DeveloperExtrasEnabled)) {
        if (!m_webInspector) {
            m_webInspector = new QWebInspector;
            m_webInspector->setPage(page());
            connect(page(), SIGNAL(destroyed()), m_webInspector, SLOT(deleteLater()));
        }
        action = new KAction(m_actionCollection);
        action->setSeparator(true);
        editableContentActions.append(action);
        editableContentActions.append(pageAction(QWebPage::InspectElement));
    }

    partGroupMap.insert("editactions" , editableContentActions);
}


void WebView::partActionPopupMenu(KParts::BrowserExtension::ActionGroupMap& partGroupMap)
{
    QList<QAction*> partActions;

    if (m_result.imageUrl().isValid()) {
        KAction *action;
        action = new KAction(i18n("Save Image As..."), this);
        m_actionCollection->addAction(QL1S("saveimageas"), action);
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotSaveImageAs()));
        partActions.append(action);

        action = new KAction(i18n("Send Image..."), this);
        m_actionCollection->addAction(QL1S("sendimage"), action);
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotSendImage()));
        partActions.append(action);

        action = new KAction(i18n("Copy Image URL"), this);
        m_actionCollection->addAction(QL1S("copyimageurl"), action);
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotCopyImageURL()));
        partActions.append(action);

        action = new KAction(i18n("Copy Image"), this);
        m_actionCollection->addAction(QL1S("copyimage"), action);
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotCopyImage()));
        action->setEnabled(!m_result.pixmap().isNull());
        partActions.append(action);

        action = new KAction(i18n("View Image (%1)", KUrl(m_result.imageUrl()).fileName()), this);
        m_actionCollection->addAction(QL1S("viewimage"), action);
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotViewImage()));
        partActions.append(action);

        if (WebKitSettings::self()->isAdFilterEnabled()) {
            action = new KAction(i18n("Block Image..."), this);
            m_actionCollection->addAction(QL1S("blockimage"), action);
            connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotBlockImage()));
            partActions.append(action);

            if (!m_result.imageUrl().host().isEmpty() &&
                !m_result.imageUrl().scheme().isEmpty())
            {
                action = new KAction(i18n("Block Images From %1" , m_result.imageUrl().host()), this);
                m_actionCollection->addAction(QL1S("blockhost"), action);
                connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotBlockHost()));
                partActions.append(action);
            }
        }
    } else if (m_result.frame() && m_result.frame()->parentFrame() && !m_result.isContentSelected() && m_result.linkUrl().isEmpty()) {
        KActionMenu * menu = new KActionMenu(i18nc("@title:menu HTML frame/iframe", "Frame"), this);

        KAction* action = new KAction(KIcon("window-new"), i18n("Open in New &Window"), this);
        m_actionCollection->addAction(QL1S("frameinwindow"), action);
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotFrameInWindow()));
        menu->addAction(action);

        action = new KAction(i18n("Open in &This Window"), this);
        m_actionCollection->addAction(QL1S("frameintop"), action);
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotFrameInTop()));
        menu->addAction(action);

        action = new KAction(KIcon("tab-new"), i18n("Open in &New Tab"), this);
        m_actionCollection->addAction(QL1S("frameintab"), action);
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotFrameInTab()));
        menu->addAction(action);

        action = new KAction(m_actionCollection);
        action->setSeparator(true);
        menu->addAction(action);

        action = new KAction(i18n("Reload Frame"), this);
        m_actionCollection->addAction(QL1S("reloadframe"), action);
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotReloadFrame()));
        menu->addAction(action);

        action = new KAction(KIcon("document-print-frame"), i18n("Print Frame..."), this);
        m_actionCollection->addAction(QL1S("printFrame"), action);
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(print()));
        menu->addAction(action);

        action = new KAction(i18n("Save &Frame As..."), this);
        m_actionCollection->addAction(QL1S("saveFrame"), action);
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotSaveFrame()));
        menu->addAction(action);

        action = new KAction(i18n("View Frame Source"), this);
        m_actionCollection->addAction(QL1S("viewFrameSource"), action);
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotViewFrameSource()));
        menu->addAction(action);
///TODO Slot not implemented yet
//         action = new KAction(i18n("View Frame Information"), this);
//         m_actionCollection->addAction(QL1S("viewFrameInfo"), action);
//         connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotViewPageInfo()));

        action = new KAction(m_actionCollection);
        action->setSeparator(true);
        menu->addAction(action);

        if (WebKitSettings::self()->isAdFilterEnabled()) {
            action = new KAction(i18n("Block IFrame..."), this);
            m_actionCollection->addAction(QL1S("blockiframe"), action);
            connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotBlockIFrame()));
            menu->addAction(action);
        }

        partActions.append(menu);
    }

    const bool showDocSourceAction = (!m_result.linkUrl().isValid() &&
                                      !m_result.imageUrl().isValid() &&
                                      !m_result.isContentSelected());
    const bool showInspectorAction = settings()->testAttribute(QWebSettings::DeveloperExtrasEnabled);

    if (showDocSourceAction || showInspectorAction) {
        KAction *separatorAction = new KAction(m_actionCollection);
        separatorAction->setSeparator(true);
        partActions.append(separatorAction);
    }

    if (showDocSourceAction)
        partActions.append(m_part->actionCollection()->action("viewDocumentSource"));

    if (showInspectorAction) {
        if (!m_webInspector) {
            m_webInspector = new QWebInspector;
            m_webInspector->setPage(page());
            connect(page(), SIGNAL(destroyed()), m_webInspector, SLOT(deleteLater()));
        }
        partActions.append(pageAction(QWebPage::InspectElement));
    } else {
        if (m_webInspector) {
            delete m_webInspector;
            m_webInspector = 0;
        }
    }

    partGroupMap.insert("partactions", partActions);
}

void WebView::selectActionPopupMenu(KParts::BrowserExtension::ActionGroupMap& selectGroupMap)
{
    QList<QAction*> selectActions;

    KAction* copyAction = m_actionCollection->addAction(KStandardAction::Copy, QL1S("copy"),  m_part->browserExtension(), SLOT(copy()));
    copyAction->setText(i18n("&Copy Text"));
    copyAction->setEnabled(m_part->browserExtension()->isActionEnabled("copy"));
    selectActions.append(copyAction);

    addSearchActions(selectActions, this);

    KUriFilterData data (selectedText().simplified().left(256));
    data.setCheckForExecutables(false);
    if (KUriFilter::self()->filterUri(data, QStringList() << "kshorturifilter" << "fixhosturifilter") &&
        data.uri().isValid() && data.uriType() == KUriFilterData::NetProtocol) {
        KAction *action = new KAction(KIcon("window-new"), i18nc("open selected url", "Open '%1'",
                                            KStringHandler::rsqueeze(data.uri().url(), 18)), this);
        m_actionCollection->addAction(QL1S("openSelection"), action);
        action->setData(QUrl(data.uri()));
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotOpenSelection()));
        selectActions.append(action);
    }

    selectGroupMap.insert("editactions", selectActions);
}

void WebView::linkActionPopupMenu(KParts::BrowserExtension::ActionGroupMap& linkGroupMap)
{
    Q_ASSERT(!m_result.linkUrl().isEmpty());

    const KUrl url(m_result.linkUrl());

    QList<QAction*> linkActions;

    KAction* action;

    if (m_result.isContentSelected()) {
        action = m_actionCollection->addAction(KStandardAction::Copy, QL1S("copy"),  m_part->browserExtension(), SLOT(copy()));
        action->setText(i18n("&Copy Text"));
        action->setEnabled(m_part->browserExtension()->isActionEnabled("copy"));
        linkActions.append(action);
    }

    if (url.protocol() == "mailto") {
        action = new KAction(i18n("&Copy Email Address"), this);
        m_actionCollection->addAction(QL1S("copylinklocation"), action);
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotCopyEmailAddress()));
        linkActions.append(action);
    } else {
        if (!m_result.isContentSelected()) {
            action = new KAction(KIcon("edit-copy"), i18n("Copy Link &Text"), this);
            m_actionCollection->addAction(QL1S("copylinktext"), action);
            connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotCopyLinkText()));
            linkActions.append(action);
        }

        action = new KAction(i18n("Copy Link &URL"), this);
        m_actionCollection->addAction(QL1S("copylinkurl"), action);
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotCopyLinkURL()));
        linkActions.append(action);

        action = new KAction(i18n("&Save Link As..."), this);
        m_actionCollection->addAction(QL1S("savelinkas"), action);
        connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(slotSaveLinkAs()));
        linkActions.append(action);
    }

    linkGroupMap.insert("linkactions", linkActions);
}

void WebView::multimediaActionPopupMenu(KParts::BrowserExtension::ActionGroupMap& mmGroupMap)
{
    QList<QAction*> multimediaActions;

    QWebElement element (m_result.element());
    const bool isPaused = element.evaluateJavaScript(QL1S("this.paused")).toBool();
    const bool isMuted = element.evaluateJavaScript(QL1S("this.muted")).toBool();
    const bool isLoopOn = element.evaluateJavaScript(QL1S("this.loop")).toBool();
    const bool areControlsOn = element.evaluateJavaScript(QL1S("this.controls")).toBool();
    const bool isVideoElement = (element.tagName().compare(QL1S("video"), Qt::CaseInsensitive) == 0);
    const bool isAudioElement = (element.tagName().compare(QL1S("audio"), Qt::CaseInsensitive) == 0);

    KAction* action = new KAction((isPaused ? i18n("&Play") : i18n("&Pause")), this);
    m_actionCollection->addAction(QL1S("playmultimedia"), action);
    connect(action, SIGNAL(triggered()), m_part->browserExtension(), SLOT(slotPlayMedia()));
    multimediaActions.append(action);

    action = new KAction((isMuted ? i18n("Un&mute") : i18n("&Mute")), this);
    m_actionCollection->addAction(QL1S("mutemultimedia"), action);
    connect(action, SIGNAL(triggered()), m_part->browserExtension(), SLOT(slotMuteMedia()));
    multimediaActions.append(action);

    action = new KAction(i18n("&Loop"), this);
    action->setCheckable(true);
    action->setChecked(isLoopOn);
    m_actionCollection->addAction(QL1S("loopmultimedia"), action);
    connect(action, SIGNAL(triggered()), m_part->browserExtension(), SLOT(slotLoopMedia()));
    multimediaActions.append(action);

    action = new KAction(i18n("Show &Controls"), this);
    action->setCheckable(true);
    action->setChecked(areControlsOn);
    m_actionCollection->addAction(QL1S("showmultimediacontrols"), action);
    connect(action, SIGNAL(triggered()), m_part->browserExtension(), SLOT(slotShowMediaControls()));
    multimediaActions.append(action);

    action = new KAction(m_actionCollection);
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

    action = new KAction(saveMediaText, this);
    m_actionCollection->addAction(QL1S("savemultimedia"), action);
    connect(action, SIGNAL(triggered()), m_part->browserExtension(), SLOT(slotSaveMedia()));
    multimediaActions.append(action);

    action = new KAction(copyMediaText, this);
    m_actionCollection->addAction(QL1S("copymultimediaurl"), action);
    connect(action, SIGNAL(triggered()), m_part->browserExtension(), SLOT(slotCopyMedia()));
    multimediaActions.append(action);

    mmGroupMap.insert("partactions", multimediaActions);
}

void WebView::slotStopAutoScroll()
{
    if (m_autoScrollTimerId == -1) {
        return;
    }

    killTimer(m_autoScrollTimerId);
    m_autoScrollTimerId = -1;
    m_verticalAutoScrollSpeed = 0;
    m_horizontalAutoScrollSpeed = 0;

}

void WebView::addSearchActions(QList<QAction*>& selectActions, QWebView* view)
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
        KAction *action = new KAction(KIcon(data.iconName()),
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

                KAction *action = new KAction(KIcon(data.iconNameForPreferredSearchProvider(searchProvider)), searchProvider, view);
                action->setData(data.queryForPreferredSearchProvider(searchProvider));
                m_actionCollection->addAction(searchProvider, action);
                connect(action, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(searchProvider()));

                providerList->addAction(action);
            }
            m_actionCollection->addAction(QL1S("searchProviderList"), providerList);
            selectActions.append(providerList);
        }
    }
}

bool WebView::checkForAccessKey(QKeyEvent *event)
{
    if (m_accessKeyLabels.isEmpty())
        return false;

    QString text = event->text();
    if (text.isEmpty())
        return false;
    QChar key = text.at(0).toUpper();
    bool handled = false;
    if (m_accessKeyNodes.contains(key)) {
        QWebElement element = m_accessKeyNodes[key];
        QPoint p = element.geometry().center();
        QWebFrame *frame = element.webFrame();
        Q_ASSERT(frame);
        do {
            p -= frame->scrollPosition();
            frame = frame->parentFrame();
        } while (frame && frame != page()->mainFrame());
        QMouseEvent pevent(QEvent::MouseButtonPress, p, Qt::LeftButton, 0, 0);
        QCoreApplication::sendEvent(this, &pevent);
        QMouseEvent revent(QEvent::MouseButtonRelease, p, Qt::LeftButton, 0, 0);
        QCoreApplication::sendEvent(this, &revent);
        handled = true;
    }
    return handled;
}

void WebView::hideAccessKeys()
{
    if (!m_accessKeyLabels.isEmpty()) {
        for (int i = 0, count = m_accessKeyLabels.count(); i < count; ++i) {
            QLabel *label = m_accessKeyLabels[i];
            label->hide();
            label->deleteLater();
        }
        m_accessKeyLabels.clear();
        m_accessKeyNodes.clear();
        m_duplicateLinkElements.clear();
        m_accessKeyActivated = NotActivated;
        emit statusBarMessage(QString());
        update();
    }
}

static QString linkElementKey(const QWebElement& element)
{
    if (element.hasAttribute(QL1S("href"))) {
        const QUrl url = element.webFrame()->baseUrl().resolved(element.attribute(QL1S("href")));
        QString linkKey (url.toString());
        if (element.hasAttribute(QL1S("target"))) {
            linkKey += QL1C('+');
            linkKey += element.attribute(QL1S("target"));
        }
        return linkKey; 
    }
    return QString();
}

static void handleDuplicateLinkElements(const QWebElement& element, QHash<QString, QChar>* dupLinkList, QChar* accessKey)
{
    if (element.tagName().compare(QL1S("A"), Qt::CaseInsensitive) == 0) {
        const QString linkKey (linkElementKey(element));
        // kDebug() << "LINK KEY:" << linkKey;
        if (dupLinkList->contains(linkKey)) {
            // kDebug() << "***** Found duplicate link element:" << linkKey << endl;
            *accessKey = dupLinkList->value(linkKey);
        } else if (!linkKey.isEmpty()) {
            dupLinkList->insert(linkKey, *accessKey);
        }
        if (linkKey.isEmpty())
            *accessKey = QChar();
    }
}

static bool isHiddenElement(const QWebElement& element)
{
    // width property set to less than zero
    if (element.hasAttribute(QL1S("width")) && element.attribute(QL1S("width")).toInt() < 1) {
        return true;
    }

    // height property set to less than zero
    if (element.hasAttribute(QL1S("height")) && element.attribute(QL1S("height")).toInt() < 1) {
        return true;
    }

    // visiblity set to 'hidden' in the element itself or its parent elements.
    if (element.styleProperty(QL1S("visibility"),QWebElement::ComputedStyle).compare(QL1S("hidden"), Qt::CaseInsensitive) == 0) {
        return true;
    }

    // display set to 'none' in the element itself or its parent elements.
    if (element.styleProperty(QL1S("display"),QWebElement::ComputedStyle).compare(QL1S("none"), Qt::CaseInsensitive) == 0) {
        return true;
    }

    return false;
}

void WebView::showAccessKeys()
{
    QList<QChar> unusedKeys;
    for (char c = 'A'; c <= 'Z'; ++c)
        unusedKeys << QLatin1Char(c);
    for (char c = '0'; c <= '9'; ++c)
        unusedKeys << QLatin1Char(c);

    QList<QWebElement> unLabeledElements;
    QRect viewport = QRect(page()->mainFrame()->scrollPosition(), page()->viewportSize());
    const QString selectorQuery (QLatin1String("a[href],"
                                               "area,"
                                               "button:not([disabled]),"
                                               "input:not([disabled]):not([hidden]),"
                                               "label[for],"
                                               "legend,"
                                               "select:not([disabled]),"
                                               "textarea:not([disabled])"));
    QList<QWebElement> result = page()->mainFrame()->findAllElements(selectorQuery).toList();

    // Priority first goes to elements with accesskey attributes
    Q_FOREACH (const QWebElement& element, result) {
        const QRect geometry = element.geometry();
        if (geometry.size().isEmpty() || !viewport.contains(geometry.topLeft())) {
            continue;
        }
        if (isHiddenElement(element)) {
            continue;    // Do not show access key for hidden elements...
        }
        const QString accessKeyAttribute (element.attribute(QLatin1String("accesskey")).toUpper());
        if (accessKeyAttribute.isEmpty()) {
            unLabeledElements.append(element);
            continue;
        }
        QChar accessKey;
        for (int i = 0; i < accessKeyAttribute.count(); i+=2) {
            const QChar &possibleAccessKey = accessKeyAttribute[i];
            if (unusedKeys.contains(possibleAccessKey)) {
                accessKey = possibleAccessKey;
                break;
            }
        }
        if (accessKey.isNull()) {
            unLabeledElements.append(element);
            continue;
        }

        handleDuplicateLinkElements(element, &m_duplicateLinkElements, &accessKey);
        if (!accessKey.isNull()) {
            unusedKeys.removeOne(accessKey);
            makeAccessKeyLabel(accessKey, element);
        }
    }


    // Pick an access key first from the letters in the text and then from the
    // list of unused access keys
    Q_FOREACH (const QWebElement &element, unLabeledElements) {
        const QRect geometry = element.geometry();
        if (unusedKeys.isEmpty()
            || geometry.size().isEmpty()
            || !viewport.contains(geometry.topLeft()))
            continue;
        QChar accessKey;
        QString text = element.toPlainText().toUpper();
        for (int i = 0; i < text.count(); ++i) {
            const QChar &c = text.at(i);
            if (unusedKeys.contains(c)) {
                accessKey = c;
                break;
            }
        }
        if (accessKey.isNull())
            accessKey = unusedKeys.takeFirst();

        handleDuplicateLinkElements(element, &m_duplicateLinkElements, &accessKey);
        if (!accessKey.isNull()) {
            unusedKeys.removeOne(accessKey);
            makeAccessKeyLabel(accessKey, element);
        }
    }

    m_accessKeyActivated = (m_accessKeyLabels.isEmpty() ? Activated : NotActivated);
}

void WebView::makeAccessKeyLabel(const QChar &accessKey, const QWebElement &element)
{
    QLabel *label = new QLabel(this);
    QFont font (label->font());
    font.setBold(true);
    label->setFont(font);
    label->setText(accessKey);
    label->setPalette(QToolTip::palette());
    label->setAutoFillBackground(true);
    label->setFrameStyle(QFrame::Box | QFrame::Plain);
    QPoint point = element.geometry().center();
    point -= page()->mainFrame()->scrollPosition();
    label->move(point);
    label->show();
    point.setX(point.x() - label->width() / 2);
    label->move(point);
    m_accessKeyLabels.append(label);
    m_accessKeyNodes.insertMulti(accessKey, element);
}
