/*
    SPDX-FileCopyrightText: 2001 Malte Starostik <malte@kde.org>
    SPDX-FileCopyrightText: 2020 Jonathan Marten <jjm@keelhaul.me.uk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "webarchivecreator.h"

#include <QDebug>
#include <QPixmap>
#include <QImage>
#include <QApplication>
#include <QUrl>
#include <QTimer>
#include <QMimeType>
#include <QMimeDatabase>
#include <QTemporaryDir>

#ifdef THUMBNAIL_USE_WEBKIT
#include <qwebview.h>
#include <qwebpage.h>
#include <qwebsettings.h>
#include <QNetworkCookie>
#else // THUMBNAIL_USE_WEBKIT
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineCookieStore>
#endif // THUMBNAIL_USE_WEBKIT

#include <ktar.h>
#include <karchivedirectory.h>

#include "webarchiverdebug.h"


#undef SHOW_RENDER_WINDOW

//TODO KF6: remove all instances of THUMBNAIL_USE_WEBKIT

// This is an time limit for the entire thumbnail generation process
// (page loading and rendering).  If it expires then it is assumed
// that there is a problem and no thumbnail is generated.
static const int c_completionTimeout = 5000;

// After the page is loaded, the rendering happens in the background
// with no way to find out when it has finished.  So this timer sets a
// reasonable time for that to happen, when it expires the thumbnail
// image is generated.
static const int c_renderTimeout = 500;

// The size of the pixmap onto which the rendered page is drawn, and
// the rendering scale for the web page.  These settings have nothing
// to do with the size of the pixmap requested when create() is called,
// they are chosen for a reasonable rendering of the page (which should
// work at an effective width of 800 pixels).  For the scale factor,
// 0.25 is the minimum allowed by Qt.
static const QSize c_pixmapSize = QSize(400, 600);
static const double c_renderScale = 0.5;


extern "C"
{
    Q_DECL_EXPORT KIO::ThumbnailCreator *new_creator()
    {
        return (new WebArchiveCreator{nullptr, {}});
    }
}

WebArchiveCreator::WebArchiveCreator(QObject *parent, const QVariantList &va)
    : KIO::ThumbnailCreator(parent, va)
{
    qDebug() << "WEBARCHIVECREATOR CREATED";
    m_tempDir = nullptr;
}


WebArchiveCreator::~WebArchiveCreator()
{
    delete m_tempDir;
}


#ifndef THUMBNAIL_USE_WEBKIT
static bool disallowWebEngineCookies(const QWebEngineCookieStore::FilterRequest &req)
{
    return (false);
}
#endif // THUMBNAIL_USE_WEBKIT

KIO::ThumbnailResult WebArchiveCreator::create(const KIO::ThumbnailRequest& request)
{
    // QImage img;
    // bool success = create(request.url().path(), request.targetSize().width(), request.targetSize().height(), img);
    // return success ? KIO::ThumbnailResult::pass(img) : KIO::ThumbnailResult::fail();
    QString path = request.url().path();
    int width = request.targetSize().width();
    int height = request.targetSize().height();

    QMimeDatabase db;
    // Only use the file path to look up its MIME type.  Web archives are
    // gzip-compressed tar files, so if the content detection has to be
    // used it may report that.  So a web archive file must have the correct
    // file extension.
    QMimeType mimeType = db.mimeTypeForFile(path, QMimeDatabase::MatchExtension);

    qCDebug(WEBARCHIVERPLUGIN_LOG) << "path" << path;
    qCDebug(WEBARCHIVERPLUGIN_LOG) << "wh" << width << height << "mime" << mimeType.name();

    // We are using QWebEngine here directly, not the WebEnginePart KPart.
    // This means that it will only be able to use the network access methods
    // that it supports internally, effectively 'file' and 'http(s)'.  In particular
    // it does not support any other KIO protocols, including 'tar' which would
    // be needed to look into web archives.  The WebEnginePart interfaces QWebEngine
    // to KIO.
    //
    // One option would be to do the same, i.e. to implement a network access handler
    // or a URL scheme handler that forwards requests to KIO.  However, the random
    // and possible repeated access to the page elements required would mean lots
    // of seeking around in the compressed web archive file.  Therefore, the web
    // archive is first extracted into a temporary directory and then QWebEngine
    // is told to render that.

    QString indexFile = path;				// the main page to render

    if (mimeType.inherits("application/x-webarchive"))	// archive needs to be extracted?
    {
        KTar tar(path);					// auto-detects compression type
        tar.open(QIODevice::ReadOnly);
        const KArchiveDirectory *archiveDir = tar.directory();

        m_tempDir = new QTemporaryDir;
        const QString tempPath = m_tempDir->path();
        if (path.isEmpty())
        {
            qCWarning(WEBARCHIVERPLUGIN_LOG) << "Cannot create temporary directory";
            return (KIO::ThumbnailResult::fail());
        }

        qCDebug(WEBARCHIVERPLUGIN_LOG) << "extracting to tempPath" << tempPath;
        archiveDir->copyTo(tempPath, true);		// recursive extract from archive
        tar.close();					// finished with archive file

        const QDir tempDir(tempPath);
        const QStringList entries = tempDir.entryList(QDir::Files|QDir::QDir::NoDotAndDotDot);
        qCDebug(WEBARCHIVERPLUGIN_LOG) << "found" << entries.count() << "entries";

        QString indexHtml;
        for (const QString &name : entries)
        {
            // Look though the extracted archive files to try to identify the
            // HTML page is to be rendered.  If "index.html" or "index.htm" is
            // found, that file is used;  otherwise, the first HTML file that
            // was found is used.
            const QMimeType mime = db.mimeTypeForFile(tempDir.absoluteFilePath(name), QMimeDatabase::MatchExtension);
            if (mime.inherits("text/html"))
            {
                if (name.startsWith("index.", Qt::CaseInsensitive))
                {					// the index HTML file
                    indexHtml = name;
                    break;				// no need to look further
                }
                else if (indexHtml.isEmpty())		// any other HTML file
                {
                    indexHtml = name;
                }
            }
        }

        if (indexHtml.isEmpty())
        {
            qCWarning(WEBARCHIVERPLUGIN_LOG) << "No HTML file found in archive";
            return (KIO::ThumbnailResult::fail());
        }

        qCDebug(WEBARCHIVERPLUGIN_LOG) << "identified index file" << indexHtml;
        indexFile = tempPath+'/'+indexHtml;
    }

    const QUrl indexUrl = QUrl::fromLocalFile(indexFile);
    qCDebug(WEBARCHIVERPLUGIN_LOG) << "indexUrl" << indexUrl;

#ifdef THUMBNAIL_USE_WEBKIT
    QWebView view;
    connect(&view, &QWebView::loadFinished, this, &WebArchiveCreator::slotLoadFinished);

    QWebSettings *settings = view.settings();
    settings->setThirdPartyCookiePolicy(QWebSettings::AlwaysBlockThirdPartyCookies);
    settings->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls, false);
    settings->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, true);
    settings->setAttribute(QWebSettings::ZoomTextOnly, false);
    settings->setAttribute(QWebSettings::PrivateBrowsingEnabled, true);
    settings->setAttribute(QWebSettings::NotificationsEnabled, false);
    settings->setAttribute(QWebSettings::JavascriptEnabled, false);
    settings->setAttribute(QWebSettings::JavaEnabled, false);
    settings->setAttribute(QWebSettings::LocalStorageEnabled, false);
    settings->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls, false);
    settings->setAttribute(QWebSettings::PluginsEnabled, false);
    settings->setAttribute(QWebSettings::AllowRunningInsecureContent, false);
    settings->setAttribute(QWebSettings::PrintElementBackgrounds, true);
    settings->setAttribute(QWebSettings::PrivateBrowsingEnabled, true);

    QWebPage *page = view.page();
    auto *cookieJar = new WebArchiveCreatorCookieJar;
    page->networkAccessManager()->setCookieJar(cookieJar);
#else // THUMBNAIL_USE_WEBKIT
    QWebEngineView view;
    connect(&view, &QWebEngineView::loadFinished, this, &WebArchiveCreator::slotLoadFinished);

    QWebEngineSettings *settings = view.settings();
    settings->setUnknownUrlSchemePolicy(QWebEngineSettings::DisallowUnknownUrlSchemes);
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, false);
    settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, false);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
    settings->setAttribute(QWebEngineSettings::PluginsEnabled, false);
    settings->setAttribute(QWebEngineSettings::AutoLoadIconsForPage, false);
    settings->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, false);
    settings->setAttribute(QWebEngineSettings::ShowScrollBars, false);
    settings->setAttribute(QWebEngineSettings::PdfViewerEnabled, false);
    settings->setAttribute(QWebEngineSettings::PrintElementBackgrounds, true);

    QWebEnginePage *page = view.page();
    QWebEngineProfile *profile = page->profile();
    profile->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);
    profile->setSpellCheckEnabled(false);
    profile->cookieStore()->setCookieFilter(&disallowWebEngineCookies);
#endif // THUMBNAIL_USE_WEBKIT

    view.resize(c_pixmapSize);
    view.setZoomFactor(c_renderScale);				// 0.25 is the minimum allowed

    m_error = false;
    m_rendered = false;

    view.load(indexUrl);
#ifndef SHOW_RENDER_WINDOW
    view.setAttribute(Qt::WA_ShowWithoutActivating);
    view.setAttribute(Qt::WA_OutsideWSRange);
    view.setWindowFlags(view.windowFlags()|Qt::BypassWindowManagerHint|Qt::FramelessWindowHint);
    view.move(5000, 5000);
#endif
    view.show();

    QTimer::singleShot(c_completionTimeout, this, &WebArchiveCreator::slotProcessingTimeout);
    while (!m_error && !m_rendered) qApp->processEvents(QEventLoop::WaitForMoreEvents);
    qCDebug(WEBARCHIVERPLUGIN_LOG) << "finished loop error?" << m_error;
    if (m_error) return (KIO::ThumbnailResult::fail());			// load error or timeout

    // Render the HTML page on a bigger pixmap and leave the scaling to the
    // caller.  Looks better than directly scaling with the QPainter (malte).
    QSize pixSize = c_pixmapSize;
    if (pixSize.width()<width || pixSize.height()<height)
    {							// default size is too small
        if ((height*3)>(width*4)) pixSize = QSize(width, (width*4)/3);
        else pixSize = QSize((height*3)/4, height);
    }

    QPixmap pix(pixSize);
    // First fill the pixmap with a light grey background, in case the
    // rendered page does not completely cover it.  If there was an error
    // then we will already have given up above.
    pix.fill(QColor(245, 245, 245));

    view.render(&pix);					// render the view into the pixmap
    view.hide();					// finished with the view and page
#ifdef THUMBNAIL_USE_WEBKIT
    page->setVisibilityState(QWebPage::VisibilityStateHidden);
#else // THUMBNAIL_USE_WEBKIT

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    page->setLifecycleState(QWebEnginePage::LifecycleState::Discarded);
#endif // QT_VERSION
#endif // THUMBNAIL_USE_WEBKIT

    return KIO::ThumbnailResult::pass(pix.toImage());
}

void WebArchiveCreator::slotLoadFinished(bool ok)
{
    qCDebug(WEBARCHIVERPLUGIN_LOG) << "ok?" << ok;
    if (!ok)
    {
        // If WebKit is being used, it is possible that 'ok' can be false
        // here even if the page load succeeded but it could only be
        // partially rendered (for example, a broken image source link).
        // Ignore the error indication and render the page anyway.
#ifndef THUMBNAIL_USE_WEBKIT
        m_error = true;
        return;
#endif // THUMBNAIL_USE_WEBKIT
    }

#ifdef THUMBNAIL_USE_WEBKIT
    // WebKit will have finished rendering when the loadFinished() signal has been
    // delivered.  Render the bitmap immediately.
    slotRenderTimer();
#else // THUMBNAIL_USE_WEBKIT
    // WebEngine renders asynchronously after the loadFinished() signal has been
    // delivered.  It is not possible to tell when page rendering has finished, so
    // a timer is used and the page is assumed to be ready when it expires.
    QTimer::singleShot(c_renderTimeout, this, &WebArchiveCreator::slotRenderTimer);
#endif // THUMBNAIL_USE_WEBKIT
}


void WebArchiveCreator::slotProcessingTimeout()
{
    m_error = true;
}


void WebArchiveCreator::slotRenderTimer()
{
    m_rendered = true;
}


#ifdef THUMBNAIL_USE_WEBKIT

// WebArchiveCreatorCookieJar
//
// A cookie jar that ignores any cookies sent to it and never
// delivers any.

WebArchiveCreatorCookieJar::WebArchiveCreatorCookieJar(QObject *parent)
    : QNetworkCookieJar(parent)
{
}

QList<QNetworkCookie> WebArchiveCreatorCookieJar::cookiesForUrl(const QUrl &url) const
{
    return (QList<QNetworkCookie>());
}

bool WebArchiveCreatorCookieJar::insertCookie(const QNetworkCookie &cookie)
{
    return (false);
}


bool WebArchiveCreatorCookieJar::setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url)
{
    return (false);
}

#endif // THUMBNAIL_USE_WEBKIT
