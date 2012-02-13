/*
 * This file is part of the KDE project.
 *
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

#include "kwebkitpart_ext.h"

#include "kwebkitpart.h"
#include "webview.h"
#include "webpage.h"
#include "settings/webkitsettings.h"

#include <KDE/KAction>
#include <KDE/KUriFilterData>
#include <KDE/KDesktopFile>
#include <KDE/KConfigGroup>
#include <KDE/KToolInvocation>
#include <KDE/KGlobal>
#include <KDE/KSharedConfig>
#include <KDE/KRun>
#include <KDE/KDebug>
#include <KDE/KPrintPreview>
#include <KDE/KSaveFile>
#include <KDE/KComponentData>
#include <KDE/KProtocolInfo>
#include <KDE/KInputDialog>
#include <KDE/KLocalizedString>
#include <KDE/KTemporaryFile>
#include <Sonnet/Dialog>
#include <sonnet/backgroundchecker.h>
#include <kdeversion.h>

#include <QtCore/QBuffer>
#include <QtCore/QCryptographicHash>
#include <QtGui/QClipboard>
#include <QtGui/QApplication>
#include <QtGui/QPrinter>
#include <QtGui/QPrintDialog>
#include <QtGui/QPrintPreviewDialog>
#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebHistory>
#include <QtWebKit/QWebElement>
#include <QtWebKit/QWebElementCollection>

#define QL1S(x)     QLatin1String(x)
#define QL1C(x)     QLatin1Char(x)


WebKitBrowserExtension::WebKitBrowserExtension(KWebKitPart *parent, const QByteArray& historyData)
                       :KParts::BrowserExtension(parent),
                        m_part(QWeakPointer<KWebKitPart>(parent))
{
    enableAction("cut", false);
    enableAction("copy", false);
    enableAction("paste", false);
    enableAction("print", true);
    connect(this, SIGNAL(openUrlNotify()), this, SLOT(slotSaveHistory()));

    if (!historyData.isEmpty()) {
        QBuffer buffer;
        buffer.setData(historyData);
        if (buffer.open(QIODevice::ReadOnly)) {
            // NOTE: When restoring history, webkit automatically navigates to
            // the previous "currentItem". Since we do not want that to happen,
            // we set a property on the WebPage object that is used to allow or
            // disallow history navigation in WebPage::acceptNavigationRequest.
            view()->page()->setProperty("HistoryNavigationLocked", true);
            QDataStream s (&buffer);
            s >> *(view()->history());
        }
    }
}

WebKitBrowserExtension::~WebKitBrowserExtension()
{
}

WebView* WebKitBrowserExtension::view()
{
    if (!m_view) {
        if (!m_part)
            return 0;
        m_view = QWeakPointer<WebView>(qobject_cast<WebView*>(m_part.data()->view()));
    }

    return m_view.data();
}

int WebKitBrowserExtension::xOffset()
{
    if (view())
        return view()->page()->mainFrame()->scrollPosition().x();

    return KParts::BrowserExtension::xOffset();
}

int WebKitBrowserExtension::yOffset()
{
    if (view())
        return view()->page()->mainFrame()->scrollPosition().y();

    return KParts::BrowserExtension::yOffset();
}

void WebKitBrowserExtension::saveState(QDataStream &stream)
{
    QByteArray historyData;
    QWebHistory* history = (view() ? view()->page()->history() : 0);

    // Since history data can grow large, we compress it in order to limit
    // memeory usage before providing it to the hosting app for storage.
    if (history) {
        QBuffer buffer (&historyData);
        if (!buffer.open(QIODevice::WriteOnly))
            return;

        QDataStream stream (&buffer);
        stream << *history;
        historyData = qCompress(historyData, 9);
    }

    stream << m_part.data()->url()
           << static_cast<qint32>(xOffset())
           << static_cast<qint32>(yOffset())
           << static_cast<qint32>(history ? history->currentItemIndex() : -1)
           << historyData;
}

void WebKitBrowserExtension::restoreState(QDataStream &stream)
{
    KUrl u;
    QByteArray historyData;
    qint32 xOfs = -1, yOfs = -1, historyItemIndex = -1;
    stream >> u >> xOfs >> yOfs >> historyItemIndex >> historyData;

    QWebHistory* history = (view() ? view()->page()->history() : 0);
    if (history) {
        bool success = false;
        if (history->count() == 0) {
            historyData = qUncompress(historyData); // uncompress the history data...
            QBuffer buffer (&historyData);
            if (buffer.open(QIODevice::ReadOnly)) {
                QDataStream stream (&buffer);
                view()->page()->setProperty("HistoryNavigationLocked", true);
                stream >> *history;
                QWebHistoryItem currentItem (history->currentItem());
                if (currentItem.isValid()) {
                    if (currentItem.userData().isNull() && (xOfs != -1 || yOfs != -1)) {
                        const QPoint scrollPos (xOfs, yOfs);
                        currentItem.setUserData(scrollPos);
                    }
                    // NOTE 1: The following Konqueror specific workaround is necessary
                    // because Konqueror only preserves information for the last visited
                    // page. However, we save the entire history content in saveState and
                    // and hence need to elimiate all but the current item here.
                    // NOTE 2: This condition only applies when Konqueror is restored from
                    // abnormal termination ; a crash and/or a session restoration.
                    if (QCoreApplication::applicationName() == QLatin1String("konqueror")) {
                        history->clear();
                    }
                    m_part.data()->setProperty("NoEmitOpenUrlNotification", true);
                    history->goToItem(currentItem);
                }
            }
            success = (history->count() > 0);
        } else {
            if (history->count() > historyItemIndex && historyItemIndex > -1) {
                QWebHistoryItem item (history->itemAt(historyItemIndex));
                if (u == item.url()) {
                    m_part.data()->setProperty("NoEmitOpenUrlNotification", true);
                    history->goToItem(item);
                    success = true;
                    //kDebug() << "history count:" << history->count() << "request index:" << historyItemIndex;
                }
            }
        }
        if (success) {
            return;
        }
    }

    // As a last resort, in case the history restoration logic above fails,
    // attempt to open the requested URL directly.
    kWarning() << "Normal history navgation logic failed! Attempting to use a workaround!";
    m_part.data()->openUrl(u);
}

void WebKitBrowserExtension::cut()
{
    if (view())
        view()->triggerPageAction(QWebPage::Cut);
}

void WebKitBrowserExtension::copy()
{
    if (view())
        view()->triggerPageAction(QWebPage::Copy);
}

void WebKitBrowserExtension::paste()
{
    if (view())
        view()->triggerPageAction(QWebPage::Paste);
}

void WebKitBrowserExtension::slotSaveDocument()
{
    if (view())
        emit saveUrl(view()->url());
}

void WebKitBrowserExtension::slotSaveFrame()
{
    if (view())
        emit saveUrl(view()->page()->currentFrame()->url());
}

void WebKitBrowserExtension::print()
{
    if (view())
        slotPrintRequested(view()->page()->currentFrame());
}

void WebKitBrowserExtension::updateEditActions()
{
    if (!view())
        return;

    enableAction("cut", view()->pageAction(QWebPage::Cut)->isEnabled());
    enableAction("copy", view()->pageAction(QWebPage::Copy)->isEnabled());
    enableAction("paste", view()->pageAction(QWebPage::Paste)->isEnabled());
}

void WebKitBrowserExtension::updateActions()
{
    const QString protocol (m_part.data()->url().protocol());
    const bool isValidDocument = (protocol != QL1S("about") && protocol != QL1S("error"));
    enableAction("print", isValidDocument);
}

void WebKitBrowserExtension::searchProvider()
{
    if (!view())
        return;

    KAction *action = qobject_cast<KAction*>(sender());
    if (!action)
        return;

    KUrl url = action->data().toUrl();

    if (url.host().isEmpty()) {
        KUriFilterData data;
        data.setData(action->data().toString());
        if (KUriFilter::self()->filterSearchUri(data, KUriFilter::WebShortcutFilter))
            url = data.uri();
    }

    if (!url.isValid())
      return;

    KParts::BrowserArguments bargs;
    bargs.frameName = QL1S("_blank");
    emit openUrlRequest(url, KParts::OpenUrlArguments(), bargs);
}

void WebKitBrowserExtension::reparseConfiguration()
{
    // Force the configuration stuff to reparse...
    WebKitSettings::self()->init();
}

void WebKitBrowserExtension::disableScrolling()
{
    QWebView* currentView = view();
    QWebPage* page = currentView ? currentView->page() : 0;
    QWebFrame* frame = page ? page->mainFrame() : 0;

    if (!frame)
        return;

    frame->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
    frame->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
}

void WebKitBrowserExtension::zoomIn()
{
    if (view())
        view()->setZoomFactor(view()->zoomFactor() + 0.1);
}

void WebKitBrowserExtension::zoomOut()
{
    if (view())
        view()->setZoomFactor(view()->zoomFactor() - 0.1);
}

void WebKitBrowserExtension::zoomNormal()
{
    if (view())
        view()->setZoomFactor(1);
}

void WebKitBrowserExtension::toogleZoomTextOnly()
{
    if (!view())
        return;

    KConfigGroup cgHtml(KGlobal::config(), "HTML Settings");
    bool zoomTextOnly = cgHtml.readEntry( "ZoomTextOnly", false );
    cgHtml.writeEntry("ZoomTextOnly", !zoomTextOnly);
    KGlobal::config()->reparseConfiguration();

    view()->settings()->setAttribute(QWebSettings::ZoomTextOnly, !zoomTextOnly);
}

void WebKitBrowserExtension::slotSelectAll()
{
    if (view())
        view()->triggerPageAction(QWebPage::SelectAll);
}

void WebKitBrowserExtension::slotFrameInWindow()
{
    if (!view())
        return;

    KParts::BrowserArguments bargs;
    bargs.setForcesNewWindow(true);

    KParts::OpenUrlArguments uargs;
    uargs.setActionRequestedByUser(true);

    QUrl url (view()->page()->currentFrame()->baseUrl());
    url.resolved(view()->page()->currentFrame()->url());

    emit createNewWindow(KUrl(url), uargs, bargs);
}

void WebKitBrowserExtension::slotFrameInTab()
{
    if (!view())
        return;

    KParts::OpenUrlArguments uargs;
    uargs.setActionRequestedByUser(true);

    KParts::BrowserArguments bargs;
    bargs.setNewTab(true);

    QUrl url (view()->page()->currentFrame()->baseUrl());
    url.resolved(view()->page()->currentFrame()->url());

    emit createNewWindow(KUrl(url), uargs, bargs);
}

void WebKitBrowserExtension::slotFrameInTop()
{
    if (!view())
        return;

    KParts::BrowserArguments bargs;//( m_m_khtml->browserExtension()->browserArguments() );
    bargs.frameName = QL1S("_top");
    emit openUrlRequest(view()->page()->currentFrame()->url(), KParts::OpenUrlArguments(), bargs);
}

void WebKitBrowserExtension::slotReloadFrame()
{
    if (view())
        view()->page()->currentFrame()->load(view()->page()->currentFrame()->url());
}

void WebKitBrowserExtension::slotBlockIFrame()
{
    if (!view())
        return;

    bool ok = false;

    const QWebFrame* frame = view()->contextMenuResult().frame();
    const QString urlStr = frame ? frame->url().toString() : QString();

    const QString url = KInputDialog::getText(i18n("Add URL to Filter"),
                                              i18n("Enter the URL:"),
                                              urlStr, &ok);
    if (ok) {
        WebKitSettings::self()->addAdFilter(url);
        reparseConfiguration();
    }
}

void WebKitBrowserExtension::slotSaveImageAs()
{
    if (view())
        view()->triggerPageAction(QWebPage::DownloadImageToDisk);
}

void WebKitBrowserExtension::slotSendImage()
{
    if (!view())
        return;

    QStringList urls;
    urls.append(view()->contextMenuResult().imageUrl().path());
    const QString subject = view()->contextMenuResult().imageUrl().path();
    KToolInvocation::invokeMailer(QString(), QString(), QString(), subject,
                                  QString(), //body
                                  QString(),
                                  urls); // attachments
}

void WebKitBrowserExtension::slotCopyImageURL()
{
    if (!view())
        return;

    KUrl safeURL(view()->contextMenuResult().imageUrl());
    safeURL.setPass(QString());
    // Set it in both the mouse selection and in the clipboard
    QMimeData* mimeData = new QMimeData;
    safeURL.populateMimeData(mimeData);
    QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);

    mimeData = new QMimeData;
    safeURL.populateMimeData(mimeData);
    QApplication::clipboard()->setMimeData(mimeData, QClipboard::Selection);
}


void WebKitBrowserExtension::slotCopyImage()
{
    if (!view())
        return;

    KUrl safeURL(view()->contextMenuResult().imageUrl());
    safeURL.setPass(QString());

    // Set it in both the mouse selection and in the clipboard
    QMimeData* mimeData = new QMimeData;
    mimeData->setImageData(view()->contextMenuResult().pixmap());
    safeURL.populateMimeData(mimeData);
    QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);

    mimeData = new QMimeData;
    mimeData->setImageData(view()->contextMenuResult().pixmap());
    safeURL.populateMimeData(mimeData);
    QApplication::clipboard()->setMimeData(mimeData, QClipboard::Selection);
}

void WebKitBrowserExtension::slotViewImage()
{
    if (view())
        emit createNewWindow(view()->contextMenuResult().imageUrl());
}

void WebKitBrowserExtension::slotBlockImage()
{
    if (!view())
        return;

    bool ok = false;
    const QString url = KInputDialog::getText(i18n("Add URL to Filter"),
                                              i18n("Enter the URL:"),
                                              view()->contextMenuResult().imageUrl().toString(),
                                              &ok);
    if (ok) {
        WebKitSettings::self()->addAdFilter(url);
        reparseConfiguration();
    }
}

void WebKitBrowserExtension::slotBlockHost()
{
    if (!view())
        return;

    QUrl url (view()->contextMenuResult().imageUrl());
    url.setPath(QL1S("/*"));
    WebKitSettings::self()->addAdFilter(url.toString(QUrl::RemoveUserInfo | QUrl::RemovePort));
    reparseConfiguration();
}

void WebKitBrowserExtension::slotCopyLinkURL()
{
    if (view())
        view()->triggerPageAction(QWebPage::CopyLinkToClipboard);
}

void WebKitBrowserExtension::slotSaveLinkAs()
{
    if (view())
        view()->triggerPageAction(QWebPage::DownloadLinkToDisk);
}

void WebKitBrowserExtension::slotViewDocumentSource()
{
    if (!view())
        return;
#if 0
    //FIXME: This workaround is necessary because freakin' QtWebKit does not provide
    //a means to obtain the original content of the frame. Actually it does, but the
    //returned content is royally screwed up! *sigh*
    KRun::runUrl(view()->page()->mainFrame()->url(), QL1S("text/plain"), view(), false);
#else
    KTemporaryFile tempFile;
    tempFile.setSuffix(QL1S(".html"));
    tempFile.setAutoRemove(false);
    if (tempFile.open()) {
        tempFile.write(view()->page()->mainFrame()->toHtml().toUtf8());
        KRun::runUrl(tempFile.fileName(), QL1S("text/plain"), view(), true, false);
    }
#endif
}

void WebKitBrowserExtension::slotViewFrameSource()
{
    if (!view())
        return;
#if 0
    //FIXME: This workaround is necessary because freakin' QtWebKit does not provide
    //a means to obtain the original content of the frame. Actually it does, but the
    //returned content is royally screwed up! *sigh*
    KRun::runUrl(view()->page()->mainFrame()->url(), QL1S("text/plain"), view(), false);
#else
    KTemporaryFile tempFile;
    tempFile.setSuffix(QL1S(".html"));
    tempFile.setAutoRemove(false);
    if (tempFile.open()) {
        tempFile.write(view()->page()->currentFrame()->toHtml().toUtf8());
        KRun::runUrl(tempFile.fileName(), QL1S("text/plain"), view(), true, false);
    }
#endif
}

static bool isMultimediaElement(const QWebElement& element)
{
    if (element.tagName().compare(QL1S("video"), Qt::CaseInsensitive) == 0)
        return true;

    if (element.tagName().compare(QL1S("audio"), Qt::CaseInsensitive) == 0)
        return true;

    return false;
}

void WebKitBrowserExtension::slotLoopMedia()
{
    if (!view())
        return;

    QWebElement element (view()->contextMenuResult().element());
    if (!isMultimediaElement(element))
        return;

    element.evaluateJavaScript(QL1S("this.loop = !this.loop;"));
}

void WebKitBrowserExtension::slotMuteMedia()
{
    if (!view())
        return;

    QWebElement element (view()->contextMenuResult().element());
    if (!isMultimediaElement(element))
        return;

    element.evaluateJavaScript(QL1S("this.muted = !this.muted;"));
}

void WebKitBrowserExtension::slotPlayMedia()
{
    if (!view())
        return;

    QWebElement element (view()->contextMenuResult().element());
    if (!isMultimediaElement(element))
        return;

    element.evaluateJavaScript(QL1S("this.paused ? this.play() : this.pause();"));
}

void WebKitBrowserExtension::slotShowMediaControls()
{
    if (!view())
        return;

    QWebElement element (view()->contextMenuResult().element());
    if (!isMultimediaElement(element))
        return;

    element.evaluateJavaScript(QL1S("this.controls = !this.controls;"));
}

static KUrl mediaUrlFrom(QWebElement& element)
{
    QWebFrame* frame = element.webFrame();
    QString src = frame ? element.attribute(QL1S("src")) : QString();
    if (src.isEmpty())
        src = frame ? element.evaluateJavaScript(QL1S("this.src")).toString() : QString();

    if (src.isEmpty())
        return KUrl();

    return KUrl(frame->baseUrl().resolved(QUrl::fromEncoded(QUrl::toPercentEncoding(src), QUrl::StrictMode)));
}

void WebKitBrowserExtension::slotSaveMedia()
{
    if (!view())
        return;

    QWebElement element (view()->contextMenuResult().element());
    if (!isMultimediaElement(element))
        return;

    emit saveUrl(mediaUrlFrom(element));
}

void WebKitBrowserExtension::slotCopyMedia()
{
    if (!view())
        return;

    QWebElement element (view()->contextMenuResult().element());
    if (!isMultimediaElement(element))
        return;

    KUrl safeURL(mediaUrlFrom(element));
    if (!safeURL.isValid())
        return;

    safeURL.setPass(QString());
    // Set it in both the mouse selection and in the clipboard
    QMimeData* mimeData = new QMimeData;
    safeURL.populateMimeData(mimeData);
    QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);

    mimeData = new QMimeData;
    safeURL.populateMimeData(mimeData);
    QApplication::clipboard()->setMimeData(mimeData, QClipboard::Selection);
}

void WebKitBrowserExtension::slotTextDirectionChanged()
{
    KAction* action = qobject_cast<KAction*>(sender());
    if (action) {
        bool ok = false;
        const int value = action->data().toInt(&ok);
        if (ok) {
            view()->triggerPageAction(static_cast<QWebPage::WebAction>(value));
        }
    }
}

static QVariant execJScript(WebView* view, const QString& script)
{
    QWebElement element (view->contextMenuResult().element());
    if (element.isNull())
        return QVariant();
    return element.evaluateJavaScript(script);
}

void WebKitBrowserExtension::slotCheckSpelling()
{
    const QString text (execJScript(view(), QL1S("this.value")).toString());

    if ( text.isEmpty() ) {
        return;
    }

    m_spellTextSelectionStart = 0;
    m_spellTextSelectionEnd = 0;

    Sonnet::Dialog* spellDialog = new Sonnet::Dialog(new Sonnet::BackgroundChecker(this), view());
    connect(spellDialog, SIGNAL(replace(QString,int,QString)), this, SLOT(spellCheckerCorrected(QString,int,QString)));
    connect(spellDialog, SIGNAL(misspelling(QString,int)), this, SLOT(spellCheckerMisspelling(QString,int)));
    spellDialog->setBuffer(text);
    spellDialog->show();
}

void WebKitBrowserExtension::slotSpellCheckSelection()
{
    QString text (execJScript(view(), QL1S("this.value")).toString());

    if ( text.isEmpty() ) {
        return;
    }

    m_spellTextSelectionStart = qMax(0, execJScript(view(), QL1S("this.selectionStart")).toInt());
    m_spellTextSelectionEnd = qMax(0, execJScript(view(), QL1S("this.selectionEnd")).toInt());
    // kDebug() << "selection start:" << m_spellTextSelectionStart << "end:" << m_spellTextSelectionEnd;

    Sonnet::Dialog* spellDialog = new Sonnet::Dialog(new Sonnet::BackgroundChecker(this), view());
    connect(spellDialog, SIGNAL(replace(QString,int,QString)), this, SLOT(spellCheckerCorrected(QString,int,QString)));
    connect(spellDialog, SIGNAL(misspelling(QString,int)), this, SLOT(spellCheckerMisspelling(QString,int)));
    spellDialog->setBuffer(text.mid(m_spellTextSelectionStart, m_spellTextSelectionEnd));
    spellDialog->show();
}

void WebKitBrowserExtension::spellCheckerCorrected(const QString& original, int pos, const QString& replacement)
{
    // Adjust the selection end...
    if (m_spellTextSelectionEnd > 0) {
        m_spellTextSelectionEnd += qMax (0, (replacement.length() - original.length()));
    }

    const int index = pos + m_spellTextSelectionStart;
    QString script(QL1S("this.value=this.value.substring(0,"));
    script += QString::number(index);
    script += QL1S(") + \"");
    script +=  replacement;
    script += QL1S("\" + this.value.substring(");
    script += QString::number(index + original.length());
    script += QL1S(")");

    if (m_spellTextSelectionStart > 0 || m_spellTextSelectionEnd > 0) {

        script += QL1S("; this.setSelectionRange(");
        script += QString::number(m_spellTextSelectionStart);
        script += QL1C(',');
        script += QString::number(m_spellTextSelectionEnd);
        script += QL1C(')');
    }
    //kDebug() << "**** script:" << script;
    execJScript(view(), script);
}

void WebKitBrowserExtension::spellCheckerMisspelling(const QString& text, int pos)
{
    // kDebug() << text << pos;
    QString selectionScript (QL1S("this.setSelectionRange("));
    selectionScript += QString::number(pos + m_spellTextSelectionStart);
    selectionScript += QL1C(',');
    selectionScript += QString::number(pos + text.length() + m_spellTextSelectionStart);
    selectionScript += QL1C(')');
    execJScript(view(), selectionScript);
}

void WebKitBrowserExtension::slotSaveHistory()
{
    QByteArray histData;
    QBuffer buff (&histData);
    if (!buff.open(QIODevice::WriteOnly)) {
        kWarning() << "Failed to save history data!";
        return;
    }

    QWebHistory* history = view() ? view()->history() : 0;
    if (history && history->count() > 0) {
        QDataStream stream (&buff);
        stream << *history;
        //kDebug() << "# of items:" << history->count() << "current item:" << history->currentItemIndex() << "url:" << history->currentItem().url();
        QWidget* mainWidget = m_part.data() ? m_part.data()->widget() : 0;
        QWidget* frameWidget = mainWidget ? mainWidget->parentWidget() : 0;
        if (frameWidget) {
            emit saveHistory(frameWidget, histData);
        }
    }
}

void WebKitBrowserExtension::slotPrintRequested(QWebFrame* frame)
{
    if (!frame)
        return;

    // Make it non-modal, in case a redirection deletes the part
    QScopedPointer<QPrintDialog> dlg (new QPrintDialog(view()));
    if (dlg->exec() == QPrintDialog::Accepted) {
        frame->print(dlg->printer());
    }
}

void WebKitBrowserExtension::slotPrintPreview()
{
    // Make it non-modal, in case a redirection deletes the part
    QScopedPointer<QPrintPreviewDialog> dlg (new QPrintPreviewDialog(view()));
    connect(dlg.data(), SIGNAL(paintRequested(QPrinter*)),
            view()->page()->currentFrame(), SLOT(print(QPrinter*)));
    dlg->exec();
}


////

KWebKitTextExtension::KWebKitTextExtension(KWebKitPart* part)
    : KParts::TextExtension(part)
{
    connect(part->view(), SIGNAL(selectionChanged()), this, SIGNAL(selectionChanged()));
}

KWebKitPart* KWebKitTextExtension::part() const
{
    return static_cast<KWebKitPart*>(parent());
}

bool KWebKitTextExtension::hasSelection() const
{
#if QTWEBKIT_VERSION >= QTWEBKIT_VERSION_CHECK(2, 2, 0)
    return part()->view()->hasSelection();
#else
    return !part()->view()->selectedText().isEmpty();
#endif
}

QString KWebKitTextExtension::selectedText(Format format) const
{
    switch(format) {
    case PlainText:
        return part()->view()->selectedText();
    case HTML:
#if QTWEBKIT_VERSION >= QTWEBKIT_VERSION_CHECK(2, 2, 0)
        return part()->view()->selectedHtml();
#else
        return part()->view()->page()->currentFrame()->toHtml();
#endif
    }
    return QString();
}

QString KWebKitTextExtension::completeText(Format format) const
{
    switch(format) {
    case PlainText:
        return part()->view()->page()->currentFrame()->toPlainText();
    case HTML:
        return part()->view()->page()->currentFrame()->toHtml();
    }
    return QString();
}

////

KWebKitHtmlExtension::KWebKitHtmlExtension(KWebKitPart* part)
    : KParts::HtmlExtension(part)
{
}


KUrl KWebKitHtmlExtension::baseUrl() const
{
    return part()->view()->page()->mainFrame()->baseUrl();
}

bool KWebKitHtmlExtension::hasSelection() const
{
#if QTWEBKIT_VERSION >= QTWEBKIT_VERSION_CHECK(2, 2, 0)
    return part()->view()->hasSelection();
#else
    return !part()->view()->selectedText().isEmpty();
#endif
}

KParts::SelectorInterface::QueryMethods KWebKitHtmlExtension::supportedQueryMethods() const
{
    // TODO: Add support for selected content...
    return KParts::SelectorInterface::EntireContent;
}

static KParts::SelectorInterface::Element convertWebElement(const QWebElement& webElem)
{
    KParts::SelectorInterface::Element element;
    element.setTagName(webElem.tagName());
    Q_FOREACH(const QString &attr, webElem.attributeNames()) {
        element.setAttribute(attr, webElem.attribute(attr));
    }
    return element;
}

KParts::SelectorInterface::Element KWebKitHtmlExtension::querySelector(const QString& query, KParts::SelectorInterface::QueryMethod method) const
{
    KParts::SelectorInterface::Element element;

    // If the specified method is None, return an empty list...
    if (method == KParts::SelectorInterface::None)
        return element;

    // If the specified method is not supported, return an empty list...
    if (!(supportedQueryMethods() & method))
        return element;

    switch (method) {
    case KParts::SelectorInterface::EntireContent: {
        const QWebFrame* webFrame = part()->view()->page()->mainFrame();
        element = convertWebElement(webFrame->findFirstElement(query));
        break;
    }
    case KParts::SelectorInterface::SelectedContent:
        // TODO: Implement support for querying only selected content...
    default:
        break;
    }

    return element;
}

QList<KParts::SelectorInterface::Element> KWebKitHtmlExtension::querySelectorAll(const QString& query, KParts::SelectorInterface::QueryMethod method) const
{
    QList<KParts::SelectorInterface::Element> elements;

    // If the specified method is None, return an empty list...
    if (method == KParts::SelectorInterface::None)
        return elements;

    // If the specified method is not supported, return an empty list...
    if (!(supportedQueryMethods() & method))
        return elements;

    switch (method) {
    case KParts::SelectorInterface::EntireContent: {
        const QWebFrame* webFrame = part()->view()->page()->mainFrame();
        const QWebElementCollection collection = webFrame->findAllElements(query);
        elements.reserve(collection.count());
        Q_FOREACH(const QWebElement& element, collection)
            elements.append(convertWebElement(element));
        break;
    }
    case KParts::SelectorInterface::SelectedContent:
        // TODO: Implement support for querying only selected content...
    default:
        break;
    }
    return elements;
}

KWebKitPart* KWebKitHtmlExtension::part() const
{
    return static_cast<KWebKitPart*>(parent());
}

#include "kwebkitpart_ext.moc"
