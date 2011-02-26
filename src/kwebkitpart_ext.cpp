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
#include <kdeversion.h>

#include <QtGui/QClipboard>
#include <QtGui/QApplication>
#include <QtGui/QPrinter>
#include <QtGui/QPrintPreviewDialog>
#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebHistory>
#include <QtWebKit/QWebElement>
#include <QtWebKit/QWebElementCollection>

#define QL1S(x)     QLatin1String(x)
#define QL1C(x)     QLatin1Char(x)


WebKitBrowserExtension::WebKitBrowserExtension(KWebKitPart *parent, const QString &historyFileName)
                       :KParts::BrowserExtension(parent),
                        m_part(QWeakPointer<KWebKitPart>(parent)),
                        m_historyFileName(historyFileName)
{
    enableAction("cut", false);
    enableAction("copy", false);
    enableAction("paste", false);
    enableAction("print", true);
}

WebKitBrowserExtension::~WebKitBrowserExtension()
{
}

WebView* WebKitBrowserExtension::view()
{
    if (!m_part)
        return 0;

    if (!m_view)
        m_view = QWeakPointer<WebView>(qobject_cast<WebView*>(m_part.data()->view()));

    return m_view.data();
}

void WebKitBrowserExtension::saveHistoryState()
{
    if (!view())
        return;

    if (!view()->page()->history()->count())
        return;

    KSaveFile saveFile (m_historyFileName, m_part.data()->componentData());
    if (!saveFile.open())
      return;

    //kDebug() << "Saving history data to"  << saveFile.fileName();
    QDataStream stream (&saveFile);
    stream << *(view()->page()->history());
    if (!saveFile.finalize())
        kWarning() << "Failed to save session history to" << saveFile.fileName();

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
    stream << m_part.data()->url()
           << static_cast<qint32>(xOffset())
           << static_cast<qint32>(yOffset())
           << static_cast<qint32>(view()->page()->history()->currentItemIndex())
           << m_historyFileName;
}

void WebKitBrowserExtension::restoreState(QDataStream &stream)
{
    KUrl u;
    KParts::OpenUrlArguments args;
    qint32 xOfs, yOfs, historyItemIndex;

    if (view() && view()->page()->history()->count() > 0) {
        stream >> u >> xOfs >> yOfs >> historyItemIndex;
    } else {
        QString historyFileName;
        stream >> u >> xOfs >> yOfs >> historyItemIndex >> historyFileName;
        //kDebug() << "Attempting to restore history from" << historyFileName;
        QFile file (historyFileName);
        if (file.open(QIODevice::ReadOnly)) {
            QDataStream stream (&file);
            stream >> *(view()->page()->history());
        }

        if (file.exists())
            file.remove();
    }

    // kDebug() << "Restoring item #" << historyItemIndex << "of" << view()->page()->history()->count() << "at offset (" << xOfs << yOfs << ")";
    args.metaData().insert(QL1S("kwebkitpart-restore-state"), QString::number(historyItemIndex));
    args.metaData().insert(QL1S("kwebkitpart-restore-scrollx"), QString::number(xOfs));
    args.metaData().insert(QL1S("kwebkitpart-restore-scrolly"), QString::number(yOfs));
    m_part.data()->setArguments(args);
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
    if (!view())
        return;

    QPrintPreviewDialog dlg(view());
    connect(&dlg, SIGNAL(paintRequested(QPrinter *)),
            view(), SLOT(print(QPrinter *)));
    dlg.exec();
}

void WebKitBrowserExtension::printFrame()
{
    if (!view())
        return;

    QPrintPreviewDialog dlg(view());
    connect(&dlg, SIGNAL(paintRequested(QPrinter *)),
            view()->page()->currentFrame(), SLOT(print(QPrinter *)));
    dlg.exec();
}

void WebKitBrowserExtension::updateEditActions()
{
    if (!view())
        return;

    enableAction("cut", view()->pageAction(QWebPage::Cut));
    enableAction("copy", view()->pageAction(QWebPage::Copy));
    enableAction("paste", view()->pageAction(QWebPage::Paste));
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
    emit createNewWindow(view()->page()->currentFrame()->url(), KParts::OpenUrlArguments(), bargs);
}

void WebKitBrowserExtension::slotFrameInTab()
{
    if (!view())
        return;

    KParts::BrowserArguments bargs;//( m_m_khtml->browserExtension()->browserArguments() );
    bargs.setNewTab(true);
    emit createNewWindow(view()->page()->currentFrame()->url(), KParts::OpenUrlArguments(), bargs);

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
    WebKitSettings::self()->addAdFilter(url.toString(QUrl::RemoveAuthority));
    reparseConfiguration();
}

void WebKitBrowserExtension::slotCopyLinkURL()
{
    if (!view())
        return;

    KUrl safeURL(view()->contextMenuResult().linkUrl());
    safeURL.setPass(QString());
    // Set it in both the mouse selection and in the clipboard
    QMimeData* mimeData = new QMimeData;
    safeURL.populateMimeData(mimeData);
    QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);

    mimeData = new QMimeData;
    safeURL.populateMimeData(mimeData);
    QApplication::clipboard()->setMimeData(mimeData, QClipboard::Selection);
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
#if 1
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
#if 1
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

    return KUrl(frame->baseUrl().resolved(QUrl::fromEncoded(src.toAscii(), QUrl::StrictMode)));
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
    // looks a bit slow
    return !part()->view()->selectedText().isEmpty();
}

QString KWebKitTextExtension::selectedText(Format format) const
{
    switch(format) {
    case PlainText:
        return part()->view()->selectedText();
    case HTML:
        // selectedTextAsHTML is missing in QWebKit:
        // https://bugs.webkit.org/show_bug.cgi?id=35028
        return part()->view()->page()->currentFrame()->toHtml();
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
{   // Hmm... QWebPage needs something faster than this to check
    // whether there is selected content...
    return !part()->view()->selectedText().isEmpty();
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
