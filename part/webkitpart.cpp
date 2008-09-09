/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
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

#include "webkitpart.h"

#include "webview.h"

#include <KDE/KParts/GenericFactory>
#include <KDE/KParts/Plugin>
#include <KDE/KAboutData>
#include <KDE/KUriFilterData>
#include <KDE/KDesktopFile>
#include <KDE/KConfigGroup>
#include <KDE/KAction>
#include <KDE/KActionCollection>
#include <KDE/KRun>
#include <KDE/KTemporaryFile>
#include <KDE/KToolInvocation>
#include <KDE/KIO/NetAccess>
#include <KDE/KFileDialog>

#include <QHttpRequestHeader>
#include <QtWebKit/QWebHistory>
#include <QtWebKit/QWebHitTestResult>
#include <QClipboard>
#include <QApplication>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QPrintPreviewDialog>

WebKitPart::WebKitPart(QWidget *parentWidget, QObject *parent, const QStringList &/*args*/)
    : KParts::ReadOnlyPart(parent)
{
    setWidget(new QWidget(parentWidget));
    QVBoxLayout* lay = new QVBoxLayout(widget());
    lay->setMargin(0);
    lay->setSpacing(0);
    m_webView = new WebView(this, widget());
    lay->addWidget(m_webView);
    lay->addWidget(m_webView->searchBar());

    KAboutData* about = new KAboutData("webkitpart", 0, ki18n("WebKit HTML Component"),
                           /*version*/ "0.1", ki18n(/*shortDescription*/ ""),
                           KAboutData::License_LGPL,
                           ki18n("(c) 2008, Urs Wolfer\n"
                                 "(c) 2007 Trolltech ASA"));

    about->addAuthor(ki18n("Laurent Montel"), KLocalizedString(), "montel@kde.org");
    about->addAuthor(ki18n("Michael Howell"), KLocalizedString(), "mhowell123@gmail.com");
    about->addAuthor(ki18n("Urs Wolfer"), KLocalizedString(), "uwolfer@kde.org");
    about->addAuthor(ki18n("Dirk Mueller"), KLocalizedString(), "mueller@kde.org");
    KComponentData componentData(about);
    setComponentData(componentData);

    connect(m_webView, SIGNAL(loadStarted()),
            this, SLOT(loadStarted()));
    connect(m_webView, SIGNAL(loadFinished(bool)),
            this, SLOT(loadFinished()));
    connect(m_webView, SIGNAL(titleChanged(const QString &)),
            this, SIGNAL(setWindowCaption(const QString &)));

    connect(m_webView->page(), SIGNAL(linkHovered(const QString &, const QString &, const QString &)),
            this, SIGNAL(setStatusBarText(const QString &)));

    m_browserExtension = new WebKitBrowserExtension(this);

    connect(m_webView->page(), SIGNAL(loadProgress(int)),
            m_browserExtension, SIGNAL(loadingProgress(int)));
    connect(m_webView, SIGNAL(urlChanged(const QUrl &)),
            this, SLOT(urlChanged(const QUrl &)));

    initAction();

    setXMLFile("webkitpart.rc");
}

WebKitPart::~WebKitPart()
{
}

void WebKitPart::initAction()
{
    KAction *action = new KAction(KIcon("format-font-size-more"), i18n("Enlarge Font"), this);
    actionCollection()->addAction("incFontSizes", action);
    action->setShortcut(KShortcut("CTRL++; CTRL+="));
    connect(action, SIGNAL(triggered(bool)), m_browserExtension, SLOT(zoomIn()));

    action = new KAction(KIcon("format-font-size-less"), i18n("Shrink Font"), this);
    actionCollection()->addAction("decFontSizes", action);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Minus));
    connect(action, SIGNAL(triggered(bool)), m_browserExtension, SLOT(zoomOut()));

    action = actionCollection()->addAction(KStandardAction::SelectAll, "selectAll",
                                           m_browserExtension, SLOT(slotSelectAll()));

    action = new KAction(i18n("View Do&cument Source"), this);
    actionCollection()->addAction("viewDocumentSource", action);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_U));
    connect(action, SIGNAL(triggered(bool)), m_browserExtension, SLOT(slotViewDocumentSource()));

    action = actionCollection()->addAction(KStandardAction::Find, "find", m_webView->searchBar(), SLOT(show()));
    action->setWhatsThis(i18n("Find text<br /><br />"
                              "Shows a dialog that allows you to find text on the displayed page."));
}

bool WebKitPart::openUrl(const KUrl &url)
{
    setUrl(url);

    m_webView->load(url);

    return true;
}

bool WebKitPart::closeUrl()
{
    m_webView->stop();
    return true;
}

WebKitBrowserExtension *WebKitPart::browserExtension() const
{
    return m_browserExtension;
}

bool WebKitPart::openFile()
{
    // never reached
    return false;
}

void WebKitPart::loadStarted()
{
    emit started(0);
}

void WebKitPart::loadFinished()
{
    emit completed();
}

void WebKitPart::urlChanged(const QUrl &url)
{
    const QList<QWebHistoryItem> backItemsList = view()->history()->backItems(2);
    if (backItemsList.count() > 0)
        kDebug() << backItemsList.at(0).url() << url;

    if (!((backItemsList.count() > 0) && (backItemsList.at(0).url() == url))) {
        emit m_browserExtension->openUrlNotify();
    }

    setUrl(url);
    emit m_browserExtension->setLocationBarUrl(KUrl(url).prettyUrl());
}

WebView * WebKitPart::view()
{
    return m_webView;
}

void WebKitPart::setStatusBarTextProxy(const QString &message)
{
    emit setStatusBarText(message);
}


WebKitBrowserExtension::WebKitBrowserExtension(WebKitPart *parent)
    : KParts::BrowserExtension(parent), part(parent)
{
    connect(part->view()->page(), SIGNAL(selectionChanged()),
            this, SLOT(updateEditActions()));
    connect(part->view(), SIGNAL(openUrl(const KUrl &)), this, SIGNAL(openUrlRequest(const KUrl &)));
    connect(part->view(), SIGNAL(openUrlInNewTab(const KUrl &)), this, SIGNAL(createNewWindow(const KUrl &)));

    enableAction("cut", false);
    enableAction("copy", false);
    enableAction("paste", false);
    enableAction("print", true);
}

void WebKitBrowserExtension::cut()
{
    part->view()->page()->triggerAction(KWebPage::Cut);
}

void WebKitBrowserExtension::copy()
{
    part->view()->page()->triggerAction(KWebPage::Copy);
}

void WebKitBrowserExtension::paste()
{
    part->view()->page()->triggerAction(KWebPage::Paste);
}

void WebKitBrowserExtension::print()
{
    QPrintPreviewDialog dlg(part->view());
    connect(&dlg, SIGNAL(paintRequested(QPrinter *)),
            part->view(), SLOT(print(QPrinter *)));
    dlg.exec();
}

void WebKitBrowserExtension::updateEditActions()
{
    KWebPage *page = part->view()->page();
    enableAction("cut", page->action(KWebPage::Cut));
    enableAction("copy", page->action(KWebPage::Copy));
    enableAction("paste", page->action(KWebPage::Paste));
}

void WebKitBrowserExtension::searchProvider()
{
    // action name is of form "previewProvider[<searchproviderprefix>:]"
    const QString searchProviderPrefix = QString(sender()->objectName()).mid(14);

    const QString text = part->view()->page()->selectedText();
    KUriFilterData data;
    QStringList list;
    data.setData(searchProviderPrefix + text);
    list << "kurisearchfilter" << "kuriikwsfilter";

    if (!KUriFilter::self()->filterUri(data, list)) {
        KDesktopFile file("services", "searchproviders/google.desktop");
        QString encodedSearchTerm = QUrl::toPercentEncoding(text);
        KConfigGroup cg(file.desktopGroup());
        data.setData(cg.readEntry("Query").replace("\\{@}", encodedSearchTerm));
    }

    KParts::BrowserArguments browserArgs;
    browserArgs.frameName = "_blank";

    emit openUrlRequest(data.uri(), KParts::OpenUrlArguments(), browserArgs);
}

void WebKitBrowserExtension::zoomIn()
{
    part->view()->setTextSizeMultiplier(part->view()->textSizeMultiplier() * 2);
}

void WebKitBrowserExtension::zoomOut()
{
    part->view()->setTextSizeMultiplier(part->view()->textSizeMultiplier() / 2);
}

void WebKitBrowserExtension::slotSelectAll()
{
//     part->view()->selectAll(); //TODO implement in QWebView (?)
}

void WebKitBrowserExtension::slotFrameInWindow()
{
    KParts::OpenUrlArguments args;// = d->m_khtml->arguments();
    args.metaData()["referrer"] = part->view()->contextMenuResult().linkText();
    args.metaData()["forcenewwindow"] = "true";
    emit createNewWindow(part->view()->contextMenuResult().linkUrl(), args);
}

void WebKitBrowserExtension::slotFrameInTab()
{
    KParts::OpenUrlArguments args;// = d->m_khtml->arguments();
    args.metaData()["referrer"] = part->view()->contextMenuResult().linkText();
    KParts::BrowserArguments browserArgs;//( d->m_khtml->browserExtension()->browserArguments() );
    browserArgs.setNewTab(true);
    emit createNewWindow(part->view()->contextMenuResult().linkUrl(), args, browserArgs);
}

void WebKitBrowserExtension::slotFrameInTop()
{
    KParts::OpenUrlArguments args;// = d->m_khtml->arguments();
    args.metaData()["referrer"] = part->view()->contextMenuResult().linkText();
    KParts::BrowserArguments browserArgs;//( d->m_khtml->browserExtension()->browserArguments() );
    browserArgs.frameName = "_top";
    emit openUrlRequest(part->view()->contextMenuResult().linkUrl(), args, browserArgs);
}

void WebKitBrowserExtension::slotSaveImageAs()
{
    QList<KUrl> urls;
    urls.append(part->view()->contextMenuResult().imageUrl());
    for (int i = 0; i != urls.count(); i++) {
        QString file = KFileDialog::getSaveFileName(KUrl(), QString(), part->widget());
        KIO::NetAccess::file_copy(urls.at(i), file, part->widget());
    }
}

void WebKitBrowserExtension::slotSendImage()
{
    QStringList urls;
    urls.append(part->view()->contextMenuResult().imageUrl().path());
    QString subject = part->view()->contextMenuResult().imageUrl().path();
    KToolInvocation::invokeMailer(QString(), QString(), QString(), subject,
                                  QString(), //body
                                  QString(),
                                  urls); // attachments
}

void WebKitBrowserExtension::slotCopyImage()
{
    KUrl safeURL(part->view()->contextMenuResult().imageUrl());
    safeURL.setPass(QString());

    // Set it in both the mouse selection and in the clipboard
    QMimeData* mimeData = new QMimeData;
    mimeData->setImageData(part->view()->contextMenuResult().pixmap());
    safeURL.populateMimeData(mimeData);
    QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);

    mimeData = new QMimeData;
    mimeData->setImageData(part->view()->contextMenuResult().pixmap());
    safeURL.populateMimeData(mimeData);
    QApplication::clipboard()->setMimeData(mimeData, QClipboard::Selection);
}

void WebKitBrowserExtension::slotViewDocumentSource()
{
    //TODO test http requests
    KUrl currentUrl(part->view()->page()->mainFrame()->url());
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

    KRun::runUrl(currentUrl, QLatin1String("text/plain"), part->view(), isTempFile);
}

#include "webkitpart.moc"
