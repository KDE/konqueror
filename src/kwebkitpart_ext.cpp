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
#include "websslinfo.h"
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

#include <QtCore/QPointer>
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

class WebKitBrowserExtension::WebKitBrowserExtensionPrivate
{
 public:
    WebKitBrowserExtensionPrivate() {}

    QPointer<KWebKitPart> part;
    QPointer<WebView> view;
    QString historyFileName;
};


WebKitBrowserExtension::WebKitBrowserExtension(KWebKitPart *parent, const QString &historyFileName)
                       :KParts::BrowserExtension(parent),
                        d (new WebKitBrowserExtensionPrivate)
{
    d->part = parent;
    d->view = qobject_cast<WebView*>(parent->view());
    d->historyFileName = historyFileName;

    enableAction("cut", false);
    enableAction("copy", false);
    enableAction("paste", false);
    enableAction("print", true);
}

WebKitBrowserExtension::~WebKitBrowserExtension()
{
    delete d;
}

void WebKitBrowserExtension::saveHistoryState()
{
    if (d->view->page()->history()->count()) {
        KSaveFile saveFile (d->historyFileName, d->part->componentData());
        if (saveFile.open()) {
            //kDebug() << "Saving history data to"  << saveFile.fileName();
            QDataStream stream (&saveFile);
            stream << *(d->view->page()->history());
            if (!saveFile.finalize())
                kWarning() << "Failed to save session history to" << saveFile.fileName();
        }
    }
}

int WebKitBrowserExtension::xOffset()
{
    if (d->view)
        return d->view->page()->mainFrame()->scrollPosition().x();

    return KParts::BrowserExtension::xOffset();
}

int WebKitBrowserExtension::yOffset()
{
    if (d->view)
        return d->view->page()->mainFrame()->scrollPosition().y();

    return KParts::BrowserExtension::yOffset();
}

void WebKitBrowserExtension::saveState(QDataStream &stream)
{
    stream << d->part->url()
           << static_cast<qint32>(xOffset())
           << static_cast<qint32>(yOffset())
           << static_cast<qint32>(d->view->page()->history()->currentItemIndex())
           << d->historyFileName;
}

void WebKitBrowserExtension::restoreState(QDataStream &stream)
{
    Q_ASSERT(d);

    KUrl u;
    KParts::OpenUrlArguments args;
    qint32 xOfs, yOfs, historyItemIndex;

    if (d->view->page()->history()->count() > 0) {
        stream >> u >> xOfs >> yOfs >> historyItemIndex;
    } else {
        QString historyFileName;
        stream >> u >> xOfs >> yOfs >> historyItemIndex >> historyFileName;
        //kDebug() << "Attempting to restore history from" << historyFileName;
        QFile file (historyFileName);
        if (file.open(QIODevice::ReadOnly)) {
            QDataStream stream (&file);
            stream >> *(d->view->page()->history());
        }

        if (file.exists())
            file.remove();
    }

    // kDebug() << "Restoring item #" << historyItemIndex << "of" << d->view->page()->history()->count() << "at offset (" << xOfs << yOfs << ")";
    args.metaData().insert(QL1S("kwebkitpart-restore-state"), QString::number(historyItemIndex));
    args.metaData().insert(QL1S("kwebkitpart-restore-scrollx"), QString::number(xOfs));
    args.metaData().insert(QL1S("kwebkitpart-restore-scrolly"), QString::number(yOfs));
    d->part->setArguments(args);
    d->part->openUrl(u);
}

void WebKitBrowserExtension::cut()
{
    if (d->view)
        d->view->triggerPageAction(QWebPage::Cut);
}

void WebKitBrowserExtension::copy()
{
    if (d->view)
        d->view->triggerPageAction(QWebPage::Copy);
}

void WebKitBrowserExtension::paste()
{
    if (d->view)
        d->view->triggerPageAction(QWebPage::Paste);
}

void WebKitBrowserExtension::slotSaveDocument()
{
    if (d->view)
        emit saveUrl(d->view->url());
}

void WebKitBrowserExtension::slotSaveFrame()
{
    if (d->view)
        emit saveUrl(d->view->page()->currentFrame()->url());
}

void WebKitBrowserExtension::print()
{
    if (d->view) {
        QPrintPreviewDialog dlg(d->view);
        connect(&dlg, SIGNAL(paintRequested(QPrinter *)),
                d->view, SLOT(print(QPrinter *)));
        dlg.exec();
    }
}

void WebKitBrowserExtension::printFrame()
{
    if (d->view) {
        QPrintPreviewDialog dlg(d->view);
        connect(&dlg, SIGNAL(paintRequested(QPrinter *)),
                d->view->page()->currentFrame(), SLOT(print(QPrinter *)));
        dlg.exec();
    }
}

void WebKitBrowserExtension::updateEditActions()
{
    if (d->view) {
        enableAction("cut", d->view->pageAction(QWebPage::Cut));
        enableAction("copy", d->view->pageAction(QWebPage::Copy));
        enableAction("paste", d->view->pageAction(QWebPage::Paste));
    }
}

void WebKitBrowserExtension::searchProvider()
{
    if (!d->view)
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
    
    KParts::BrowserArguments browserArgs;
    browserArgs.frameName = "_blank";
    emit openUrlRequest(url, KParts::OpenUrlArguments(), browserArgs);
}

void WebKitBrowserExtension::reparseConfiguration()
{
    // Force the configuration stuff to reparse...
    WebKitSettings::self()->init();
}

void WebKitBrowserExtension::zoomIn()
{
    if (d->view)
        d->view->setZoomFactor(d->view->zoomFactor() + 0.1);
}

void WebKitBrowserExtension::zoomOut()
{
    if (d->view)
        d->view->setZoomFactor(d->view->zoomFactor() - 0.1);
}

void WebKitBrowserExtension::zoomNormal()
{
    if (d->view)
        d->view->setZoomFactor(1);
}

void WebKitBrowserExtension::toogleZoomTextOnly()
{
    if (d->view) {
        KConfigGroup cgHtml(KGlobal::config(), "HTML Settings");
        bool zoomTextOnly = cgHtml.readEntry( "ZoomTextOnly", false );
        cgHtml.writeEntry("ZoomTextOnly", !zoomTextOnly);
        KGlobal::config()->reparseConfiguration();

        d->view->settings()->setAttribute(QWebSettings::ZoomTextOnly, !zoomTextOnly);
    }
}

void WebKitBrowserExtension::slotSelectAll()
{
    if (d->view)
        d->view->triggerPageAction(QWebPage::SelectAll);
}

void WebKitBrowserExtension::slotFrameInWindow()
{
    if (d->view) {
        KParts::OpenUrlArguments args;// = d->m_khtml->arguments();
        args.metaData()["forcenewwindow"] = "true";
        emit createNewWindow(d->view->page()->currentFrame()->url(), args);
    }
}

void WebKitBrowserExtension::slotFrameInTab()
{
    if (d->view) {
        KParts::BrowserArguments browserArgs;//( d->m_khtml->browserExtension()->browserArguments() );
        browserArgs.setNewTab(true);
        emit createNewWindow(d->view->page()->currentFrame()->url(), KParts::OpenUrlArguments(), browserArgs);
    }
}

void WebKitBrowserExtension::slotFrameInTop()
{
    if (d->view) {
        KParts::BrowserArguments browserArgs;//( d->m_khtml->browserExtension()->browserArguments() );
        browserArgs.frameName = "_top";
        emit openUrlRequest(d->view->page()->currentFrame()->url(), KParts::OpenUrlArguments(), browserArgs);
    }
}

void WebKitBrowserExtension::slotReloadFrame()
{
    if (d->view) {
        d->view->page()->currentFrame()->load(d->view->page()->currentFrame()->url());
    }
}

void WebKitBrowserExtension::slotSaveImageAs()
{
    if (d->view) {
        d->view->triggerPageAction(QWebPage::DownloadImageToDisk);
    }
}

void WebKitBrowserExtension::slotSendImage()
{
    if (d->view) {
        QStringList urls;
        urls.append(d->view->contextMenuResult().imageUrl().path());
        const QString subject = d->view->contextMenuResult().imageUrl().path();
        KToolInvocation::invokeMailer(QString(), QString(), QString(), subject,
                                      QString(), //body
                                      QString(),
                                      urls); // attachments
    }
}

void WebKitBrowserExtension::slotCopyImage()
{
    if (d->view) {
        KUrl safeURL(d->view->contextMenuResult().imageUrl());
        safeURL.setPass(QString());

        // Set it in both the mouse selection and in the clipboard
        QMimeData* mimeData = new QMimeData;
        mimeData->setImageData(d->view->contextMenuResult().pixmap());
        safeURL.populateMimeData(mimeData);
        QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);

        mimeData = new QMimeData;
        mimeData->setImageData(d->view->contextMenuResult().pixmap());
        safeURL.populateMimeData(mimeData);
        QApplication::clipboard()->setMimeData(mimeData, QClipboard::Selection);
    }
}

void WebKitBrowserExtension::slotViewImage()
{
    if (d->view)
        emit createNewWindow(d->view->contextMenuResult().imageUrl());
}

void WebKitBrowserExtension::slotBlockImage()
{
    if (!d->view)
        return;
    
    bool ok = false;
    const QString url = KInputDialog::getText(i18n("Add URL to Filter"),
                                              i18n("Enter the URL:"),
                                              d->view->contextMenuResult().imageUrl().toString(),
                                              &ok);
    if (ok)
        WebKitSettings::self()->addAdFilter(url);
}

void WebKitBrowserExtension::slotBlockHost()
{
    if (!d->view)
        return;
    
    QUrl url (d->view->contextMenuResult().imageUrl());
    url.setPath(QL1S("/*"));
    WebKitSettings::self()->addAdFilter(url.toString(QUrl::RemoveAuthority));
}

void WebKitBrowserExtension::slotCopyLinkLocation()
{
    if (d->view) {
        KUrl safeURL(d->view->contextMenuResult().linkUrl());
        safeURL.setPass(QString());
        // Set it in both the mouse selection and in the clipboard
        QMimeData* mimeData = new QMimeData;
        safeURL.populateMimeData(mimeData);
        QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);

        mimeData = new QMimeData;
        safeURL.populateMimeData(mimeData);
        QApplication::clipboard()->setMimeData(mimeData, QClipboard::Selection);
    }
}

void WebKitBrowserExtension::slotSaveLinkAs()
{
    if (d->view)
        //emit saveUrl(d->view->contextMenuResult().linkUrl());
        d->view->triggerPageAction(QWebPage::DownloadLinkToDisk);
}

void WebKitBrowserExtension::slotViewDocumentSource()
{
    if (d->view) {
#if 1
        //FIXME: This workaround is necessary because freakin' QtWebKit does not provide
        //a means to obtain the original content of the frame. Actually it does, but the
        //returned content is royally screwed up! *sigh*
        KRun::runUrl(d->view->page()->mainFrame()->url(), QL1S("text/plain"), d->view, false);
#else
        KTemporaryFile tempFile;
        tempFile.setSuffix(QL1S(".html"));
        tempFile.setAutoRemove(false);
        if (tempFile.open()) {
            tempFile.write(d->view->page()->mainFrame()->toHtml().toUtf8());
            KRun::runUrl(tempFile.fileName(), QL1S("text/plain"), d->view, true, false);
        }
#endif
    }
}

void WebKitBrowserExtension::slotViewFrameSource()
{
  if (d->view) {
#if 1
      //FIXME: This workaround is necessary because freakin' QtWebKit does not provide
      //a means to obtain the original content of the frame. Actually it does, but the
      //returned content is royally screwed up! *sigh*
      KRun::runUrl(d->view->page()->mainFrame()->url(), QL1S("text/plain"), d->view, false);
#else
      KTemporaryFile tempFile;
      tempFile.setSuffix(QL1S(".html"));
      tempFile.setAutoRemove(false);
      if (tempFile.open()) {
          tempFile.write(d->view->page()->currentFrame()->toHtml().toUtf8());
          KRun::runUrl(tempFile.fileName(), QL1S("text/plain"), d->view, true, false);
      }
#endif
  }
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
