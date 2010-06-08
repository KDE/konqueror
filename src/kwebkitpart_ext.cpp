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

#include <KDE/KUriFilterData>
#include <KDE/KDesktopFile>
#include <KDE/KConfigGroup>
#include <KDE/KToolInvocation>
#include <KDE/KFileDialog>
#include <KDE/KIO/NetAccess>
#include <KDE/KGlobal>
#include <KDE/KSharedConfig>
#include <KDE/KRun>
#include <KDE/KDebug>
#include <KDE/KPrintPreview>
#include <KDE/KStandardDirs>

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
    QFile* historyFile;
};

WebKitBrowserExtension::WebKitBrowserExtension(KWebKitPart *parent)
                       :KParts::BrowserExtension(parent),
                        d (new WebKitBrowserExtensionPrivate)
{
    d->part = parent;
    d->view = qobject_cast<WebView*>(parent->view());
    d->historyFile = new QFile(KStandardDirs::locateLocal("data", "kwebkitpart/autosave/history"));

    enableAction("cut", false);
    enableAction("copy", false);
    enableAction("paste", false);
    enableAction("print", true);
}

WebKitBrowserExtension::~WebKitBrowserExtension()
{
    // If the history file is empty, remove it.
    if (d->historyFile->size() == 0)
        d->historyFile->remove();
    delete d->historyFile;
    delete d;
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
           << static_cast<qint32>(d->view->page()->history()->currentItemIndex());

    // Save the QtWebKit history into its own file...
    if (d->historyFile->isWritable() || d->historyFile->open(QIODevice::WriteOnly)) {
        d->historyFile->seek(0);  // do not append, but rewrite...
        QDataStream output (d->historyFile);
        output << *(d->view->page()->history());
    } else {
        kDebug() << "Unable to open history file:" <<  d->historyFile->fileName();
    }
}

void WebKitBrowserExtension::restoreState(QDataStream &stream)
{
    KUrl u;
    qint32 xOfs, yOfs,historyItemIndex;
    KParts::OpenUrlArguments args;

    stream >> u >> xOfs >> yOfs >> historyItemIndex;
    args.metaData().insert(QL1S("kwebkitpart-restore-state"), QString::number(historyItemIndex));
    d->part->setArguments(args);
    d->part->openUrl(u);
}

void WebKitBrowserExtension::restoreHistory()
{
    // If the history file is open, we are already saving history data ; so we
    // ignore any restore history requests.
    if (!d->historyFile->isOpen() && d->historyFile->open(QIODevice::ReadOnly)) {
        QDataStream stream (d->historyFile);
        stream >> *(d->view->page()->history());
        d->historyFile->close();
    }
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
    if (d->view) {
        // action name is of form "previewProvider[<searchproviderprefix>:]"
        const QString searchProviderPrefix = QString(sender()->objectName()).mid(14);

        const QString text = d->view->selectedText();
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
    if (d->view) {
        emit createNewWindow(d->view->contextMenuResult().imageUrl());
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
        //emit saveUrl(d->view->contextMenuResult().linkUrl());
        d->view->triggerPageAction(QWebPage::DownloadLinkToDisk);
}

void WebKitBrowserExtension::slotViewDocumentSource()
{
    if (d->view) {
        //TODO: we can do better than retreiving the document again!
        KRun::runUrl(d->view->page()->mainFrame()->url(), QL1S("text/plain"), d->view, false);
    }
}

void WebKitBrowserExtension::slotViewFrameSource()
{
    if (d->view) {
        //TODO: we can do better than retreiving the document again!
        KRun::runUrl(KUrl(d->view->page()->currentFrame()->url()), QL1S("text/plain"), d->view, false);
    }
}

#include "kwebkitpart_ext.moc"
