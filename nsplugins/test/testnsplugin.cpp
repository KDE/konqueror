/*
  This is an encapsulation of the  Netscape plugin API.

  Copyright (c) 2000 Stefan Schimanski <1Stein@gmx.de>

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

#include "testnsplugin.h"
#include "../nspluginloader.h"

#include <stdio.h>

#include <QHBoxLayout>

#include <kactioncollection.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <kstandardaction.h>
#include <kaction.h>



TestNSPlugin::TestNSPlugin()
{
   m_loader = NSPluginLoader::instance();

   // client area
   m_client = new QWidget( this );
   setCentralWidget( m_client );
   m_client->show();
   m_layout = new QHBoxLayout( m_client );
   m_layout->setMargin( 0 );
   m_layout->setSpacing( 0 );

   // file menu
   actionCollection()->addAction(KStandardAction::Open, this, SLOT(newView()));
   actionCollection()->addAction(KStandardAction::Close, this, SLOT(closeView()));
   actionCollection()->addAction(KStandardAction::Quit,kapp,SLOT(quit()));

   createGUI( "testnspluginui.rc" );
}


TestNSPlugin::~TestNSPlugin()
{
   kDebug() << "-> TestNSPlugin::~TestNSPlugin";
   m_loader->release();
   kDebug() << "<- TestNSPlugin::~TestNSPlugin";
}


void TestNSPlugin::newView()
{
   QStringList _argn, _argv;

   //QString src = "file:/home/sschimanski/kimble_themovie.swf";
   //QString src = "file:/home/sschimanski/in_ani.swf";
   //QString src = "http://homepages.tig.com.au/~dkl/swf/promo.swf";
   //QString mime = "application/x-shockwave-flash";

   _argn << "name" << "controls" << "console";
   _argv << "audio" << "ControlPanel" << "Clip1";
   QString src = "http://welt.is-kunden.de:554/ramgen/welt/avmedia/realaudio/0701lw177135.rm";
//   QString src = "nothing";
   QString mime = "audio/x-pn-realaudio-plugin";

   _argn << "SRC" << "TYPE" << "WIDTH" << "HEIGHT";
   _argv << src << mime << "400" << "100";
   QWidget *win = m_loader->newInstance( m_client, src, mime, 1, _argn, _argv, QString("appid"), QString("callbackid"), false );

/*
    _argn << "TYPE" << "WIDTH" << "HEIGHT" << "java_docbase" << "CODE";
    _argv << "application/x-java-applet" << "450" << "350" << "file:///none" << "sun/plugin/panel/ControlPanelApplet.class";
    QWidget *win = loader->NewInstance(0, "", "application/x-java-applet", 1, _argn, _argv);
*/

   if ( win )
   {
      m_plugins.append( win );
      connect( win, SIGNAL(destroyed(NSPluginInstance *)),
               this, SLOT(viewDestroyed(NSPluginInstance *)) );
      m_layout->addWidget( win );
      win->show();
   } else
   {
      kDebug() << "No widget created";
   }
}

void TestNSPlugin::closeView()
{
   kDebug() << "closeView";
   QWidget *win = m_plugins.last();
   if ( win )
   {
      m_plugins.removeAll( win );
      delete win;
   } else
   {
      kDebug() << "No widget available";
   }
}


void TestNSPlugin::viewDestroyed( NSPluginInstance *inst )
{
   kDebug() << "TestNSPlugin::viewDestroyed";
   m_plugins.removeAll( inst );
}


int main(int argc, char *argv[])
{
   kDebug() << "main";
   setvbuf( stderr, NULL, _IONBF, 0 );
   KCmdLineArgs::init(argc, argv, "nsplugin", 0, ki18n("testnsplugin"), "0.1", ki18n("A Netscape Plugin test program"));

   KApplication app("nsplugin");

   TestNSPlugin win;
   win.show();
   app.exec();
}

#include "testnsplugin.moc"
