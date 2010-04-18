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
#include <KDE/KTemporaryFile>
#include <KDE/KToolInvocation>
#include <KDE/KFileDialog>
#include <KDE/KIO/NetAccess>
#include <KDE/KGlobal>
#include <KDE/KSharedConfig>
#include <KDE/KRun>
#include <KDE/KDebug>
#include <KDE/KPrintPreview>

#include <QtCore/QPointer>
#include <QtGui/QClipboard>
#include <QtGui/QApplication>
#include <QtGui/QPrinter>
#include <QtGui/QPrintPreviewDialog>
#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebElement>
#include <QtWebKit/QWebElementCollection>

#define QL1S(x)     QLatin1String(x)
#define QL1C(x)     QLatin1Char(x)

static QString getFormData(const QWebFrame *frame)
{
    QStringList formData;

    if (frame) {
        QString key, temp;
        QWebElementCollection collection = frame->findAllElements(QL1S("input[type=text], input:not([type]), textarea"));
        QWebElementCollection::iterator it = collection.begin();
        const QWebElementCollection::iterator itEnd = collection.end();
        for (; it != itEnd; ++it) {
            const QString value = (*it).evaluateJavaScript("this.value").toString().trimmed();
            if (!value.isEmpty() && value != (*it).attribute("value")) {
                if ((*it).parent().isNull())
                    key = (*it).tagName();
                else
                    key = (*it).parent().tagName() + QL1S(" > ") + (*it).tagName();
                temp = (*it).attribute(QL1S("name"));
                if (!temp.isEmpty())
                    key += QString::fromLatin1("[name=\"%1\"]").arg(temp);
                temp = (*it).attribute(QL1S("class"));
                if (!temp.isEmpty())
                   key += QString::fromLatin1("[class=\"%1\"]").arg(temp);
                temp = (*it).attribute(QL1S("id"));
                if (!temp.isEmpty())
                   key += QString::fromLatin1("[id=\"%1\"]").arg(temp);

                formData << (key + QL1C(',') + value);
            }
        }
    }

    return formData.join(QL1S(";"));
}


static QStringList getChildrenFrameState(const QWebFrame *frame)
{
    QStringList info;
    if (frame) {
        QListIterator <QWebFrame*> it (frame->childFrames());
        while (it.hasNext()) {
            QWebFrame* childFrame = it.next();
            info << childFrame->frameName();
            info << childFrame->url().toString();
            info << QString::number(childFrame->scrollPosition().x());
            info << QString::number(childFrame->scrollPosition().y());
            info << getFormData(frame);
        }
    }

    return info;
}

class WebKitBrowserExtension::WebKitBrowserExtensionPrivate
{
 public:
    QPointer<KWebKitPart> part;
    QPointer<WebView> view;

};

WebKitBrowserExtension::WebKitBrowserExtension(KWebKitPart *parent)
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

int WebKitBrowserExtension::xOffset()
{
    if (d->view->page())
        return d->view->page()->mainFrame()->scrollPosition().x();

    return KParts::BrowserExtension::xOffset();
}

int WebKitBrowserExtension::yOffset()
{
    if (d->view->page())
        return d->view->page()->mainFrame()->scrollPosition().y();

    return KParts::BrowserExtension::yOffset();
}

void WebKitBrowserExtension::saveState(QDataStream &stream)
{
    QVariant sslinfo;
    QString formData;
    QStringList childFrameState;

    if (d->view) {
        WebPage *page = qobject_cast<WebPage*>(d->view->page());

        if (page) {
            // Save the SSL information...
            sslinfo = page->sslInfo().toMetaData();

            // Save the state (name, url, scroll position) for all frames...
            childFrameState = getChildrenFrameState(page->mainFrame());

            // Save the data typed into forms...
            formData = getFormData(page->mainFrame());
            //kDebug() << "Saving form data:" << formData;
        }
    }

    stream << d->part->url()
           << static_cast<qint32>(xOffset())
           << static_cast<qint32>(yOffset())
           << formData
           << sslinfo
           << childFrameState;
}

void WebKitBrowserExtension::restoreState(QDataStream &stream)
{  
    KUrl u;
    qint32 xOfs, yOfs;
    QVariant sslinfo;
    QString formData;
    KIO::MetaData metaData;
    KParts::OpenUrlArguments args;
    KParts::BrowserArguments bargs;

    stream >> u >> xOfs >> yOfs >> formData >> sslinfo >> bargs.docState;

    if (sslinfo.isValid() && sslinfo.type() == QVariant::Map)
        metaData += sslinfo.toMap();

    args.setXOffset(xOfs);
    args.setYOffset(yOfs);
    args.metaData() = metaData;
    args.metaData().insert(QL1S("kwebkitpart-restore-state"), QString());
    args.metaData().insert(QL1S("kwebkitpart-saved-form-data"), formData);

    d->part->setArguments(args);
    d->part->browserExtension()->setBrowserArguments(bargs);
    d->part->openUrl(u);
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
        d->view->page()->triggerAction(QWebPage::SelectAll);
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
/*
        QList<KUrl> urls;
        urls.append(d->view->contextMenuResult().imageUrl());
        const int nbUrls = urls.count();
        for (int i = 0; i != nbUrls; i++) {
            QString file = KFileDialog::getSaveFileName(KUrl(), QString(), d->part->widget());
            KIO::NetAccess::file_copy(urls.at(i), file, d->part->widget());
        }
*/
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

        KRun::runUrl(currentUrl, QL1S("text/plain"), d->view, isTempFile);
    }
}

void WebKitBrowserExtension::slotViewFrameSource()
{
    if (d->view) {
        KRun::runUrl(KUrl(d->view->page()->currentFrame()->url()), QL1S("text/plain"), d->view, false);
    }
}

#include "kwebkitpart_ext.moc"
