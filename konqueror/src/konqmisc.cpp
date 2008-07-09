/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>

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
#include "konqsessionmanager.h"
#include "konqsettingsxt.h"
#include "konqmainwindow.h"
#include "konqviewmanager.h"
#include "konqview.h"

#include <kapplication.h>
#include <kdebug.h>
#include <kmessagebox.h>
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

KonqMainWindow * KonqMisc::createNewWindow( const KUrl &url, const KParts::OpenUrlArguments &args,
                                            const KParts::BrowserArguments& browserArgs,
                                            bool forbidUseHTML, const QStringList &filesToSelect, bool tempFile, bool openUrl )
{
  kDebug() << "KonqMisc::createNewWindow url=" << url;

  // For HTTP or html files, use the web browsing profile, otherwise use filemanager profile
  QString profileName = (!(KProtocolManager::supportsListing(url)) ||
                        KMimeType::findByUrl(url)->name() == "text/html")
          ? "webbrowsing" : "filemanagement";

  QString profile = KStandardDirs::locate( "data", QLatin1String("konqueror/profiles/") + profileName );
  return createBrowserWindowFromProfile(profile, profileName,
					url, args, browserArgs,
					forbidUseHTML, filesToSelect, tempFile, openUrl );
}

KonqMainWindow * KonqMisc::createBrowserWindowFromProfile( const QString& _path, const QString &filename, const KUrl &url,
                                                           const KParts::OpenUrlArguments &args,
                                                           const KParts::BrowserArguments& browserArgs,
                                                           bool forbidUseHTML, const QStringList& filesToSelect, bool tempFile, bool openUrl )
{
    QString path(_path);
    kDebug(1202) << "path=" << path << ", filename=" << filename << ", url=" << url;
    Q_ASSERT(!path.isEmpty());
    // Well the path can be empty when misusing DBUS calls....
    if (path.isEmpty())
        path = defaultProfilePath();

  abortFullScreenMode();

  KonqOpenURLRequest req;
  req.args = args;
  req.browserArgs = browserArgs;
  req.filesToSelect = filesToSelect;
  req.tempFile = tempFile;

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
      mainWindow->viewManager()->loadViewProfileFromFile( path, filename, url, req, true );
  }
  else
  {
      KSharedConfigPtr cfg = KSharedConfig::openConfig(path, KConfig::SimpleConfig);
      const KConfigGroup profileGroup(cfg, "Profile");
      const QString xmluiFile = profileGroup.readPathEntry("XMLUIFile","konqueror.rc");

      mainWindow = new KonqMainWindow(KUrl(), xmluiFile);
      mainWindow->viewManager()->loadViewProfileFromConfig(cfg, path, filename, url, req, false, openUrl);
  }
  if ( forbidUseHTML )
      mainWindow->setShowHTML( false );
  mainWindow->setInitialFrameName( browserArgs.frameName );
  mainWindow->show();
  return mainWindow;
}

KonqMainWindow * KonqMisc::newWindowFromHistory( KonqView* view, int steps )
{
  int oldPos = view->historyIndex();
  int newPos = oldPos + steps;

  const HistoryEntry * he = view->historyAt(newPos);
  if(!he)
      return 0L;

  KonqMainWindow* mainwindow = createNewWindow(he->url, KParts::OpenUrlArguments(),
                                               KParts::BrowserArguments(),
					       false, QStringList(), false, /*openUrl*/false);
  if(!mainwindow)
      return 0L;
  KonqView* newView = mainwindow->currentView();

  if(!newView)
      return 0L;

  newView->copyHistory(view);
  newView->setHistoryIndex(newPos);
  newView->restoreHistory();
  return mainwindow;
}

QString KonqMisc::konqFilteredURL( QWidget* parent, const QString& _url, const QString& _path )
{
  if ( !_url.startsWith( "about:" ) ) // Don't filter "about:" URLs
  {
    KUriFilterData data(_url);

    if( !_path.isEmpty() )
      data.setAbsolutePath(_path);

    // We do not want to the filter to check for executables
    // from the location bar.
    data.setCheckForExecutables (false);

    if( KUriFilter::self()->filterUri( data ) )
    {
      if( data.uriType() == KUriFilterData::Error && !data.errorMsg().isEmpty() )
      {
        KMessageBox::sorry( parent, i18n( data.errorMsg().toUtf8() ) );
        return QString();
      }
      else
        return data.uri().url();
    }
  }
  else if ( _url != "about:blank" && _url != "about:plugins" ) {
    return "about:";
  }
  return _url;  // return the original url if it cannot be filtered.
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

#include "konqmisc.moc"
