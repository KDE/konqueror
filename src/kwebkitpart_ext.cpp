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

#include <QBuffer>
#include <QVariant>
#include <QClipboard>
#include <QApplication>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QWebFrame>
#include <QWebHistory>
#include <QWebElement>
#include <QWebElementCollection>

#define QL1S(x)     QLatin1String(x)
#define QL1C(x)     QLatin1Char(x)


WebKitBrowserExtension::WebKitBrowserExtension(KWebKitPart *parent, const QByteArray& cachedHistoryData)
                       :KParts::BrowserExtension(parent),
                        m_part(parent)
{
    enableAction("cut", false);
    enableAction("copy", false);
    enableAction("paste", false);
    enableAction("print", true);

    if (cachedHistoryData.isEmpty()) {
        return;
    }

    QBuffer buffer;
    buffer.setData(cachedHistoryData);
    if (!buffer.open(QIODevice::ReadOnly)) {
        return;
    }

    // NOTE: When restoring history, webkit automatically navigates to
    // the previous "currentItem". Since we do not want that to happen,
    // we set a property on the WebPage object that is used to allow or
    // disallow history navigation in WebPage::acceptNavigationRequest.
    view()->page()->setProperty("HistoryNavigationLocked", true);
    QDataStream s (&buffer);
    s >> *(view()->history());
}

WebKitBrowserExtension::~WebKitBrowserExtension()
{
}

WebView* WebKitBrowserExtension::view()
{
    if (!m_view && m_part) {
        m_view = qobject_cast<WebView*>(m_part->view());
    }

    return m_view;
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
    // TODO: Save information such as form data from the current page.
    QWebHistory* history = (view() ? view()->history() : 0);
    const int historyIndex = (history ? history->currentItemIndex() : -1);
    const KUrl historyUrl = (history ? KUrl(history->currentItem().url()) : m_part->url());

    stream << historyUrl
           << static_cast<qint32>(xOffset())
           << static_cast<qint32>(yOffset())
           << historyIndex
           << m_historyData;
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
        if (history->count() == 0) {   // Handle restoration: crash recovery, tab close undo, session restore
            if (!historyData.isEmpty()) {
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
                        //kDebug() << "Restoring URL:" << currentItem.url();
                        m_part->setProperty("NoEmitOpenUrlNotification", true);
                        history->goToItem(currentItem);
                    }
                }
            }
            success = (history->count() > 0);
        } else {        // Handle navigation: back and forward button navigation.
            //kDebug() << "history count:" << history->count() << "request index:" << historyItemIndex;
            if (history->count() > historyItemIndex && historyItemIndex > -1) {
                QWebHistoryItem item (history->itemAt(historyItemIndex));
                //kDebug() << "URL:" << u << "Item URL:" << item.url();
                if (u == item.url()) {
                    if (item.userData().isNull() && (xOfs != -1 || yOfs != -1)) {
                        const QPoint scrollPos (xOfs, yOfs);
                        item.setUserData(scrollPos);
                    }
                    m_part->setProperty("NoEmitOpenUrlNotification", true);
                    history->goToItem(item);
                    success = true;
                }
            }
        }

        if (success) {
            return;
        }
    }

    // As a last resort, in case the history restoration logic above fails,
    // attempt to open the requested URL directly.
    kDebug() << "Normal history navgation logic failed! Falling back to opening url directly.";
    m_part->openUrl(u);
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
    const QString protocol (m_part->url().protocol());
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

    KParts::OpenUrlArguments uargs;
    uargs.setActionRequestedByUser(true);

    KParts::BrowserArguments bargs;
    bargs.frameName = QL1S("_top");

    QUrl url (view()->page()->currentFrame()->baseUrl());
    url.resolved(view()->page()->currentFrame()->url());

    emit openUrlRequest(KUrl(url), uargs, bargs);
}

void WebKitBrowserExtension::slotReloadFrame()
{
    if (view())
        view()->page()->currentFrame()->load(view()->page()->currentFrame()->url());
}

static QString iframeUrl(QWebFrame* frame)
{
   return ((frame && frame->baseUrl().isValid()) ? frame->baseUrl() : frame->url()).toString();
}

void WebKitBrowserExtension::slotBlockIFrame()
{
    if (!view())
        return;

    bool ok = false;
    const QString urlStr = iframeUrl(view()->contextMenuResult().frame());
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

void WebKitBrowserExtension::slotCopyLinkText()
{
    if (view()) {
        QMimeData* data = new QMimeData;
        data->setText(view()->contextMenuResult().linkText());
        QApplication::clipboard()->setMimeData(data, QClipboard::Clipboard);
    }
}

void WebKitBrowserExtension::slotCopyEmailAddress()
{
    if (view()) {
        QMimeData* data = new QMimeData;
        const QUrl url (view()->contextMenuResult().linkUrl());
        data->setText(url.path());
        QApplication::clipboard()->setMimeData(data, QClipboard::Clipboard);
    }
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

    const KUrl pageUrl (view()->url());
    if (pageUrl.isLocalFile()) {
        KRun::runUrl(pageUrl, QL1S("text/plain"), view(), false);
    } else {
        KTemporaryFile tempFile;
        tempFile.setSuffix(QL1S(".html"));
        tempFile.setAutoRemove(false);
        if (tempFile.open()) {
            tempFile.write(view()->page()->mainFrame()->toHtml().toUtf8());
            KRun::runUrl(tempFile.fileName(), QL1S("text/plain"), view(), true, false);
        }
    }
}

void WebKitBrowserExtension::slotViewFrameSource()
{
    if (!view())
        return;

    const KUrl frameUrl(view()->page()->currentFrame()->url());
    if (frameUrl.isLocalFile()) {
        KRun::runUrl(frameUrl, QL1S("text/plain"), view(), false);
    } else {
        KTemporaryFile tempFile;
        tempFile.setSuffix(QL1S(".html"));
        tempFile.setAutoRemove(false);
        if (tempFile.open()) {
            tempFile.write(view()->page()->currentFrame()->toHtml().toUtf8());
            KRun::runUrl(tempFile.fileName(), QL1S("text/plain"), view(), true, false);
        }
    }
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
    spellDialog->showSpellCheckCompletionMessage(true);
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
    spellDialog->showSpellCheckCompletionMessage(true);
    connect(spellDialog, SIGNAL(replace(QString,int,QString)), this, SLOT(spellCheckerCorrected(QString,int,QString)));
    connect(spellDialog, SIGNAL(misspelling(QString,int)), this, SLOT(spellCheckerMisspelling(QString,int)));
    connect(spellDialog, SIGNAL(done(QString)), this, SLOT(slotSpellCheckDone(QString)));
    spellDialog->setBuffer(text.mid(m_spellTextSelectionStart, (m_spellTextSelectionEnd - m_spellTextSelectionStart)));
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

void WebKitBrowserExtension::slotSpellCheckDone(const QString&)
{
    // Restore the text selection if one was present before we started the
    // spell check.
    if (m_spellTextSelectionStart > 0 || m_spellTextSelectionEnd > 0) {
        QString script (QL1S("; this.setSelectionRange("));
        script += QString::number(m_spellTextSelectionStart);
        script += QL1C(',');
        script += QString::number(m_spellTextSelectionEnd);
        script += QL1C(')');
        execJScript(view(), script);
    }
}


void WebKitBrowserExtension::saveHistory()
{
    QWebHistory* history = (view() ? view()->history() : 0);

    if (history && history->count() > 0) {
        //kDebug() << "Current history: index=" << history->currentItemIndex() << "url=" << history->currentItem().url();
        QByteArray histData;
        QBuffer buff (&histData);
        m_historyData.clear();
        if (buff.open(QIODevice::WriteOnly)) {
            QDataStream stream (&buff);
            stream << *history;
            m_historyData = qCompress(histData, 9);
        }
        QWidget* mainWidget = m_part ? m_part->widget() : 0;
        QWidget* frameWidget = mainWidget ? mainWidget->parentWidget() : 0;
        if (frameWidget) {
            emit saveHistory(frameWidget, m_historyData);
            // kDebug() << "# of items:" << history->count() << "current item:" << history->currentItemIndex() << "url:" << history->currentItem().url();
        }
    } else {
        Q_ASSERT(false); // should never happen!!!
    }
}

void WebKitBrowserExtension::slotPrintRequested(QWebFrame* frame)
{
    if (!frame)
        return;

    // Make it non-modal, in case a redirection deletes the part
    QPointer<QPrintDialog> dlg (new QPrintDialog(view()));
    if (dlg->exec() == QPrintDialog::Accepted) {
        frame->print(dlg->printer());
    }
    delete dlg;
}

void WebKitBrowserExtension::slotPrintPreview()
{
    // Make it non-modal, in case a redirection deletes the part
    QPointer<QPrintPreviewDialog> dlg (new QPrintPreviewDialog(view()));
    connect(dlg.data(), SIGNAL(paintRequested(QPrinter*)),
            view()->page()->currentFrame(), SLOT(print(QPrinter*)));
    dlg->exec();
    delete dlg;
}

void WebKitBrowserExtension::slotOpenSelection()
{
    QAction *action = qobject_cast<KAction*>(sender());
    if (action) {
        KParts::BrowserArguments browserArgs;
        browserArgs.frameName = "_blank";
        emit openUrlRequest(KUrl(action->data().toUrl()), KParts::OpenUrlArguments(), browserArgs);
    }
}

void WebKitBrowserExtension::slotLinkInTop()
{
    if (!view())
        return;

    KParts::OpenUrlArguments uargs;
    uargs.setActionRequestedByUser(true);

    KParts::BrowserArguments bargs;
    bargs.frameName = QL1S("_top");

    const KUrl url (view()->contextMenuResult().linkUrl());

    emit openUrlRequest(url, uargs, bargs);
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
    return (KParts::SelectorInterface::EntireContent
            | KParts::SelectorInterface::SelectedContent);
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


static QString queryOne(const QString& query)
{
   QString jsQuery = QL1S("(function(query) { var element; var selectedElement = window.getSelection().getRangeAt(0).cloneContents().querySelector(\"");
   jsQuery += query;
   jsQuery += QL1S("\"); if (selectedElement && selectedElement.length > 0) { element = new Object; "
                   "element.tagName = selectedElements[0].tagName; element.href = selectedElements[0].href; } "
                   "return element; }())");
   return jsQuery;
}

static QString queryAll(const QString& query)
{
   QString jsQuery = QL1S("(function(query) { var elements = []; var selectedElements = window.getSelection().getRangeAt(0).cloneContents().querySelectorAll(\"");
   jsQuery += query;
   jsQuery += QL1S("\"); var numSelectedElements = (selectedElements ? selectedElements.length : 0);"
                   "for (var i = 0; i < numSelectedElements; ++i) { var element = new Object; "
                   "element.tagName = selectedElements[i].tagName; element.href = selectedElements[i].href;"
                   "elements.push(element); } return elements; } ())");
   return jsQuery;
}

static KParts::SelectorInterface::Element convertSelectionElement(const QVariant& variant)
{
    KParts::SelectorInterface::Element element;
    if (!variant.isNull() && variant.type() == QVariant::Map) {
        const QVariantMap elementMap (variant.toMap());
        element.setTagName(elementMap.value(QL1S("tagName")).toString());
        element.setAttribute(QL1S("href"), elementMap.value(QL1S("href")).toString());
    }
    return element;
}

static QList<KParts::SelectorInterface::Element> convertSelectionElements(const QVariant& variant)
{
    QList<KParts::SelectorInterface::Element> elements;
    const QVariantList resultList (variant.toList());
    Q_FOREACH(const QVariant& result, resultList) {
        const QVariantMap elementMap = result.toMap();
        KParts::SelectorInterface::Element element;
        element.setTagName(elementMap.value(QL1S("tagName")).toString());
        element.setAttribute(QL1S("href"), elementMap.value(QL1S("href")).toString());
        elements.append(element);
    }
    return elements;
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
    case KParts::SelectorInterface::SelectedContent: {
        QWebFrame* webFrame = part()->view()->page()->mainFrame();
        element = convertSelectionElement(webFrame->evaluateJavaScript(queryOne(query)));
        break;
    }
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
    case KParts::SelectorInterface::SelectedContent: {
        QWebFrame* webFrame = part()->view()->page()->mainFrame();
        elements = convertSelectionElements(webFrame->evaluateJavaScript(queryAll(query)));
        break;
    }
    default:
        break;
    }
    return elements;
}

QVariant KWebKitHtmlExtension::htmlSettingsProperty(KParts::HtmlSettingsInterface::HtmlSettingsType type) const
{
    QWebView* view = part() ? part()->view() : 0;
    QWebPage* page = view ? view->page() : 0;
    QWebSettings* settings = page ? page->settings() : 0;

    if (settings) {
        switch (type) {
        case KParts::HtmlSettingsInterface::AutoLoadImages:
            return settings->testAttribute(QWebSettings::AutoLoadImages);
        case KParts::HtmlSettingsInterface::JavaEnabled:
            return settings->testAttribute(QWebSettings::JavaEnabled);
        case KParts::HtmlSettingsInterface::JavascriptEnabled:
            return settings->testAttribute(QWebSettings::JavascriptEnabled);
        case KParts::HtmlSettingsInterface::PluginsEnabled:
            return settings->testAttribute(QWebSettings::PluginsEnabled);
        case KParts::HtmlSettingsInterface::DnsPrefetchEnabled:
            return settings->testAttribute(QWebSettings::DnsPrefetchEnabled);
        case KParts::HtmlSettingsInterface::MetaRefreshEnabled:
            return view->pageAction(QWebPage::StopScheduledPageRefresh)->isEnabled();
        case KParts::HtmlSettingsInterface::LocalStorageEnabled:
            return settings->testAttribute(QWebSettings::LocalStorageEnabled);
        case KParts::HtmlSettingsInterface::OfflineStorageDatabaseEnabled:
            return settings->testAttribute(QWebSettings::OfflineStorageDatabaseEnabled);
        case KParts::HtmlSettingsInterface::OfflineWebApplicationCacheEnabled:
            return settings->testAttribute(QWebSettings::OfflineWebApplicationCacheEnabled);
        case KParts::HtmlSettingsInterface::PrivateBrowsingEnabled:
            return settings->testAttribute(QWebSettings::PrivateBrowsingEnabled);
        case KParts::HtmlSettingsInterface::UserDefinedStyleSheetURL:
            return settings->userStyleSheetUrl();
        default:
            break;
        }
    }

    return QVariant();
}

bool KWebKitHtmlExtension::setHtmlSettingsProperty(KParts::HtmlSettingsInterface::HtmlSettingsType type, const QVariant& value)
{
    QWebView* view = part() ? part()->view() : 0;
    QWebPage* page = view ? view->page() : 0;
    QWebSettings* settings = page ? page->settings() : 0;

    if (settings) {
        switch (type) {
        case KParts::HtmlSettingsInterface::AutoLoadImages:
            settings->setAttribute(QWebSettings::AutoLoadImages, value.toBool());
            return true;
        case KParts::HtmlSettingsInterface::JavaEnabled:
            settings->setAttribute(QWebSettings::JavaEnabled, value.toBool());
            return true;
        case KParts::HtmlSettingsInterface::JavascriptEnabled:
            settings->setAttribute(QWebSettings::JavascriptEnabled, value.toBool());
            return true;
        case KParts::HtmlSettingsInterface::PluginsEnabled:
            settings->setAttribute(QWebSettings::PluginsEnabled, value.toBool());
            return true;
        case KParts::HtmlSettingsInterface::DnsPrefetchEnabled:
            settings->setAttribute(QWebSettings::DnsPrefetchEnabled, value.toBool());
            return true;
        case KParts::HtmlSettingsInterface::MetaRefreshEnabled:
            view->triggerPageAction(QWebPage::StopScheduledPageRefresh);
            return true;
        case KParts::HtmlSettingsInterface::LocalStorageEnabled:
            settings->setAttribute(QWebSettings::LocalStorageEnabled, value.toBool());
            return true;
        case KParts::HtmlSettingsInterface::OfflineStorageDatabaseEnabled:
            settings->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, value.toBool());
            return true;
        case KParts::HtmlSettingsInterface::OfflineWebApplicationCacheEnabled:
            settings->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, value.toBool());
            return true;
        case KParts::HtmlSettingsInterface::PrivateBrowsingEnabled:
            settings->setAttribute(QWebSettings::PrivateBrowsingEnabled, value.toBool());
            return true;
        case KParts::HtmlSettingsInterface::UserDefinedStyleSheetURL:
            //kDebug() << "Setting user style sheet for" << page << "to" << value.toUrl();
            settings->setUserStyleSheetUrl(value.toUrl());
            return true;
        default:
            break;
        }
    }

    return false;
}

KWebKitPart* KWebKitHtmlExtension::part() const
{
    return static_cast<KWebKitPart*>(parent());
}

KWebKitScriptableExtension::KWebKitScriptableExtension(KWebKitPart* part)
    : ScriptableExtension(part)
{
}

QVariant KWebKitScriptableExtension::rootObject()
{
    return QVariant::fromValue(KParts::ScriptableExtension::Object(this, reinterpret_cast<quint64>(this)));
}

bool KWebKitScriptableExtension::setException (KParts::ScriptableExtension* callerPrincipal, const QString& message)
{
    return KParts::ScriptableExtension::setException (callerPrincipal, message);
}

QVariant KWebKitScriptableExtension::get (KParts::ScriptableExtension* callerPrincipal, quint64 objId, const QString& propName)
{
    //kDebug() << "caller:" << callerPrincipal << "id:" << objId << "propName:" << propName;
    return callerPrincipal->get (0, objId, propName);
}

bool KWebKitScriptableExtension::put (KParts::ScriptableExtension* callerPrincipal, quint64 objId, const QString& propName, const QVariant& value)
{
    return KParts::ScriptableExtension::put (callerPrincipal, objId, propName, value);
}

static QVariant exception(const char* msg)
{
    kWarning() << msg;
    return QVariant::fromValue(KParts::ScriptableExtension::Exception(QString::fromLatin1(msg)));
}

QVariant KWebKitScriptableExtension::evaluateScript (KParts::ScriptableExtension* callerPrincipal,
                                                     quint64 contextObjectId,
                                                     const QString& code,
                                                     KParts::ScriptableExtension::ScriptLanguage lang)
{
    Q_UNUSED(contextObjectId);
    //kDebug() << "principal:" << callerPrincipal << "id:" << contextObjectId << "language:" << lang << "code:" << code;

    if (lang != ECMAScript)
        return exception("unsupported language");


    KParts::ReadOnlyPart* part = callerPrincipal ? qobject_cast<KParts::ReadOnlyPart*>(callerPrincipal->parent()) : 0;
    QWebFrame* frame = part ? qobject_cast<QWebFrame*>(part->parent()) : 0;
    if (!frame)
        return exception("failed to resolve principal");

    QVariant result (frame->evaluateJavaScript(code));

    if (result.type() == QVariant::Map) {
        const QVariantMap map (result.toMap());
        for (QVariantMap::const_iterator it = map.constBegin(), itEnd = map.constEnd(); it != itEnd; ++it) {
            callerPrincipal->put(callerPrincipal, 0, it.key(), it.value());
        }
    } else {
        const QString propName(code.contains(QLatin1String("__nsplugin")) ? QLatin1String("__nsplugin") : QString());
        callerPrincipal->put(callerPrincipal, 0, propName, result.toString());
    }

    return QVariant::fromValue(ScriptableExtension::Null());
}

bool KWebKitScriptableExtension::isScriptLanguageSupported (KParts::ScriptableExtension::ScriptLanguage lang) const
{
    return (lang == KParts::ScriptableExtension::ECMAScript);
}

QVariant KWebKitScriptableExtension::encloserForKid (KParts::ScriptableExtension* kid)
{
    KParts::ReadOnlyPart* part = kid ? qobject_cast<KParts::ReadOnlyPart*>(kid->parent()) : 0;
    QWebFrame* frame = part ? qobject_cast<QWebFrame*>(part->parent()) : 0;
    if (frame) {
        return QVariant::fromValue(KParts::ScriptableExtension::Object(kid, reinterpret_cast<quint64>(kid)));
    }

    return QVariant::fromValue(ScriptableExtension::Null());
}

KWebKitPart* KWebKitScriptableExtension::part()
{
    return qobject_cast<KWebKitPart*>(parent());
}

#include "kwebkitpart_ext.moc"
