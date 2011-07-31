/*****************************************************
 * FSView, a simple TreeMap application
 *
 * (C) 2002, Josef Weidendorfer
 */

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <kglobal.h>
#include <kconfig.h>

#include "fsview.h"
#include <kconfiggroup.h>


int main(int argc, char* argv[])
{
  // KDE compliant startup
  KAboutData aboutData("fsview", 0, ki18n("FSView"), "0.1",
                       ki18n("Filesystem Viewer"),
                       KAboutData::License_GPL,
                       ki18n("(c) 2002, Josef Weidendorfer"));
  KCmdLineArgs::init(argc, argv, &aboutData);

  KCmdLineOptions options;
  options.add("+[folder]", ki18n("View filesystem starting from this folder"));
  KCmdLineArgs::addCmdLineOptions(options);
  KApplication a;

  KConfigGroup gconfig(KGlobal::config(), "General");
  QString path = gconfig.readPathEntry("Path", ".");

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  if (args->count()>0) path = args->arg(0);

  // TreeMap Widget as toplevel window
  FSView w(new Inode());

  QObject::connect(&w,SIGNAL(clicked(TreeMapItem*)),
                   &w,SLOT(selected(TreeMapItem*)));
  QObject::connect(&w,SIGNAL(returnPressed(TreeMapItem*)),
                   &w,SLOT(selected(TreeMapItem*)));
  QObject::connect(&w,
                   SIGNAL(contextMenuRequested(TreeMapItem*,QPoint)),
                   &w,SLOT(contextMenu(TreeMapItem*,QPoint)));

  w.setPath(path);
  w.show();

  a.connect( &a, SIGNAL(lastWindowClosed()), &w, SLOT(quit()) );
  return a.exec();
}
