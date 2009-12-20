/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
 * Copyright (C) 2008 - 2009 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
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

#include "kwebkitpart.h"

#include "kwebkitpart_ext.h"
#include "webview.h"
#include "webpage.h"
#include "websslinfo.h"
#include "sslinfodialog_p.h"

#include "ui/searchbar.h"
#include "ui/passwordbar.h"
#include "settings/webkitsettings.h"

#include <KDE/KParts/GenericFactory>
#include <KDE/KParts/Plugin>
#include <KDE/KAboutData>
#include <KDE/KConfigGroup>
#include <KDE/KAction>
#include <KDE/KActionCollection>
#include <KDE/KMessageBox>
#include <KDE/KStandardDirs>
#include <KDE/KIconLoader>
#include <KDE/KGlobal>
#include <KDE/KStringHandler>
#include <kio/global.h>
#include <kdewebkit/kwebwallet.h>

#include <QtCore/QUrl>
#include <QtCore/QFile>
#include <QtCore/QPair>
#include <QtCore/QHash>
#include <QtGui/QApplication>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPrintPreviewDialog>
#include <QtWebKit/QWebFrame>
#include <QWebHistory>


#define QL1S(x)  QLatin1String(x)
#define QL1C(x)  QLatin1Char(x)

static QString htmlError (int code, const QString& text, const KUrl& reqUrl)
{
  QString errorName, techName, description;
  QStringList causes, solutions;

  QByteArray raw = KIO::rawErrorDetail( code, text, &reqUrl );
  QDataStream stream(raw);

  stream >> errorName >> techName >> description >> causes >> solutions;

  QString url, protocol, datetime;
  url = reqUrl.url();
  protocol = reqUrl.protocol();
  datetime = KGlobal::locale()->formatDateTime( QDateTime::currentDateTime(),
                                                KLocale::LongDate );

  QString filename( KStandardDirs::locate( "data", "kwebkitpart/error.html" ) );
  QFile file( filename );
  bool isOpened = file.open( QIODevice::ReadOnly );
  if ( !isOpened )
    kWarning() << "Could not open error html template:" << filename;

  QString html = QString( QL1S( file.readAll() ) );

  html.replace( QL1S( "TITLE" ), i18n( "Error: %1", errorName ) );
  html.replace( QL1S( "DIRECTION" ), QApplication::isRightToLeft() ? "rtl" : "ltr" );
  html.replace( QL1S( "ICON_PATH" ), KUrl(KIconLoader::global()->iconPath("dialog-warning", -KIconLoader::SizeHuge)).url() );

  QString doc = QL1S( "<h1>" );
  doc += i18n( "The requested operation could not be completed" );
  doc += QL1S( "</h1><h2>" );
  doc += errorName;
  doc += QL1S( "</h2>" );

  if ( !techName.isNull() ) {
    doc += QL1S( "<h2>" );
    doc += i18n( "Technical Reason: " );
    doc += techName;
    doc += QL1S( "</h2>" );
  }

  doc += QL1S( "<h3>" );
  doc += i18n( "Details of the Request:" );
  doc += QL1S( "</h3><ul><li>" );
  doc += i18n( "URL: %1" ,  url );
  doc += QL1S( "</li><li>" );

  if ( !protocol.isNull() ) {
    doc += i18n( "Protocol: %1", protocol );
    doc += QL1S( "</li><li>" );
  }

  doc += i18n( "Date and Time: %1" ,  datetime );
  doc += QL1S( "</li><li>" );
  doc += i18n( "Additional Information: %1" ,  text );
  doc += QL1S( "</li></ul><h3>" );
  doc += i18n( "Description:" );
  doc += QL1S( "</h3><p>" );
  doc += description;
  doc += QL1S( "</p>" );

  if ( causes.count() ) {
    doc += QL1S( "<h3>" );
    doc += i18n( "Possible Causes:" );
    doc += QL1S( "</h3><ul><li>" );
    doc += causes.join( "</li><li>" );
    doc += QL1S( "</li></ul>" );
  }

  if ( solutions.count() ) {
    doc += QL1S( "<h3>" );
    doc += i18n( "Possible Solutions:" );
    doc += QL1S( "</h3><ul><li>" );
    doc += solutions.join( "</li><li>" );
    doc += QL1S( "</li></ul>" );
  }

  html.replace( QL1S("TEXT"), doc );

  return html;
}

class KWebKitPart::KWebKitPartPrivate
{
public:
  enum PageSecurity { Unencrypted, Encrypted, Mixed };
  KWebKitPartPrivate() : updateHistory(true) {}

  bool updateHistory;

  QPointer<WebView> webView;
  QPointer<WebPage> webPage;
  QPointer<KDEPrivate::SearchBar> searchBar;
  WebKitBrowserExtension *browserExtension;
};

KWebKitPart::KWebKitPart(QWidget *parentWidget, QObject *parent, const QStringList &/*args*/)
            :KParts::ReadOnlyPart(parent), d(new KWebKitPart::KWebKitPartPrivate())
{
    KAboutData about = KAboutData("kwebkitpart", "webkitkde", ki18n("WebKit HTML Component"),
                                  /*version*/ "0.9", /*ki18n("shortDescription")*/ KLocalizedString(),
                                  KAboutData::License_LGPL,
                                  ki18n("(c) 2009 Dawit Alemayehu\n"
                                        "(c) 2008-2009 Urs Wolfer\n"
                                        "(c) 2007 Trolltech ASA"));

    about.addAuthor(ki18n("Laurent Montel"), KLocalizedString(), "montel@kde.org");
    about.addAuthor(ki18n("Michael Howell"), KLocalizedString(), "mhowell123@gmail.com");
    about.addAuthor(ki18n("Urs Wolfer"), KLocalizedString(), "uwolfer@kde.org");
    about.addAuthor(ki18n("Dirk Mueller"), KLocalizedString(), "mueller@kde.org");
    about.addAuthor(ki18n("Dawit Alemayehu"), KLocalizedString(), "adawit@kde.org");
    KComponentData componentData(&about);
    setComponentData(componentData);

    // NOTE: If the application does not set its version number, we automatically
    // set it to KDE's version number so that the default user-agent string contains
    // proper application version number information. See QWebPage::userAgentForUrl...
    if (QCoreApplication::applicationVersion().isEmpty())
        QCoreApplication::setApplicationVersion(QString("%1.%2.%3")
                                                .arg(KDE::versionMajor())
                                                .arg(KDE::versionMinor())
                                                .arg(KDE::versionRelease()));

    QWidget *mainWidget = new QWidget (parentWidget);
    mainWidget->setObjectName("kwebkitpart");
    setWidget(mainWidget);

    // Create the WebView...
    d->webView = new WebView (this, mainWidget);
    connect(d->webView, SIGNAL(titleChanged(const QString &)),
            this, SIGNAL(setWindowCaption(const QString &)));
    connect(d->webView, SIGNAL(loadFinished(bool)),
            this, SLOT(slotLoadFinished(bool)));
    connect(d->webView, SIGNAL(urlChanged(const QUrl &)),
            this, SLOT(slotUrlChanged(const QUrl &)));
    connect(d->webView, SIGNAL(linkMiddleOrCtrlClicked(const KUrl &)),
            this, SLOT(slotLinkMiddleOrCtrlClicked(const KUrl &)));

    // Create the search bar...
    d->searchBar = new KDEPrivate::SearchBar;
    connect(d->searchBar, SIGNAL(searchTextChanged(const QString &, bool)),
            this, SLOT(slotSearchForText(const QString &, bool)));

    d->webPage = qobject_cast<WebPage*>(d->webView->page());
    Q_ASSERT(d->webPage);

    connect(d->webPage, SIGNAL(loadStarted()),
            this, SLOT(slotLoadStarted()));
    connect(d->webPage, SIGNAL(loadAborted(const KUrl &)),
            this, SLOT(slotLoadAborted(const KUrl &)));
    connect(d->webPage, SIGNAL(navigationRequestFinished(const KUrl &, QWebFrame *)),
            this, SLOT(slotNavigationRequestFinished(const KUrl &, QWebFrame *)));
    connect(d->webPage, SIGNAL(linkHovered(const QString &, const QString &, const QString &)),
            this, SLOT(slotLinkHovered(const QString &, const QString &, const QString &)));
    connect(d->webPage, SIGNAL(saveFrameStateRequested(QWebFrame *, QWebHistoryItem *)),
            this, SLOT(slotSaveFrameState(QWebFrame *, QWebHistoryItem *)));
    connect(d->webPage, SIGNAL(jsStatusBarMessage(const QString &)),
            this, SIGNAL(setStatusBarText(const QString &)));
    connect(d->webView, SIGNAL(linkShiftClicked(const KUrl &)),
            d->webPage, SLOT(downloadUrl(const KUrl &)));

    d->browserExtension = new WebKitBrowserExtension(this);
    connect(d->webPage, SIGNAL(loadProgress(int)),
            d->browserExtension, SIGNAL(loadingProgress(int)));
    connect(d->webPage, SIGNAL(selectionChanged()),
            d->browserExtension, SLOT(updateEditActions()));
    connect(d->browserExtension, SIGNAL(saveUrl(const KUrl&)),
            d->webPage, SLOT(downloadUrl(const KUrl &)));

    connect(d->webView, SIGNAL(selectionClipboardUrlPasted(const KUrl &)),
            d->browserExtension, SIGNAL(openUrlRequest(const KUrl &)));

    KDEPrivate::PasswordBar *passwordBar = new KDEPrivate::PasswordBar(mainWidget);

    // Create the password bar...
    if (d->webPage->wallet()) {
        connect (d->webPage->wallet(), SIGNAL(saveFormDataRequested(const QString &, const QUrl &)),
                 passwordBar, SLOT(onSaveFormData(const QString &, const QUrl &)));
        connect(passwordBar, SIGNAL(saveFormDataAccepted(const QString &)),
                d->webPage->wallet(), SLOT(acceptSaveFormDataRequest(const QString &)));
        connect(passwordBar, SIGNAL(saveFormDataRejected(const QString &)),
                d->webPage->wallet(), SLOT(rejectSaveFormDataRequest(const QString &)));
    }

    QVBoxLayout* lay = new QVBoxLayout(mainWidget);
    lay->setMargin(0);
    lay->setSpacing(0);
    lay->addWidget(passwordBar);
    lay->addWidget(d->webView);
    lay->addWidget(d->searchBar);

    setXMLFile("kwebkitpart.rc");
    initAction();
    mainWidget->setFocusProxy(d->webView);
}

KWebKitPart::~KWebKitPart()
{
    delete d;
}

bool KWebKitPart::openUrl(const KUrl &u)
{
    kDebug() << u;

    // Ignore empty requests...
    if (u.isEmpty())
        return false;

    // Do not update history when url is typed in since konqueror
    // automatically does that itself.
    d->updateHistory = false;

    // Handle error conditions...
    if (handleError(u, d->webView->page()->mainFrame()))
        return true;

    // Set the url...
    setUrl(u);

    if (u.url() == "about:blank") {
        emit setWindowCaption (u.url());
        d->webView->setUrl(u);
    } else {
        KParts::BrowserArguments bargs (browserExtension()->browserArguments());
        KParts::OpenUrlArguments args (arguments());
        KIO::MetaData metaData (args.metaData());

        // Get the SSL information sent, if any...
        if (metaData.contains(QL1S("ssl_in_use"))) {
            WebSslInfo sslinfo;
            sslinfo.fromMetaData(metaData.toVariant());
            sslinfo.setUrl(u);
            d->webPage->setSslInfo(sslinfo);
        }

        // Check if this is a restore state request, i.e. a history navigation
        // or session restore request. If it is, set the state information so
        // that the page can be properly restored...
        if (metaData.contains(QL1S("webkitpart-restore-state"))) {
            WebFrameState frameState;
            frameState.url = u;
            frameState.scrollPosX = args.xOffset();
            frameState.scrollPosY = args.yOffset();
            d->webPage->saveFrameState(QString(), frameState);

            const int count = bargs.docState.count();
            for (int i = 0; i < count; i += 4) {
                frameState.url = bargs.docState.at(i+1);
                frameState.scrollPosX = bargs.docState.at(i+2).toInt();
                frameState.scrollPosY = bargs.docState.at(i+3).toInt();
                d->webPage->saveFrameState(bargs.docState.at(i), frameState);
            }
        }

        d->webView->loadUrl(u, args, bargs);
    }

    return true;
}

bool KWebKitPart::closeUrl()
{
    d->webView->stop();
    return true;
}

QWebView * KWebKitPart::view()
{
    return d->webView;
}

void KWebKitPart::guiActivateEvent(KParts::GUIActivateEvent *event)
{
    Q_UNUSED(event);
    // just overwrite, but do nothing for the moment
}

bool KWebKitPart::openFile()
{
    // never reached
    return false;
}

void KWebKitPart::initAction()
{
    KAction *action = actionCollection()->addAction(KStandardAction::SaveAs, "saveDocument",
                                                    d->browserExtension, SLOT(slotSaveDocument()));

    action = new KAction(i18n("Save &Frame As..."), this);
    actionCollection()->addAction("saveFrame", action);
    connect(action, SIGNAL(triggered(bool)), d->browserExtension, SLOT(slotSaveFrame()));

    action = new KAction(KIcon("document-print-frame"), i18n("Print Frame..."), this);
    actionCollection()->addAction("printFrame", action);
    connect(action, SIGNAL(triggered(bool)), d->browserExtension, SLOT(printFrame()));

    action = new KAction(KIcon("zoom-in"), i18n("Zoom In"), this);
    actionCollection()->addAction("zoomIn", action);
    action->setShortcut(KShortcut("CTRL++; CTRL+="));
    connect(action, SIGNAL(triggered(bool)), d->browserExtension, SLOT(zoomIn()));

    action = new KAction(KIcon("zoom-out"), i18n("Zoom Out"), this);
    actionCollection()->addAction("zoomOut", action);
    action->setShortcut(KShortcut("CTRL+-; CTRL+_"));
    connect(action, SIGNAL(triggered(bool)), d->browserExtension, SLOT(zoomOut()));

    action = new KAction(KIcon("zoom-original"), i18n("Actual Size"), this);
    actionCollection()->addAction("zoomNormal", action);
    action->setShortcut(KShortcut("CTRL+0"));
    connect(action, SIGNAL(triggered(bool)), d->browserExtension, SLOT(zoomNormal()));

#if QT_VERSION >= 0x040500
    action = new KAction(i18n("Zoom Text Only"), this);
    action->setCheckable(true);
    KConfigGroup cgHtml(KGlobal::config(), "HTML Settings");
    bool zoomTextOnly = cgHtml.readEntry("ZoomTextOnly", false);
    action->setChecked(zoomTextOnly);
    actionCollection()->addAction("zoomTextOnly", action);
    connect(action, SIGNAL(triggered(bool)), d->browserExtension, SLOT(toogleZoomTextOnly()));
#endif

    action = actionCollection()->addAction(KStandardAction::SelectAll, "selectAll",
                                           d->browserExtension, SLOT(slotSelectAll()));
    action->setShortcutContext(Qt::WidgetShortcut);
    d->webView->addAction(action);

    action = new KAction(i18n("View Do&cument Source"), this);
    actionCollection()->addAction("viewDocumentSource", action);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_U));
    connect(action, SIGNAL(triggered(bool)), d->browserExtension, SLOT(slotViewDocumentSource()));

    action = new KAction(i18n("SSL"), this);
    actionCollection()->addAction("security", action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(slotShowSecurity()));

    action = actionCollection()->addAction(KStandardAction::Find, "find", this, SLOT(slotShowSearchBar()));
    action->setWhatsThis(i18n("Find text<br /><br />"
                              "Shows a dialog that allows you to find text on the displayed page."));

    action = actionCollection()->addAction(KStandardAction::FindNext, "findnext",
                                           d->searchBar, SLOT(findNext()));
    action = actionCollection()->addAction(KStandardAction::FindPrev, "findprev",
                                           d->searchBar, SLOT(findPrevious()));
}

bool KWebKitPart::handleError(const KUrl &u, QWebFrame *frame)
{
    if ( u.protocol() == "error" && u.hasSubUrl() ) {
        /**
         * The format of the error url is that two variables are passed in the query:
         * error = int kio error code, errText = QString error text from kio
         * and the URL where the error happened is passed as a sub URL.
         */
        KUrl::List urls = KUrl::split(u);

        if ( urls.count() > 1 ) {
            KUrl mainURL = urls.first();
            int error = mainURL.queryItem( "error" ).toInt();

            // error=0 isn't a valid error code, so 0 means it's missing from the URL
            if ( error == 0 )
                error = KIO::ERR_UNKNOWN;

            if (error == KIO::ERR_USER_CANCELED) {
                setUrl(d->webView->url());
                emit d->browserExtension->setLocationBarUrl(KUrl(d->webView->url()).prettyUrl());
            } else {
                const QString errorText = mainURL.queryItem( "errText" );
                urls.pop_front();
                KUrl reqUrl = KUrl::join( urls );

                const QString html = htmlError(error, errorText, reqUrl);
                if (frame->parentFrame()) {                    
                    frame->setHtml(html, reqUrl);
                } else {
                    slotUrlChanged(reqUrl);
                    frame->setHtml(html);
                }
            }
            return true;
        }
    }

    return false;
}

/*************** PRIVATE SLOTS ********************************/

void KWebKitPart::slotLoadStarted()
{
    emit started(0);
}

void KWebKitPart::slotLoadFinished(bool ok)
{
    d->updateHistory = true;

    if (ok) {
        // Restore page state as necessary...
        d->webPage->restoreAllFrameState();

        if (d->webView->title().trimmed().isEmpty()) {
            // If the document title is empty, then set it to the current url
            // squeezed at the center...
            const QString caption = d->webView->url().toString((QUrl::RemoveQuery|QUrl::RemoveFragment));
            emit setWindowCaption(KStringHandler::csqueeze(caption));

            // The urlChanged signal is emitted if and only if the main frame
            // receives the title of the page so we manually invoke the slot as
            // a work around here for pages that do not contain it, such as
            // text documents...
            slotUrlChanged(d->webView->url());
        }

        if (WebKitSettings::self()->isFormCompletionEnabled() && d->webPage->wallet()) {
            d->webPage->wallet()->fillFormData(d->webPage->mainFrame());
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
    QMapIterator<QString,QString> it (d->webView->page()->mainFrame()->metaData());
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
    emit completed();
#endif
}

void KWebKitPart::slotLoadAborted(const KUrl & url)
{
    closeUrl();
    if (url.isValid())
      emit d->browserExtension->openUrlRequest(url);
    else
      setUrl(d->webView->url());
}

void  KWebKitPart::slotNavigationRequestFinished(const KUrl& url, QWebFrame *frame)
{
    if (frame) {

        if (handleError(url, frame)) {
            return;
        }

        if (frame == d->webPage->mainFrame()) {
            if (d->webPage->sslInfo().isValid())
                d->browserExtension->setPageSecurity(KWebKitPart::KWebKitPartPrivate::Encrypted);
            else
                d->browserExtension->setPageSecurity(KWebKitPart::KWebKitPartPrivate::Unencrypted);
        }
    }
}

void KWebKitPart::slotUrlChanged(const QUrl& url)
{
    if ( this->url() != url) {
        setUrl(url);
        emit d->browserExtension->setLocationBarUrl(KUrl(url).prettyUrl());
    }
}

void KWebKitPart::slotShowSecurity()
{
    if (d->webPage->sslInfo().isValid()) {
        KSslInfoDialog *dlg = new KSslInfoDialog;
        dlg->setSslInfo(d->webPage->sslInfo().certificateChain(),
                        d->webPage->sslInfo().peerAddress().toString(),
                        url().host(),
                        d->webPage->sslInfo().protocol(),
                        d->webPage->sslInfo().ciphers(),
                        d->webPage->sslInfo().usedChiperBits(),
                        d->webPage->sslInfo().supportedChiperBits(),
                        KSslInfoDialog::errorsFromString(d->webPage->sslInfo().certificateErrors()));

        dlg->exec();
    } else {
        KMessageBox::information(0, i18n("The SSL information for this site "
                                         "appears to be corrupt."), i18n("SSL"));
    }
}

void KWebKitPart::slotSaveFrameState(QWebFrame *frame, QWebHistoryItem *item)
{
    Q_UNUSED (item);
    if (!frame->parentFrame() && d->updateHistory) {
        emit d->browserExtension->openUrlNotify();
    }
}

void KWebKitPart::slotLinkHovered(const QString &link, const QString &title, const QString &content)
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

    emit setStatusBarText(message);
}

void KWebKitPart::slotSearchForText(const QString &text, bool backward)
{
    QWebPage::FindFlags flags;

    if (backward)
        flags = QWebPage::FindBackward;

    if (d->searchBar->caseSensitive())
        flags |= QWebPage::FindCaseSensitively;

    d->searchBar->setFoundMatch(d->webView->page()->findText(text, flags));
}

void KWebKitPart::slotShowSearchBar()
{
    const QString text = d->webView->selectedText();

    if (text.isEmpty())
        d->webView->pageAction(QWebPage::Undo);
    else
        d->searchBar->setSearchText(text.left(150));

    d->searchBar->show();
}

void KWebKitPart::slotLinkMiddleOrCtrlClicked(const KUrl& linkUrl)
{
    KParts::OpenUrlArguments args;
    args.setActionRequestedByUser(true);
    args.metaData()["referrer"] = url().url();

    emit browserExtension()->createNewWindow(linkUrl, args);
}

#include "kwebkitpart.moc"
