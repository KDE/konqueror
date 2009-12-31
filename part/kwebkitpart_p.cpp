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


#include "kwebkitpart_p.h"

#include "webview.h"
#include "webpage.h"
#include "websslinfo.h"
#include "sslinfodialog_p.h"
#include "kwebkitpart.h"
#include "kwebkitpart_ext.h"

#include "settings/webkitsettings.h"
#include "ui/passwordbar.h"
#include "ui/searchbar.h"
#include <kdewebkit/kwebwallet.h>

#include <KDE/KLocalizedString>
#include <KDE/KStringHandler>
#include <KDE/KMessageBox>
#include <KDE/KDebug>

#include <QtGui/QVBoxLayout>
#include <QtWebKit/QWebFrame>

#define QL1S(x) QLatin1String(x)
#define QL1C(x) QLatin1Char(x)

KWebKitPartPrivate::KWebKitPartPrivate(KWebKitPart *parent)
                   :QObject(),
                    updateHistory(true),
                    q(parent)
{
}

void KWebKitPartPrivate::init(QWidget *mainWidget)
{
    // Create the WebView...
    webView = new WebView (q, mainWidget);
    connect(webView, SIGNAL(titleChanged(const QString &)),
            q, SIGNAL(setWindowCaption(const QString &)));
    connect(webView, SIGNAL(loadFinished(bool)),
            this, SLOT(slotLoadFinished(bool)));
    connect(webView, SIGNAL(urlChanged(const QUrl &)),
            this, SLOT(slotUrlChanged(const QUrl &)));
    connect(webView, SIGNAL(linkMiddleOrCtrlClicked(const KUrl &)),
            this, SLOT(slotLinkMiddleOrCtrlClicked(const KUrl &)));

    // Create the search bar...
    searchBar = new KDEPrivate::SearchBar;
    connect(searchBar, SIGNAL(searchTextChanged(const QString &, bool)),
            this, SLOT(slotSearchForText(const QString &, bool)));

    webPage = qobject_cast<WebPage*>(webView->page());
    Q_ASSERT(webPage);

    connect(webPage, SIGNAL(loadStarted()),
            this, SLOT(slotLoadStarted()));
    connect(webPage, SIGNAL(loadAborted(const KUrl &)),
            this, SLOT(slotLoadAborted(const KUrl &)));
    connect(webPage, SIGNAL(navigationRequestFinished(const KUrl &, QWebFrame *)),
            this, SLOT(slotNavigationRequestFinished(const KUrl &, QWebFrame *)));
    connect(webPage, SIGNAL(linkHovered(const QString &, const QString &, const QString &)),
            this, SLOT(slotLinkHovered(const QString &, const QString &, const QString &)));
    connect(webPage, SIGNAL(saveFrameStateRequested(QWebFrame *, QWebHistoryItem *)),
            this, SLOT(slotSaveFrameState(QWebFrame *, QWebHistoryItem *)));
    connect(webPage, SIGNAL(jsStatusBarMessage(const QString &)),
            q, SIGNAL(setStatusBarText(const QString &)));
    connect(webView, SIGNAL(linkShiftClicked(const KUrl &)),
            webPage, SLOT(downloadUrl(const KUrl &)));

    browserExtension = new WebKitBrowserExtension(q);
    connect(webPage, SIGNAL(loadProgress(int)),
            browserExtension, SIGNAL(loadingProgress(int)));
    connect(webPage, SIGNAL(selectionChanged()),
            browserExtension, SLOT(updateEditActions()));
    connect(browserExtension, SIGNAL(saveUrl(const KUrl&)),
            webPage, SLOT(downloadUrl(const KUrl &)));

    connect(webView, SIGNAL(selectionClipboardUrlPasted(const KUrl &)),
            browserExtension, SIGNAL(openUrlRequest(const KUrl &)));

    KDEPrivate::PasswordBar *passwordBar = new KDEPrivate::PasswordBar(mainWidget);

    // Create the password bar...
    if (webPage->wallet()) {
        connect (webPage->wallet(), SIGNAL(saveFormDataRequested(const QString &, const QUrl &)),
                 passwordBar, SLOT(onSaveFormData(const QString &, const QUrl &)));
        connect(passwordBar, SIGNAL(saveFormDataAccepted(const QString &)),
                webPage->wallet(), SLOT(acceptSaveFormDataRequest(const QString &)));
        connect(passwordBar, SIGNAL(saveFormDataRejected(const QString &)),
                webPage->wallet(), SLOT(rejectSaveFormDataRequest(const QString &)));
    }

    QVBoxLayout* lay = new QVBoxLayout(mainWidget);
    lay->setMargin(0);
    lay->setSpacing(0);
    lay->addWidget(passwordBar);
    lay->addWidget(webView);
    lay->addWidget(searchBar);

    mainWidget->setFocusProxy(webView);
}

void KWebKitPartPrivate::slotLoadStarted()
{
    emit q->started(0);
}

void KWebKitPartPrivate::slotLoadFinished(bool ok)
{
    updateHistory = true;

    if (ok) {
        // Restore page state as necessary...
        webPage->restoreAllFrameState();

        if (webView->title().trimmed().isEmpty()) {
            // If the document title is empty, then set it to the current url
            // squeezed at the center...
            const QString caption = webView->url().toString((QUrl::RemoveQuery|QUrl::RemoveFragment));
            emit q->setWindowCaption(KStringHandler::csqueeze(caption));

            // The urlChanged signal is emitted if and only if the main frame
            // receives the title of the page so we manually invoke the slot as
            // a work around here for pages that do not contain it, such as
            // text documents...
            slotUrlChanged(webView->url());
        }

        if (WebKitSettings::self()->isFormCompletionEnabled() && webPage->wallet()) {
            webPage->wallet()->fillFormData(webPage->mainFrame());
        }
    }

    /*
      NOTE #1: QtWebKit will not kill a META redirect request even if one
      triggers the WebPage::Stop action!! As such the code below is useless
      and disabled for now.

      NOTE #2: QWebFrame::metaData only provides access to META tags that
      contain a 'name' attribute and completely ignores those that do not.
      This of course includes yes the meta redirect tag, i.e. the 'http-equiv'
      attribute. Hence the convoluted code below just to check if we have a
      redirect request!
    */
#if 0
    bool refresh = false;
    QMapIterator<QString,QString> it (webView->page()->mainFrame()->metaData());
    while (it.hasNext()) {
      it.next();
      //kDebug() << "meta-key: " << it.key() << "meta-value: " << it.value();
      // HACK: QtWebKit does not parse the value of http-equiv property and
      // as such uses an empty key with a value when
      if (it.key().isEmpty() && it.value().contains(QRegExp("[0-9];url"))) {
        refresh = true;
        break;
      }
    }
    emit completed(refresh);
#else
    emit q->completed();
#endif
}

void KWebKitPartPrivate::slotLoadAborted(const KUrl & url)
{
    q->closeUrl();
    if (url.isValid())
      emit browserExtension->openUrlRequest(url);
    else
      q->setUrl(webView->url());
}

void  KWebKitPartPrivate::slotNavigationRequestFinished(const KUrl& url, QWebFrame *frame)
{
    if (frame) {

        if (q->handleError(url, frame)) {
            return;
        }

        if (frame == webPage->mainFrame()) {
            if (webPage->sslInfo().isValid())
                browserExtension->setPageSecurity(KWebKitPartPrivate::KWebKitPartPrivate::Encrypted);
            else
                browserExtension->setPageSecurity(KWebKitPartPrivate::KWebKitPartPrivate::Unencrypted);
        }
    }
}

void KWebKitPartPrivate::slotUrlChanged(const QUrl& url)
{
    kDebug() << url << q->url();
    if (url.toString() != QL1S("about:blank")) {
        q->setUrl(url);
        emit browserExtension->setLocationBarUrl(KUrl(url).prettyUrl());
    }
}

void KWebKitPartPrivate::slotShowSecurity()
{
    if (webPage->sslInfo().isValid()) {
        KSslInfoDialog *dlg = new KSslInfoDialog;
        dlg->setSslInfo(webPage->sslInfo().certificateChain(),
                        webPage->sslInfo().peerAddress().toString(),
                        q->url().host(),
                        webPage->sslInfo().protocol(),
                        webPage->sslInfo().ciphers(),
                        webPage->sslInfo().usedChiperBits(),
                        webPage->sslInfo().supportedChiperBits(),
                        KSslInfoDialog::errorsFromString(webPage->sslInfo().certificateErrors()));

        dlg->exec();
    } else {
        KMessageBox::information(0, i18n("The SSL information for this site "
                                         "appears to be corrupt."), i18n("SSL"));
    }
}

void KWebKitPartPrivate::slotSaveFrameState(QWebFrame *frame, QWebHistoryItem *item)
{
    Q_UNUSED (item);
    if (!frame->parentFrame() && updateHistory) {
        emit browserExtension->openUrlNotify();
    }
}

void KWebKitPartPrivate::slotLinkHovered(const QString &link, const QString &title, const QString &content)
{
    Q_UNUSED(title);
    Q_UNUSED(content);

    QString message;
    QUrl linkUrl (link);
    const QString scheme = linkUrl.scheme();

    if (QString::compare(scheme, QL1S("mailto"), Qt::CaseInsensitive) == 0) {
        message += i18n("Email: ");

        // Workaround: for QUrl's parsing deficiencies of "mailto:foo@bar.com".
        if (!linkUrl.hasQuery())
          linkUrl = QUrl(scheme + '?' + linkUrl.path());

        QMap<QString, QStringList> fields;
        QPair<QString, QString> queryItem;

        Q_FOREACH (queryItem, linkUrl.queryItems()) {
            //kDebug() << "query: " << queryItem.first << queryItem.second;
            if (queryItem.first.contains(QL1C('@')) && queryItem.second.isEmpty())
                fields["to"] << queryItem.first;
            if (QString::compare(queryItem.first, QL1S("to"), Qt::CaseInsensitive) == 0)
                fields["to"] << queryItem.second;
            if (QString::compare(queryItem.first, QL1S("cc"), Qt::CaseInsensitive) == 0)
                fields["cc"] << queryItem.second;
            if (QString::compare(queryItem.first, QL1S("bcc"), Qt::CaseInsensitive) == 0)
                fields["bcc"] << queryItem.second;
            if (QString::compare(queryItem.first, QL1S("subject"), Qt::CaseInsensitive) == 0)
                fields["subject"] << queryItem.second;
        }

        if (fields.contains(QL1S("to")))
            message += fields.value(QL1S("to")).join(QL1S(", "));
        if (fields.contains(QL1S("cc")))
            message += QL1S(" - CC: ") + fields.value(QL1S("cc")).join(QL1S(", "));
        if (fields.contains(QL1S("bcc")))
            message += QL1S(" - BCC: ") + fields.value(QL1S("bcc")).join(QL1S(", "));
        if (fields.contains(QL1S("subject")))
            message += QL1S(" - Subject: ") + fields.value(QL1S("subject")).join(QL1S(" "));
    } else {
        message = link;
    }

    emit q->setStatusBarText(message);
}

void KWebKitPartPrivate::slotSearchForText(const QString &text, bool backward)
{
    QWebPage::FindFlags flags;

    if (backward)
        flags = QWebPage::FindBackward;

    if (searchBar->caseSensitive())
        flags |= QWebPage::FindCaseSensitively;

    searchBar->setFoundMatch(webView->page()->findText(text, flags));
}

void KWebKitPartPrivate::slotShowSearchBar()
{
    const QString text = webView->selectedText();

    if (text.isEmpty())
        webView->pageAction(QWebPage::Undo);
    else
        searchBar->setSearchText(text.left(150));

    searchBar->show();
}

void KWebKitPartPrivate::slotLinkMiddleOrCtrlClicked(const KUrl& linkUrl)
{
    KParts::OpenUrlArguments args;
    args.setActionRequestedByUser(true);
    args.metaData()["referrer"] = q->url().url();

    emit browserExtension->createNewWindow(linkUrl, args);
}

#include "kwebkitpart_p.moc"
