#include "kcustommenueditor.h"
#include <kapplication.h>
#include <klocale.h>
#include <kconfig.h>
#include <kcmdlineargs.h>

int main(int argc, char** argv)
{
  KLocale::setMainCatalog("kdelibs");
  KCmdLineArgs::init(argc, argv, "kcustommenueditortest","kcustommenueditortest","test app","0");
  KApplication app;
  app.setQuitOnLastWindowClosed(false);
  KCustomMenuEditor editor(0);
  KConfig *cfg = new KConfig("kdesktop_custom_menu2");
  editor.load(cfg);
  if (editor.exec())
  {
     editor.save(cfg);
     cfg->sync();
  }
  delete cfg;
}

