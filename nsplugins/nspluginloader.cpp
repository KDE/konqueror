/*

  This is an encapsulation of the  Netscape plugin API.


  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
                     Stefan Schimanski <1Stein@gmx.de>
  Copyright (c) 2002-2005 George Staikos <staikos@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include "nspluginloader.h"
#include "nspluginloader.moc"

#include <QDir>
#include <QGridLayout>
#include <QResizeEvent>


#include <kapplication.h>
#include <kprocess.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <QLayout>
#include <QObject>
#include <QPushButton>
#include <QtGui/QX11EmbedContainer>
#include <QTextStream>
#include <QRegExp>
#include <QTimer>

#include "nsplugins_class_interface.h"
#include "nsplugins_instance_interface.h"
#include "nsplugins_viewer_interface.h"

#include <config-apps.h>

#include <X11/Xlib.h>
#include <unistd.h>

NSPluginLoader *NSPluginLoader::s_instance = 0;
int NSPluginLoader::s_refCount = 0;


NSPluginInstance::NSPluginInstance(QWidget *parent, const QString& viewerDBusId, const QString& id)
  : EMBEDCLASS(parent)
{
    setWindowTitle("nsp.host"); // for debugging..
    _instanceInterface = new org::kde::nsplugins::Instance( viewerDBusId, id, QDBusConnection::sessionBus() );

    _loader = 0;
    haveSize = false;
    inited   = false;

    QGridLayout *_layout = new QGridLayout(this);
    _layout->setMargin(1);
    _layout->setSpacing(1);
    KConfig _cfg( "kcmnspluginrc" );
    KConfigGroup cfg(&_cfg, "Misc");
    if ( cfg.readEntry("demandLoad", false) ) {
        _button = new QPushButton(i18n("Start Plugin"), dynamic_cast<EMBEDCLASS*>(this));
        _layout->addWidget(_button, 0, 0);
        connect(_button, SIGNAL(clicked()), this, SLOT(doLoadPlugin()));
        show();
    } else {
        _button = 0;
    }
}


void NSPluginInstance::doLoadPlugin(int w, int h) {
    if (!inited) {
        delete _button;
        _button = 0L;
        _loader = NSPluginLoader::instance();
        // resize before showing, some plugins are stupid and can't handle repeated
        // NPSetWindow() calls very well (viewer will avoid the call if not shown yet)
        qApp->syncX();
        _instanceInterface->setupWindow(winId(), w, h);
        inited = true;
    }
}

NSPluginInstance::~NSPluginInstance()
{
   kDebug() << "-> NSPluginInstance::~NSPluginInstance";
   _instanceInterface->shutdown();
   kDebug() << "release";
   _loader->release();
   kDebug() << "<- NSPluginInstance::~NSPluginInstance";
}


void NSPluginInstance::windowChanged(WId w)
{
    if (w == 0) {
        // FIXME: Put a notice here to tell the user that it crashed.
        repaint();
    }
}

void NSPluginInstance::pluginResized(int w, int h)
{
    kDebug() << w << h;
    haveSize = true;
    embedIfNeeded(w, h);
}

void NSPluginInstance::embedIfNeeded(int w, int h)
{
    if (isVisible()) {
        if (haveSize && !inited)
            doLoadPlugin(w, h);
        else if (inited)
            resizePlugin(w, h);
    }
}

void NSPluginInstance::resizeEvent(QResizeEvent *event)
{
    kDebug() << width() << height() << isVisible() << haveSize << inited;
    EMBEDCLASS::resizeEvent(event);

    embedIfNeeded(width(), height());
}

void NSPluginInstance::showEvent(QShowEvent *event)
{
    kDebug() << width() << height() << isVisible() << haveSize << inited;
    EMBEDCLASS::showEvent(event);

    embedIfNeeded(width(), height());
}

void NSPluginInstance::javascriptResult(int id, const QString &result)
{
    _instanceInterface->javascriptResult( id, result );
}

void NSPluginInstance::focusInEvent( QFocusEvent* event )
{
  Q_UNUSED( event );
  _instanceInterface->gotFocusIn();
}

void NSPluginInstance::focusOutEvent( QFocusEvent* event )
{
  Q_UNUSED( event );
  _instanceInterface->gotFocusOut();
}

void NSPluginInstance::resizePlugin( int w, int h )
{
  qApp->syncX();
  _instanceInterface->resizePlugin( clientWinId(), w, h );
}

/*******************************************************************************/


NSPluginLoader::NSPluginLoader()
   : QObject(), _mapping(), _viewer(0)
{
  scanPlugins();
}


NSPluginLoader *NSPluginLoader::instance()
{
  if (!s_instance)
    s_instance = new NSPluginLoader;

  s_refCount++;
  kDebug() << "NSPluginLoader::instance -> " <<  s_refCount;

  return s_instance;
}


void NSPluginLoader::release()
{
   s_refCount--;
   kDebug() << "NSPluginLoader::release -> " <<  s_refCount;

   if (s_refCount==0)
   {
      delete s_instance;
      s_instance = 0;
   }
}


NSPluginLoader::~NSPluginLoader()
{
   kDebug() << "-> NSPluginLoader::~NSPluginLoader";
   unloadViewer();
   kDebug() << "<- NSPluginLoader::~NSPluginLoader";
}


void NSPluginLoader::scanPlugins()
{
  QRegExp version(";version=[^:]*:");

  // open the cache file
  QFile cachef(KStandardDirs::locate("data", "nsplugins/cache"));
  if (!cachef.open(QIODevice::ReadOnly)) {
      kDebug() << "Could not load plugin cache file!";
      return;
  }

  QTextStream cache(&cachef);

  // read in cache
  QString line, plugin;
  while (!cache.atEnd()) {
      line = cache.readLine();
      if (line.isEmpty() || (line.left(1) == "#"))
        continue;

      if (line.left(1) == "[")
        {
          plugin = line.mid(1,line.length()-2);
          continue;
        }

      QStringList desc = line.split(':', QString::KeepEmptyParts);
      QString mime = desc[0].trimmed();
      QStringList suffixes;
      // If there are no suffixes, this would cause a crash
      if (desc.count() > 1)
        suffixes = desc[1].trimmed().split(',');
      if (!mime.isEmpty())
        {
          // insert the mimetype -> plugin mapping
          _mapping.insert(mime, QString(plugin).toLower());

          // insert the suffix -> mimetype mapping
          QStringList::Iterator suffix;
          for (suffix = suffixes.begin(); suffix != suffixes.end(); ++suffix) {

              // strip whitspaces and any preceding '.'
              QString stripped = (*suffix).trimmed();

              int p=0;
              for ( ; p<stripped.length() && stripped[p]=='.'; p++ ) {
                  ;
              }
              stripped = stripped.right( stripped.length()-p );

              // add filetype to list
              if ( !stripped.isEmpty() && !_filetype.contains(stripped) )
                  _filetype.insert( stripped, QString(mime));
          }
        }
    }
}


QString NSPluginLoader::lookupMimeType(const QString &url)
{
   QString result;
   QHashIterator<QString, QString> dit2(_filetype);
   while ( dit2.hasNext() ) {
    dit2.next();
    QString ext = QString(".")+dit2.key();
    if (url.right(ext.length()) == ext) {
        result = dit2.value();
        break;
    }
    }
    
   return result;
}


QString NSPluginLoader::lookup(const QString &mimeType)
{
    QString plugin;
    if (  _mapping.contains(mimeType) )
        plugin = _mapping.value(mimeType);

  kDebug() << "Looking up plugin for mimetype " << mimeType << ": " << plugin;

  return plugin;
}


bool NSPluginLoader::loadViewer()
{
   kDebug() << "NSPluginLoader::loadViewer";

   _process.clearProgram();
   // get the dbus app id
   int pid = (int)getpid();
   QString tmp;
   tmp.sprintf("org.kde.nspluginviewer-%d",pid);
   _viewerDBusId =tmp.toLatin1();

   connect( &_process, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(processTerminated(int , QProcess::ExitStatus)) );

   // find the external viewer process
   QString viewer = KGlobal::dirs()->findExe("nspluginviewer");
   if (viewer.isEmpty())
   {
      kDebug() << "can't find nspluginviewer";
      return false;
   }

   _process << viewer;

   _process << "-dbusservice";
   _process << _viewerDBusId;

   // run the process
   kDebug() << "Running nspluginviewer";
   _process.start();

   // wait for the process to run
   int cnt = 0;
   while (!QDBusConnection::sessionBus().interface()->isServiceRegistered(_viewerDBusId))
   {
       //kapp->processEvents(); // would lead to recursive calls in khtml
#ifdef HAVE_USLEEP
       usleep( 50*1000 );
#else
      sleep(1); kDebug() << "sleep";
#endif
      cnt++;
#ifdef HAVE_USLEEP
      if (cnt >= 100)
#else
      if (cnt >= 10)
#endif
      {
         kDebug() << "timeout";
         _process.kill();
         return false;
      }

      if (_process.state() == QProcess::NotRunning)
      {
         kDebug() << "nspluginviewer terminated";
         return false;
      }
   }

   // get viewer dcop interface
   _viewer = new org::kde::nsplugins::Viewer( _viewerDBusId, "/Viewer", QDBusConnection::sessionBus() );

   return _viewer!=0;
}


void NSPluginLoader::unloadViewer()
{
   kDebug() << "-> NSPluginLoader::unloadViewer";

   if ( _viewer )
   {
      _viewer->shutdown();
      kDebug() << "Shutdown viewer";
      delete _viewer;
      _process.kill();
      _viewer = 0;
   }

   kDebug() << "<- NSPluginLoader::unloadViewer";
}




void NSPluginLoader::processTerminated()
{
      kDebug() << "Viewer process  terminated";
      delete _viewer;
      _viewer = 0;
}


NSPluginInstance *NSPluginLoader::newInstance(QWidget *parent, const QString& url,
                                              const QString& mimeType, bool embed,
                                              const QStringList& _argn, const QStringList& _argv,
                                              const QString& ownDBusId, const QString& callbackId, bool reload )
{
   kDebug() << "-> NSPluginLoader::NewInstance( parent=" << (void*)parent << ", url=" << url << ", mime=" << mimeType << ", ...)";

   if ( !_viewer )
   {
      // load plugin viewer process
      loadViewer();

      if ( !_viewer )
      {
         kDebug() << "No viewer dcop stub found";
         return 0;
      }
   }

   kDebug() << "-> ownID" << ownDBusId << " viewer ID:" << _viewerDBusId;

   QStringList argn( _argn );
   QStringList argv( _argv );

   // check the mime type
   QString mime = mimeType;
   if (mime.isEmpty())
   {
      mime = lookupMimeType( url );
      argn << "MIME";
      argv << mime;
   }
   if (mime.isEmpty())
   {
      kDebug() << "Unknown MimeType";
      return 0;
   }

   // lookup plugin for mime type
   QString plugin_name = lookup(mime);
   if (plugin_name.isEmpty())
   {
      kDebug() << "No suitable plugin";
      return 0;
   }

   // get plugin class object
   QDBusObjectPath cls_ref = _viewer->newClass( plugin_name, ownDBusId );
   if ( cls_ref.path().isEmpty() )
   {
      kDebug() << "Couldn't create plugin class";
      return 0;
   }

   org::kde::nsplugins::Class* cls = new org::kde::nsplugins::Class( _viewerDBusId, cls_ref.path(), QDBusConnection::sessionBus() );

   // handle special plugin cases
   if ( mime=="application/x-shockwave-flash" )
       embed = true; // flash doesn't work in full mode :(


   // get plugin instance
   QDBusObjectPath inst_ref = cls->newInstance( url, mime, embed, argn, argv, ownDBusId, callbackId, reload );
   if ( inst_ref.path().isEmpty() )
   {
      kDebug() << "Couldn't create plugin instance";
      delete cls;
      return 0;
   }

   NSPluginInstance *plugin = new NSPluginInstance( parent, _viewerDBusId, inst_ref.path() );

   kDebug() << "<- NSPluginLoader::NewInstance = " << (void*)plugin;

   delete cls;
   return plugin;
}

// vim: ts=4 sw=4 et
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
