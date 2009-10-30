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

#include "webkitpart.h"
#include "webkitpart_ext.h"

#include "webview.h"
#include "webpage.h"
#include "websslinfo.h"

#include "sslinfodialog_p.h"

#include <ui/searchbar.h>

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

#include <QtCore/QUrl>
#include <QtCore/QFile>
#include <QtCore/QPair>
#include <QtGui/QApplication>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPrintPreviewDialog>
#include <QtWebKit/QWebFrame>
#include <QWebHistory>


#define QL1(x)    QLatin1String(x)


static QString htmlError (int code, const QString& text, const KUrl& reqUrl)
{
  // kDebug() << "errorCode" << code << "text" << text;

  QString errorName, techName, description;
  QStringList causes, solutions;

  QByteArray raw = KIO::rawErrorDetail( code, text, &reqUrl );
  QDataStream stream(raw);

  stream >> errorName >> techName >> description >> causes >> solutions;

  QString url, protocol, datetime;
  url = Qt::escape( reqUrl.prettyUrl() );
  protocol = reqUrl.protocol();
  datetime = KGlobal::locale()->formatDateTime( QDateTime::currentDateTime(),
                                                KLocale::LongDate );

  QString filename( KStandardDirs::locate( "data", "khtml/error.html" ) );
  QFile file( filename );
  bool isOpened = file.open( QIODevice::ReadOnly );
  if ( !isOpened )
    kWarning() << "Could not open error html template:" << filename;

  QString html = QString( QL1( file.readAll() ) );

  html.replace( QL1( "TITLE" ), i18n( "Error: %1 - %2", errorName, url ) );
  html.replace( QL1( "DIRECTION" ), QApplication::isRightToLeft() ? "rtl" : "ltr" );
  html.replace( QL1( "ICON_PATH" ), KIconLoader::global()->iconPath( "dialog-warning", -KIconLoader::SizeHuge ) );

  QString doc = QL1( "<h1>" );
  doc += i18n( "The requested operation could not be completed" );
  doc += QL1( "</h1><h2>" );
  doc += errorName;
  doc += QL1( "</h2>" );

  if ( !techName.isNull() ) {
    doc += QL1( "<h2>" );
    doc += i18n( "Technical Reason: " );
    doc += techName;
    doc += QL1( "</h2>" );
  }

  doc += QL1( "<h3>" );
  doc += i18n( "Details of the Request:" );
  doc += QL1( "</h3><ul><li>" );
  doc += i18n( "URL: %1" ,  url );
  doc += QL1( "</li><li>" );

  if ( !protocol.isNull() ) {
    doc += i18n( "Protocol: %1", protocol );
    doc += QL1( "</li><li>" );
  }

  doc += i18n( "Date and Time: %1" ,  datetime );
  doc += QL1( "</li><li>" );
  doc += i18n( "Additional Information: %1" ,  text );
  doc += QL1( "</li></ul><h3>" );
  doc += i18n( "Description:" );
  doc += QL1( "</h3><p>" );
  doc += description;
  doc += QL1( "</p>" );

  if ( causes.count() ) {
    doc += QL1( "<h3>" );
    doc += i18n( "Possible Causes:" );
    doc += QL1( "</h3><ul><li>" );
    doc += causes.join( "</li><li>" );
    doc += QL1( "</li></ul>" );
  }

  if ( solutions.count() ) {
    doc += QL1( "<h3>" );
    doc += i18n( "Possible Solutions:" );
    doc += QL1( "</h3><ul><li>" );
    doc += solutions.join( "</li><li>" );
    doc += QL1( "</li></ul>" );
  }

  html.replace( QL1("TEXT"), doc );

  return html;
}


class WebKitPart::WebKitPartPrivate
{
public:
  enum PageSecurity { Unencrypted, Encrypted, Mixed };
  WebKitPartPrivate() : updateHistory(true) {}

  bool updateHistory;

  QPointer<WebView> webView;
  QPointer<WebPage> webPage;
  QPointer<KDEPrivate::SearchBar> searchBar;
  WebKitBrowserExtension *browserExtension;
};

WebKitPart::WebKitPart(QWidget *parentWidget, QObject *parent, const QStringList &/*args*/)
           :KParts::ReadOnlyPart(parent), d(new WebKitPart::WebKitPartPrivate())
{
    KAboutData about = KAboutData("webkitpart", "webkitkde", ki18n("WebKit HTML Component"),
                                  /*version*/ "0.2", /*ki18n("shortDescription")*/ KLocalizedString(),
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
    mainWidget->setObjectName("webkitpart");
    setWidget(mainWidget);

    QVBoxLayout* lay = new QVBoxLayout(mainWidget);
    lay->setMargin(0);
    lay->setSpacing(0);

    // Add the WebView...
    d->webView = new WebView (this, mainWidget); 
    lay->addWidget(d->webView);
    connect(d->webView, SIGNAL(titleChanged(const QString &)),
            this, SIGNAL(setWindowCaption(const QString &)));
    connect(d->webView, SIGNAL(loadFinished(bool)),
            this, SLOT(loadFinished(bool)));
    connect(d->webView, SIGNAL(urlChanged(const QUrl &)),
            this, SLOT(urlChanged(const QUrl &)));

    // Add the search bar...
    d->searchBar = new KDEPrivate::SearchBar;
    lay->addWidget(d->searchBar);
    connect(d->searchBar, SIGNAL(searchTextChanged(const QString &, bool)),
            this, SLOT(searchForText(const QString &, bool)));

    d->webPage = qobject_cast<WebPage*>(d->webView->page());
    Q_ASSERT(d->webPage);

    connect(d->webPage, SIGNAL(loadStarted()),
            this, SLOT(loadStarted()));
    connect(d->webPage, SIGNAL(loadAborted(const QUrl &)),
            this, SLOT(loadAborted(const QUrl &)));
    connect(d->webPage, SIGNAL(loadError(int, const QString &, const QString &)),
            this, SLOT(loadError(int, const QString &, const QString &)));
    connect(d->webPage, SIGNAL(linkHovered(const QString &, const QString &, const QString &)),
            this, SLOT(linkHovered(const QString &, const QString&, const QString &)));
    connect(d->webPage, SIGNAL(updateHistory()), this, SLOT(updateHistory()));
    connect(d->webPage, SIGNAL(navigationRequestFinished()),
            this, SLOT(navigationRequestFinished()));

    d->browserExtension = new WebKitBrowserExtension(this);
    connect(d->webPage, SIGNAL(loadProgress(int)),
            d->browserExtension, SIGNAL(loadingProgress(int)));
    connect(d->webPage, SIGNAL(selectionChanged()),
            d->browserExtension, SLOT(updateEditActions()));
    connect(d->browserExtension, SIGNAL(saveUrl(const KUrl&)),
            d->webPage, SLOT(saveUrl(const KUrl &)));

    connect(d->webView, SIGNAL(openUrl(const KUrl &)),
            d->browserExtension, SIGNAL(openUrlRequest(const KUrl &)));
    connect(d->webView, SIGNAL(openUrlInNewWindow(const KUrl &)),
            d->browserExtension, SIGNAL(createNewWindow(const KUrl &)));

    setXMLFile("webkitpart.rc");
    initAction();
    mainWidget->setFocusProxy(d->webView);
}

WebKitPart::~WebKitPart()
{
    delete d;
}

void WebKitPart::initAction()
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
    connect(action, SIGNAL(triggered(bool)), this, SLOT(showSecurity()));

    action = actionCollection()->addAction(KStandardAction::Find, "find", this, SLOT(showSearchBar()));
    action->setWhatsThis(i18n("Find text<br /><br />"
                              "Shows a dialog that allows you to find text on the displayed page."));

    action = actionCollection()->addAction(KStandardAction::FindNext, "findnext",
                                           d->searchBar, SLOT(findNext()));
    action = actionCollection()->addAction(KStandardAction::FindPrev, "findprev",
                                           d->searchBar, SLOT(findPrevious()));
}

void WebKitPart::guiActivateEvent(KParts::GUIActivateEvent *event)
{
    Q_UNUSED(event);
    // just overwrite, but do nothing for the moment
}

bool WebKitPart::openUrl(const KUrl &url)
{
    kDebug() << url;

    // Ignore empty requests...
    if (url.isEmpty())
        return false;

    // Do not update history when url is typed in since konqueror
    // automatically does that itself.
    d->updateHistory = false;

    if ( url.protocol() == "error" && url.hasSubUrl() ) {
        closeUrl();

        /**
         * The format of the error url is that two variables are passed in the query:
         * error = int kio error code, errText = QString error text from kio
         * and the URL where the error happened is passed as a sub URL.
         */
        KUrl::List urls = KUrl::split( url );
        //kDebug() << "Handling error URL. URL count:" << urls.count();

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
                QString errorText = mainURL.queryItem( "errText" );
                urls.pop_front();
                KUrl reqUrl = KUrl::join( urls );

                //kDebug() << "Setting Url back to => " << reqUrl;
                emit d->browserExtension->setLocationBarUrl(reqUrl.prettyUrl());
                setUrl(reqUrl);

                emit started(0);
                showError(htmlError(error, errorText, reqUrl));
                emit completed();
            }

            return true;
        }
    }


    setUrl(url);

    if (url.url() == "about:blank") {
        emit setWindowCaption (url.url());
    } else {
        KParts::OpenUrlArguments args (arguments());
        KIO::MetaData metaData (args.metaData());

        // Get any SSL information already present. This is necessary
        // because Konqueror always does a lookup on a url to determine
        // mime-type so that it can embed the proper part. If a site was
        // SSL protected, the SSL information can be lost unless we set
        // it here...
        if (metaData.contains("ssl_in_use")) {
            WebSslInfo sslinfo;
            sslinfo.fromMetaData(metaData.toVariant());
            sslinfo.setUrl(url);
            d->webPage->setSslInfo(sslinfo);
        }

        // Check if the request originated from history navigation and
        // attempt to sync between Konqueror's and QtWebKit's history.
        // If syncing fails for any reason, fallback to opening the
        // url as if it is a new request...
        if (metaData.contains("restore-state")) {
            QWebHistory *history = d->webPage->history();
            if (history && history->count()) {
                QListIterator<QWebHistoryItem> historyIt (history->backItems(history->count()));
                historyIt.toBack(); // Search backwards (LIFO)...
                while (historyIt.hasPrevious()) {
                    QWebHistoryItem item (historyIt.previous());
                    if (url == item.originalUrl()) {
                        //kDebug() << "History back => " << url;
                        history->goToItem(item);
                        return true;
                    }
                }

                // Look in the forward items list next.
                historyIt = history->forwardItems(history->count());
                while (historyIt.hasNext()) {
                    QWebHistoryItem item (historyIt.next());
                    if (url == item.originalUrl()) {
                      //kDebug() << "History forward => " << url;
                      history->goToItem(item);
                      return true;
                    }
                }
            }
        }

        d->webView->loadUrl(url, args, browserExtension()->browserArguments());
    }

    return true;
}

bool WebKitPart::closeUrl()
{
    d->webView->stop();
    return true;
}

KParts::BrowserExtension *WebKitPart::browserExtension() const
{
    return d->browserExtension;
}

bool WebKitPart::openFile()
{
    // never reached
    return false;
}

void WebKitPart::loadStarted()
{
    d->updateHistory = false;
    emit started(0);
}

void WebKitPart::loadFinished(bool ok)
{
    d->updateHistory = true;

    if (ok && d->webView->title().trimmed().isEmpty()) {
        // If the document title is null, then set it to the current url
        // squeezed at the center.
        QString caption (d->webView->url().toString((QUrl::RemoveQuery|QUrl::RemoveFragment)));
        emit setWindowCaption(KStringHandler::csqueeze(caption));

        // The urlChanged signal is emitted if and only if the main frame
        // receives the title of the page so we manually invoke the slot as
        // a work around here for pages that do not contain it, such as
        // text documents...
        urlChanged(d->webView->url());
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
      if (it.key().isEmpty() &&
          it.value().toLower().simplified().contains(QRegExp("[0-9];url"))) {
        refresh = true;
        break;
      }
    }
    emit completed(refresh);
#else
    emit completed();
#endif
}

void WebKitPart::loadAborted(const QUrl & url)
{  
    closeUrl();
    if (url.isValid())
      emit d->browserExtension->openUrlRequest(url);
    else
      setUrl(d->webView->url());
}

void WebKitPart::loadError(int errCode, const QString &errStr, const QString &frameName)
{
    showError(htmlError(errCode, errStr, url()), frameName);
}

void  WebKitPart::navigationRequestFinished()
{
    if (d->webPage->sslInfo().isValid())
      d->browserExtension->setPageSecurity(WebKitPart::WebKitPartPrivate::Encrypted);
    else
      d->browserExtension->setPageSecurity(WebKitPart::WebKitPartPrivate::Unencrypted);
}

void WebKitPart::urlChanged(const QUrl& _url)
{  
    setUrl(_url);
    emit d->browserExtension->setLocationBarUrl(KUrl(_url).prettyUrl());
}

void WebKitPart::showSecurity()
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

void WebKitPart::updateHistory()
{  
    kDebug() << d->updateHistory;
    if (d->updateHistory) {
        emit d->browserExtension->openUrlNotify();
    }
}

QWebView * WebKitPart::view()
{
    return d->webView;
}

void WebKitPart::setStatusBarTextProxy(const QString &message)
{
    //kDebug() << message;
    emit setStatusBarText(message);
}

void WebKitPart::linkHovered(const QString &link, const QString &, const QString &)
{
    QString message;
    QUrl linkUrl (link);
    const QString scheme = linkUrl.scheme();

    if (QString::compare(scheme, QL1("mailto"), Qt::CaseInsensitive) == 0) {
        message += i18n("Email: ");

        // Workaround: for QUrl's parsing deficiencies of "mailto:foo@bar.com".
        if (!linkUrl.hasQuery())
          linkUrl = QUrl(scheme + '?' + linkUrl.path());

        QMap<QString, QStringList> fields;
        QPair<QString, QString> queryItem;

        Q_FOREACH (queryItem, linkUrl.queryItems()) {
            //kDebug() << "query: " << queryItem.first << queryItem.second;
            if (queryItem.first.contains(QChar('@')) && queryItem.second.isEmpty())
                fields["to"] << queryItem.first;
            if (QString::compare(queryItem.first, QL1("to"), Qt::CaseInsensitive) == 0)
                fields["to"] << queryItem.second;
            if (QString::compare(queryItem.first, QL1("cc"), Qt::CaseInsensitive) == 0)
                fields["cc"] << queryItem.second;
            if (QString::compare(queryItem.first, QL1("bcc"), Qt::CaseInsensitive) == 0)
                fields["bcc"] << queryItem.second;
            if (QString::compare(queryItem.first, QL1("subject"), Qt::CaseInsensitive) == 0)
                fields["subject"] << queryItem.second;
        }

        if (fields.contains(QL1("to")))
            message += fields.value(QL1("to")).join(QL1(", "));
        if (fields.contains(QL1("cc")))
            message += QL1(" - CC: ") + fields.value(QL1("cc")).join(QL1(", "));
        if (fields.contains(QL1("bcc")))
            message += QL1(" - BCC: ") + fields.value(QL1("bcc")).join(QL1(", "));
        if (fields.contains(QL1("subject")))
            message += QL1(" - Subject: ") + fields.value(QL1("subject")).join(QL1(" "));
    } else {
        message = link;
    }

    emit setStatusBarText(message);
}

void WebKitPart::showError(const QString &html, const QString &frameName)
{
  QWebFrame *frame = 0;

  if (frameName.isEmpty()) {
      frame = d->webView->page()->mainFrame();
  } else {
      QWebFrame *childFrame;
      QListIterator<QWebFrame*> it (d->webView->page()->mainFrame()->childFrames());
      while (it.hasNext()) {
          childFrame = it.next();
          if (childFrame->frameName() == frameName) {
              frame = childFrame;
              break;
          }
      }
  }

  Q_ASSERT (frame);

  frame->setHtml(html);
}

void WebKitPart::searchForText(const QString &text, bool backward)
{
    QWebPage::FindFlags flags;

    if (backward)
      flags = QWebPage::FindBackward;

    if (d->searchBar->caseSensitive())
        flags |= QWebPage::FindCaseSensitively;

    d->searchBar->setFoundMatch(d->webView->page()->findText(text, flags));
}

void WebKitPart::showSearchBar()
{
    const QString text = d->webView->selectedText();
    
    if (text.isEmpty())
        d->webView->pageAction(QWebPage::Undo);
    else
        d->searchBar->setSearchText(text.left(150));

    d->searchBar->show();
}

#include "webkitpart.moc"
