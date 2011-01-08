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


WebKitBrowserExtension::WebKitBrowserExtension(KWebKitPart *parent, const QString &historyFileName)
                       :KParts::BrowserExtension(parent), m_part(parent), m_historyFileName(historyFileName)
{
    if (parent)
      m_view = qobject_cast<WebView*>(parent->view());

    enableAction("cut", false);
    enableAction("copy", false);
    enableAction("paste", false);
    enableAction("print", true);
}

WebKitBrowserExtension::~WebKitBrowserExtension()
{
}

void WebKitBrowserExtension::saveHistoryState()
{
    if (m_view->page()->history()->count()) {
        KSaveFile saveFile (m_historyFileName, m_part->componentData());
        if (saveFile.open()) {
            //kDebug() << "Saving history data to"  << saveFile.fileName();
            QDataStream stream (&saveFile);
            stream << *(m_view->page()->history());
            if (!saveFile.finalize())
                kWarning() << "Failed to save session history to" << saveFile.fileName();
        }
    }
}

int WebKitBrowserExtension::xOffset()
{
    if (m_view)
        return m_view->page()->mainFrame()->scrollPosition().x();

    return KParts::BrowserExtension::xOffset();
}

int WebKitBrowserExtension::yOffset()
{
    if (m_view)
        return m_view->page()->mainFrame()->scrollPosition().y();

    return KParts::BrowserExtension::yOffset();
}

void WebKitBrowserExtension::saveState(QDataStream &stream)
{
    stream << m_part->url()
           << static_cast<qint32>(xOffset())
           << static_cast<qint32>(yOffset())
           << static_cast<qint32>(m_view->page()->history()->currentItemIndex())
           << m_historyFileName;
}

void WebKitBrowserExtension::restoreState(QDataStream &stream)
{
    KUrl u;
    KParts::OpenUrlArguments args;
    qint32 xOfs, yOfs, historyItemIndex;

    if (m_view->page()->history()->count() > 0) {
        stream >> u >> xOfs >> yOfs >> historyItemIndex;
    } else {
        QString historyFileName;
        stream >> u >> xOfs >> yOfs >> historyItemIndex >> historyFileName;
        //kDebug() << "Attempting to restore history from" << historyFileName;
        QFile file (historyFileName);
        if (file.open(QIODevice::ReadOnly)) {
            QDataStream stream (&file);
            stream >> *(m_view->page()->history());
        }

        if (file.exists())
            file.remove();
    }

    // kDebug() << "Restoring item #" << historyItemIndex << "of" << m_view->page()->history()->count() << "at offset (" << xOfs << yOfs << ")";
    args.metaData().insert(QL1S("kwebkitpart-restore-state"), QString::number(historyItemIndex));
    args.metaData().insert(QL1S("kwebkitpart-restore-scrollx"), QString::number(xOfs));
    args.metaData().insert(QL1S("kwebkitpart-restore-scrolly"), QString::number(yOfs));
    m_part->setArguments(args);
    m_part->openUrl(u);
}

void WebKitBrowserExtension::cut()
{
    if (m_view)
        m_view->triggerPageAction(QWebPage::Cut);
}

void WebKitBrowserExtension::copy()
{
    if (m_view)
        m_view->triggerPageAction(QWebPage::Copy);
}

void WebKitBrowserExtension::paste()
{
    if (m_view)
        m_view->triggerPageAction(QWebPage::Paste);
}

void WebKitBrowserExtension::slotSaveDocument()
{
    if (m_view)
        emit saveUrl(m_view->url());
}

void WebKitBrowserExtension::slotSaveFrame()
{
    if (m_view)
        emit saveUrl(m_view->page()->currentFrame()->url());
}

void WebKitBrowserExtension::print()
{
    if (m_view) {
        QPrintPreviewDialog dlg(m_view);
        connect(&dlg, SIGNAL(paintRequested(QPrinter *)),
                m_view, SLOT(print(QPrinter *)));
        dlg.exec();
    }
}

void WebKitBrowserExtension::printFrame()
{
    if (m_view) {
        QPrintPreviewDialog dlg(m_view);
        connect(&dlg, SIGNAL(paintRequested(QPrinter *)),
                m_view->page()->currentFrame(), SLOT(print(QPrinter *)));
        dlg.exec();
    }
}

void WebKitBrowserExtension::updateEditActions()
{
    if (m_view) {
        enableAction("cut", m_view->pageAction(QWebPage::Cut));
        enableAction("copy", m_view->pageAction(QWebPage::Copy));
        enableAction("paste", m_view->pageAction(QWebPage::Paste));
    }
}

void WebKitBrowserExtension::searchProvider()
{
    if (!m_view)
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
    if (m_view)
        m_view->setZoomFactor(m_view->zoomFactor() + 0.1);
}

void WebKitBrowserExtension::zoomOut()
{
    if (m_view)
        m_view->setZoomFactor(m_view->zoomFactor() - 0.1);
}

void WebKitBrowserExtension::zoomNormal()
{
    if (m_view)
        m_view->setZoomFactor(1);
}

void WebKitBrowserExtension::toogleZoomTextOnly()
{
    if (m_view) {
        KConfigGroup cgHtml(KGlobal::config(), "HTML Settings");
        bool zoomTextOnly = cgHtml.readEntry( "ZoomTextOnly", false );
        cgHtml.writeEntry("ZoomTextOnly", !zoomTextOnly);
        KGlobal::config()->reparseConfiguration();

        m_view->settings()->setAttribute(QWebSettings::ZoomTextOnly, !zoomTextOnly);
    }
}

void WebKitBrowserExtension::slotSelectAll()
{
    if (m_view)
        m_view->triggerPageAction(QWebPage::SelectAll);
}

void WebKitBrowserExtension::slotFrameInWindow()
{
    if (m_view) {
        KParts::OpenUrlArguments args;// = m_m_khtml->arguments();
        args.metaData()["forcenewwindow"] = "true";
        emit createNewWindow(m_view->page()->currentFrame()->url(), args);
    }
}

void WebKitBrowserExtension::slotFrameInTab()
{
    if (m_view) {
        KParts::BrowserArguments browserArgs;//( m_m_khtml->browserExtension()->browserArguments() );
        browserArgs.setNewTab(true);
        emit createNewWindow(m_view->page()->currentFrame()->url(), KParts::OpenUrlArguments(), browserArgs);
    }
}

void WebKitBrowserExtension::slotFrameInTop()
{
    if (m_view) {
        KParts::BrowserArguments browserArgs;//( m_m_khtml->browserExtension()->browserArguments() );
        browserArgs.frameName = "_top";
        emit openUrlRequest(m_view->page()->currentFrame()->url(), KParts::OpenUrlArguments(), browserArgs);
    }
}

void WebKitBrowserExtension::slotReloadFrame()
{
    if (m_view) {
        m_view->page()->currentFrame()->load(m_view->page()->currentFrame()->url());
    }
}

void WebKitBrowserExtension::slotSaveImageAs()
{
    if (m_view) {
        m_view->triggerPageAction(QWebPage::DownloadImageToDisk);
    }
}

void WebKitBrowserExtension::slotSendImage()
{
    if (m_view) {
        QStringList urls;
        urls.append(m_view->contextMenuResult().imageUrl().path());
        const QString subject = m_view->contextMenuResult().imageUrl().path();
        KToolInvocation::invokeMailer(QString(), QString(), QString(), subject,
                                      QString(), //body
                                      QString(),
                                      urls); // attachments
    }
}

void WebKitBrowserExtension::slotCopyImage()
{
    if (m_view) {
        KUrl safeURL(m_view->contextMenuResult().imageUrl());
        safeURL.setPass(QString());

        // Set it in both the mouse selection and in the clipboard
        QMimeData* mimeData = new QMimeData;
        mimeData->setImageData(m_view->contextMenuResult().pixmap());
        safeURL.populateMimeData(mimeData);
        QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);

        mimeData = new QMimeData;
        mimeData->setImageData(m_view->contextMenuResult().pixmap());
        safeURL.populateMimeData(mimeData);
        QApplication::clipboard()->setMimeData(mimeData, QClipboard::Selection);
    }
}

void WebKitBrowserExtension::slotViewImage()
{
    if (m_view)
        emit createNewWindow(m_view->contextMenuResult().imageUrl());
}

void WebKitBrowserExtension::slotBlockImage()
{
    if (!m_view)
        return;
    
    bool ok = false;
    const QString url = KInputDialog::getText(i18n("Add URL to Filter"),
                                              i18n("Enter the URL:"),
                                              m_view->contextMenuResult().imageUrl().toString(),
                                              &ok);
    if (ok)
        WebKitSettings::self()->addAdFilter(url);
}

void WebKitBrowserExtension::slotBlockHost()
{
    if (!m_view)
        return;
    
    QUrl url (m_view->contextMenuResult().imageUrl());
    url.setPath(QL1S("/*"));
    WebKitSettings::self()->addAdFilter(url.toString(QUrl::RemoveAuthority));
}

void WebKitBrowserExtension::slotCopyLinkLocation()
{
    if (m_view) {
        KUrl safeURL(m_view->contextMenuResult().linkUrl());
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
    if (m_view)
        //emit saveUrl(m_view->contextMenuResult().linkUrl());
        m_view->triggerPageAction(QWebPage::DownloadLinkToDisk);
}

void WebKitBrowserExtension::slotViewDocumentSource()
{
    if (m_view) {
#if 1
        //FIXME: This workaround is necessary because freakin' QtWebKit does not provide
        //a means to obtain the original content of the frame. Actually it does, but the
        //returned content is royally screwed up! *sigh*
        KRun::runUrl(m_view->page()->mainFrame()->url(), QL1S("text/plain"), m_view, false);
#else
        KTemporaryFile tempFile;
        tempFile.setSuffix(QL1S(".html"));
        tempFile.setAutoRemove(false);
        if (tempFile.open()) {
            tempFile.write(m_view->page()->mainFrame()->toHtml().toUtf8());
            KRun::runUrl(tempFile.fileName(), QL1S("text/plain"), m_view, true, false);
        }
#endif
    }
}

void WebKitBrowserExtension::slotViewFrameSource()
{
  if (m_view) {
#if 1
      //FIXME: This workaround is necessary because freakin' QtWebKit does not provide
      //a means to obtain the original content of the frame. Actually it does, but the
      //returned content is royally screwed up! *sigh*
      KRun::runUrl(m_view->page()->mainFrame()->url(), QL1S("text/plain"), m_view, false);
#else
      KTemporaryFile tempFile;
      tempFile.setSuffix(QL1S(".html"));
      tempFile.setAutoRemove(false);
      if (tempFile.open()) {
          tempFile.write(m_view->page()->currentFrame()->toHtml().toUtf8());
          KRun::runUrl(tempFile.fileName(), QL1S("text/plain"), m_view, true, false);
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
