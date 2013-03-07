/* This file is part of the KDE project
   Copyright (C) 1998, 1999, 2010 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "konqmisc.h"
#include <kparts/browserrun.h>
#include <QDir>
#include "konqsessionmanager.h"
#include "konqsettingsxt.h"
#include "konqmainwindow.h"
#include "konqviewmanager.h"
#include "konqview.h"

#include <kapplication.h>
#include <kdebug.h>
#include <kurifilter.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kwindowsystem.h>
#include <kprotocolmanager.h>
#include <kstartupinfo.h>
#include <kiconloader.h>
#include <kconfiggroup.h>
#include <QList>

/**********************************************
 *
 * KonqMisc
 *
 **********************************************/

// Terminates fullscreen-mode for any full-screen window on the current desktop
void KonqMisc::abortFullScreenMode()
{
#ifdef Q_WS_X11
  QList<KonqMainWindow*> *mainWindows = KonqMainWindow::mainWindowList();
  if ( mainWindows )
  {
    foreach ( KonqMainWindow* window, *mainWindows )
    {
      if ( window->fullScreenMode() )
      {
	KWindowInfo info = KWindowSystem::windowInfo( window->winId(), NET::WMDesktop );
	if ( info.valid() && info.isOnCurrentDesktop() )
          window->setWindowState( window->windowState() & ~Qt::WindowFullScreen );
      }
    }
  }
#endif
}

KonqMainWindow * KonqMisc::createSimpleWindow( const KUrl & url, const KParts::OpenUrlArguments &args,
                                               const KParts::BrowserArguments& browserArgs,
                                               bool tempFile )
{
  abortFullScreenMode();

  KonqOpenURLRequest req;
  req.args = args;
  req.browserArgs = browserArgs;
  req.tempFile = tempFile;
  KonqMainWindow *win = new KonqMainWindow;
  win->openUrl( 0L, url, QString(), req );
  win->show();

  return win;
}

KonqMainWindow * KonqMisc::createNewWindow(const KUrl &url, const KonqOpenURLRequest& req, bool openUrl)
{
    //kDebug() << "url=" << url;
    // For HTTP or html files, use the web browsing profile, otherwise use filemanager profile
    const QString profileName = url.isEmpty() || // e.g. in window.open
                                (!(KProtocolManager::supportsListing(url)) || // e.g. any HTTP url
                                 KMimeType::findByUrl(url)->name() == "text/html")
                                ? "webbrowsing" : "filemanagement";

  const QString profilePath = KStandardDirs::locate( "data", QLatin1String("konqueror/profiles/") + profileName );
  return createBrowserWindowFromProfile(profilePath, profileName,
                                        url, req, openUrl);
}

KonqMainWindow * KonqMisc::createBrowserWindowFromProfile(const QString& _path, const QString &_filename, const KUrl &url,
                                                          const KonqOpenURLRequest& req, bool openUrl)
{
    QString path(_path);
    QString filename(_filename);
    if (path.isEmpty()) { // no path given, determine it from the filename
        if (filename.isEmpty()) {
            filename = defaultProfileName();
        }
        if (QDir::isRelativePath(filename)) {
            path = KStandardDirs::locate("data", QLatin1String("konqueror/profiles/")+filename);
            if (path.isEmpty()) { // not found
                filename = defaultProfileName();
                path = defaultProfilePath();
            }
        } else {
            path = filename; // absolute path
        }
    }

  abortFullScreenMode();
  KonqMainWindow * mainWindow;
  // Ask the user to recover session if appliable
  if(KonqSessionManager::self()->askUserToRestoreAutosavedAbandonedSessions())
  {
      QList<KonqMainWindow*> *mainWindowList = KonqMainWindow::mainWindowList();
      if(mainWindowList && !mainWindowList->isEmpty())
          mainWindow = mainWindowList->first();
      else // This should never happen but just to be sure
          mainWindow = new KonqMainWindow;

      if(!url.isEmpty())
          mainWindow->openUrl( 0, url, QString(), req );
  }
  else if( KonqMainWindow::isPreloaded() && KonqMainWindow::preloadedWindow() != NULL )
  {
      mainWindow = KonqMainWindow::preloadedWindow();
#ifdef Q_WS_X11
      KStartupInfo::setWindowStartupId( mainWindow->winId(), kapp->startupId());
#endif
      KonqMainWindow::setPreloadedWindow( NULL );
      KonqMainWindow::setPreloadedFlag( false );
      mainWindow->resetWindow();
      mainWindow->reparseConfiguration();
      mainWindow->viewManager()->loadViewProfileFromFile(path, filename, url, req, true, openUrl);
  }
  else
  {
      KSharedConfigPtr cfg = KSharedConfig::openConfig(path, KConfig::SimpleConfig);
      const KConfigGroup profileGroup(cfg, "Profile");
      const QString xmluiFile = profileGroup.readPathEntry("XMLUIFile","konqueror.rc");

      mainWindow = new KonqMainWindow(KUrl(), xmluiFile);
      mainWindow->viewManager()->loadViewProfileFromConfig(cfg, path, filename, url, req, false, openUrl);
  }
  mainWindow->setInitialFrameName( req.browserArgs.frameName );
  return mainWindow;
}

KonqMainWindow * KonqMisc::newWindowFromHistory( KonqView* view, int steps )
{
  int oldPos = view->historyIndex();
  int newPos = oldPos + steps;

  const HistoryEntry * he = view->historyAt(newPos);
  if(!he)
      return 0L;

  KonqMainWindow* mainwindow = createNewWindow(he->url, KonqOpenURLRequest(),
                                               /*openUrl*/false);
  if(!mainwindow)
      return 0L;
  KonqView* newView = mainwindow->currentView();

  if(!newView)
      return 0L;

  newView->copyHistory(view);
  newView->setHistoryIndex(newPos);
  newView->restoreHistory();
  mainwindow->show();
  return mainwindow;
}

KUrl KonqMisc::konqFilteredURL(KonqMainWindow* parent, const QString& _url, const QString& _path)
{
  Q_UNUSED(parent); // Useful if we want to change the error handling again

  if ( !_url.startsWith( QLatin1String("about:") ) ) { // Don't filter "about:" URLs
    KUriFilterData data(_url);

    if( !_path.isEmpty() )
      data.setAbsolutePath(_path);

    // We do not want to the filter to check for executables
    // from the location bar.
    data.setCheckForExecutables (false);

    if( KUriFilter::self()->filterUri( data ) ) {
      if( data.uriType() == KUriFilterData::Error ) {
        if (data.errorMsg().isEmpty()) {
          return KParts::BrowserRun::makeErrorUrl(KIO::ERR_MALFORMED_URL, _url, _url);
        } else {
          return KParts::BrowserRun::makeErrorUrl(KIO::ERR_SLAVE_DEFINED, data.errorMsg(), _url);
        }
      } else {
        return data.uri();
      }
    }

    // NOTE: a valid URL like http://kde.org always passes the filtering test.
    // As such, this point could only be reached when _url is NOT a valid URL.
    return KParts::BrowserRun::makeErrorUrl(KIO::ERR_MALFORMED_URL, _url, _url);
  }

  const bool isKnownAbout = (_url == QLatin1String("about:blank")
                             || _url == QLatin1String("about:plugins")
                             || _url.startsWith(QLatin1String("about:konqueror")));

  return isKnownAbout ? KUrl(_url) : KUrl("about:");
}

QString KonqMisc::defaultProfileName()
{
    // By default try to open in webbrowser mode. People can use "konqueror ." to get a filemanager.
    return "webbrowsing";
}

QString KonqMisc::defaultProfilePath()
{
    return KStandardDirs::locate("data", QLatin1String("konqueror/profiles/")+ defaultProfileName());
}

QString KonqMisc::encodeFilename(QString filename)
{
    return filename.replace(':', '_');
}

QString KonqMisc::decodeFilename(QString filename)
{
    return filename.replace('_', ':');
}

