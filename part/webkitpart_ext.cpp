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

#include "webkitpart_ext.h"

#include "webview.h"
#include "webkitpart.h"
#include "settings/webkitsettings.h"

#include <KDE/KUriFilterData>
#include <KDE/KDesktopFile>
#include <KDE/KConfigGroup>
#include <KDE/KTemporaryFile>
#include <KDE/KToolInvocation>
#include <KDE/KFileDialog>
#include <KDE/KIO/NetAccess>
#include <KDE/KGlobal>
#include <KDE/KSharedConfig>
#include <KDE/KRun>

#include <QtCore/QPointer>
#include <QtGui/QClipboard>
#include <QtGui/QApplication>
#include <QtGui/QPrintPreviewDialog>
#include <QtWebKit/QWebFrame>




class WebKitBrowserExtension::WebKitBrowserExtensionPrivate
{
 public:
    QPointer<WebKitPart> part;
    QPointer<WebView> view;
};

WebKitBrowserExtension::WebKitBrowserExtension(WebKitPart *parent)
                       :KParts::BrowserExtension(parent),
                        d (new WebKitBrowserExtensionPrivate)
{
    d->part = parent;
    d->view = qobject_cast<WebView*>(parent->view());

    enableAction("cut", false);
    enableAction("copy", false);
    enableAction("paste", false);
    enableAction("print", true);
}

WebKitBrowserExtension::~WebKitBrowserExtension()
{
    delete d;
}

void WebKitBrowserExtension::cut()
{
    if (d->view)
        d->view->page()->triggerAction(QWebPage::Cut);
}

void WebKitBrowserExtension::copy()
{
    if (d->view)
        d->view->page()->triggerAction(QWebPage::Copy);
}

void WebKitBrowserExtension::paste()
{
    if (d->view)
        d->view->page()->triggerAction(QWebPage::Paste);
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
        QWebPage *page = d->view->page();
        enableAction("cut", page->action(QWebPage::Cut));
        enableAction("copy", page->action(QWebPage::Copy));
        enableAction("paste", page->action(QWebPage::Paste));
    }
}

void WebKitBrowserExtension::searchProvider()
{
    if (d->view) {
        // action name is of form "previewProvider[<searchproviderprefix>:]"
        const QString searchProviderPrefix = QString(sender()->objectName()).mid(14);

        const QString text = d->view->page()->selectedText();
        KUriFilterData data;
        QStringList list;
        data.setData(searchProviderPrefix + text);
        list << "kurisearchfilter" << "kuriikwsfilter";

        if (!KUriFilter::self()->filterUri(data, list)) {
            KDesktopFile file("services", "searchproviders/google.desktop");
            const QString encodedSearchTerm = QUrl::toPercentEncoding(text);
            KConfigGroup cg(file.desktopGroup());
            data.setData(cg.readEntry("Query").replace("\\{@}", encodedSearchTerm));
        }

        KParts::BrowserArguments browserArgs;
        browserArgs.frameName = "_blank";

        emit openUrlRequest(data.uri(), KParts::OpenUrlArguments(), browserArgs);
    }
}

void WebKitBrowserExtension::reparseConfiguration()
{
    // Force the configuration stuff to repase...
    WebKitSettings::self()->init();
}

void WebKitBrowserExtension::zoomIn()
{  
    if (d->view)
#if QT_VERSION >= 0x040500
        d->view->setZoomFactor(d->view->zoomFactor() + 0.1);
#else
        d->view->setTextSizeMultiplier(d->view->textSizeMultiplier() + 0.1);
#endif
}

void WebKitBrowserExtension::zoomOut()
{
    if (d->view)
#if QT_VERSION >= 0x040500
        d->view->setZoomFactor(d->view->zoomFactor() - 0.1);
#else
        d->view->setTextSizeMultiplier(d->view->textSizeMultiplier() - 0.1);
#endif
}

void WebKitBrowserExtension::zoomNormal()
{
    if (d->view)
#if QT_VERSION >= 0x040500
        d->view->setZoomFactor(1);
#else
        d->view->setTextSizeMultiplier(1);
#endif
}

void WebKitBrowserExtension::toogleZoomTextOnly()
{
#if QT_VERSION >= 0x040500
    if (d->view) {
        KConfigGroup cgHtml(KGlobal::config(), "HTML Settings");
        bool zoomTextOnly = cgHtml.readEntry( "ZoomTextOnly", false );
        cgHtml.writeEntry("ZoomTextOnly", !zoomTextOnly);
        KGlobal::config()->reparseConfiguration();

        d->view->settings()->setAttribute(QWebSettings::ZoomTextOnly, !zoomTextOnly);
    }
#endif
}

void WebKitBrowserExtension::slotSelectAll()
{
#if QT_VERSION >= 0x040500
    if (d->view)
        d->view->page()->triggerAction(QWebPage::SelectAll);
#endif
}

void WebKitBrowserExtension::slotFrameInWindow()
{
    if (d->view) {
        KParts::OpenUrlArguments args;// = d->m_khtml->arguments();
        args.metaData()["referrer"] = d->view->contextMenuResult().linkText();
        args.metaData()["forcenewwindow"] = "true";
        emit createNewWindow(d->view->contextMenuResult().linkUrl(), args);
    }
}

void WebKitBrowserExtension::slotFrameInTab()
{
    if (d->view) {
        KParts::OpenUrlArguments args;// = d->m_khtml->arguments();
        args.metaData()["referrer"] = d->view->contextMenuResult().linkText();
        KParts::BrowserArguments browserArgs;//( d->m_khtml->browserExtension()->browserArguments() );
        browserArgs.setNewTab(true);
        emit createNewWindow(d->view->contextMenuResult().linkUrl(), args, browserArgs);
    }
}

void WebKitBrowserExtension::slotFrameInTop()
{
    if (d->view) {
        KParts::OpenUrlArguments args;// = d->m_khtml->arguments();
        args.metaData()["referrer"] = d->view->contextMenuResult().linkText();
        KParts::BrowserArguments browserArgs;//( d->m_khtml->browserExtension()->browserArguments() );
        browserArgs.frameName = "_top";
        emit openUrlRequest(d->view->contextMenuResult().linkUrl(), args, browserArgs);
    }
}

void WebKitBrowserExtension::slotSaveImageAs()
{
    if (d->view) {
        QList<KUrl> urls;
        urls.append(d->view->contextMenuResult().imageUrl());
        const int nbUrls = urls.count();
        for (int i = 0; i != nbUrls; i++) {
            QString file = KFileDialog::getSaveFileName(KUrl(), QString(), d->part->widget());
            KIO::NetAccess::file_copy(urls.at(i), file, d->part->widget());
        }
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
        emit saveUrl(d->view->contextMenuResult().linkUrl());
}

void WebKitBrowserExtension::slotViewDocumentSource()
{
    if (d->view) {
        //TODO test http requests
        KUrl currentUrl(d->view->page()->mainFrame()->url());
        bool isTempFile = false;
    #if 0
        if (!(currentUrl.isLocalFile())/* && KHTMLPageCache::self()->isComplete(d->m_cacheId)*/) { //TODO implement
            KTemporaryFile sourceFile;
    //         sourceFile.setSuffix(defaultExtension());
            sourceFile.setAutoRemove(false);
            if (sourceFile.open()) {
    //             QDataStream stream (&sourceFile);
    //             KHTMLPageCache::self()->saveData(d->m_cacheId, &stream);
                currentUrl = KUrl();
                currentUrl.setPath(sourceFile.fileName());
                isTempFile = true;
            }
        }
    #endif

        KRun::runUrl(currentUrl, QLatin1String("text/plain"), d->view, isTempFile);
    }
}

#include "webkitpart_ext.moc"
