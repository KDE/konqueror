#include <QtCore/QDir>
#include <QtCore/QFile>

#include <kapplication.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>

#include "kfinddlg.h"
#include "version.h"

static const char description[] = I18N_NOOP("KDE file find utility");

int main( int argc, char ** argv )
{
  KAboutData aboutData( "kfind", "kfindpart", ki18n("KFind"),
      KFIND_VERSION, ki18n(description), KAboutData::License_GPL,
      ki18n("(c) 1998-2003, The KDE Developers"));

  aboutData.addAuthor(ki18n("Eric Coquelle"), ki18n("Current Maintainer"), "coquelle@caramail.com");
  aboutData.addAuthor(ki18n("Mark W. Webb"), ki18n("Developer"), "markwebb@adelphia.net");
  aboutData.addAuthor(ki18n("Beppe Grimaldi"), ki18n("UI Design & more search options"), "grimalkin@ciaoweb.it");
  aboutData.addAuthor(ki18n("Martin Hartig"));
  aboutData.addAuthor(ki18n("Stephan Kulow"), KLocalizedString(), "coolo@kde.org");
  aboutData.addAuthor(ki18n("Mario Weilguni"),KLocalizedString(), "mweilguni@sime.com");
  aboutData.addAuthor(ki18n("Alex Zepeda"),KLocalizedString(), "zipzippy@sonic.net");
  aboutData.addAuthor(ki18n("Miroslav Flídr"),KLocalizedString(), "flidr@kky.zcu.cz");
  aboutData.addAuthor(ki18n("Harri Porten"),KLocalizedString(), "porten@kde.org");
  aboutData.addAuthor(ki18n("Dima Rogozin"),KLocalizedString(), "dima@mercury.co.il");
  aboutData.addAuthor(ki18n("Carsten Pfeiffer"),KLocalizedString(), "pfeiffer@kde.org");
  aboutData.addAuthor(ki18n("Hans Petter Bieker"), KLocalizedString(), "bieker@kde.org");
  aboutData.addAuthor(ki18n("Waldo Bastian"), ki18n("UI Design"), "bastian@kde.org");
  aboutData.addAuthor(ki18n("Alexander Neundorf"), KLocalizedString(), "neundorf@kde.org");
  aboutData.addAuthor(ki18n("Clarence Dang"), KLocalizedString(), "dang@kde.org");

  KCmdLineArgs::init( argc, argv, &aboutData );

  KCmdLineOptions options;
  options.add("+[searchpath]", ki18n("Path(s) to search"));
  KCmdLineArgs::addCmdLineOptions( options );

  KApplication app;

  KCmdLineArgs *args= KCmdLineArgs::parsedArgs();

  KUrl url;
  if (args->count() > 0)
    url = args->url(0);
  if (url.isEmpty())
    url = QDir::currentPath();
  if (url.isEmpty())
    url = QDir::homePath();
  args->clear();

  KfindDlg kfinddlg(url);
  return kfinddlg.exec();
}
