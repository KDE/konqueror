/*****************************************************
 * FSView, a simple TreeMap application
 *
 * (C) 2002, Josef Weidendorfer
 */

#include <kaboutdata.h>
#include <kconfig.h>

#include "fsview.h"
#include <kconfiggroup.h>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QApplication>
#include <KAboutData>
#include <KLocalizedString>
#include <KSharedConfig>

int main(int argc, char *argv[])
{
    // KDE compliant startup
    KAboutData aboutData("fsview", i18n("FSView"), "0.1",
                         i18n("Filesystem Viewer"),
                         KAboutLicense::GPL,
                         i18n("(c) 2002, Josef Weidendorfer"));
    QApplication app(argc, argv);
    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("+[folder]"), i18n("View filesystem starting from this folder")));

    KConfigGroup gconfig(KSharedConfig::openConfig(), "General");
    QString path = gconfig.readPathEntry("Path", ".");

    if (parser.positionalArguments().count() > 0) {
        path = parser.positionalArguments().at(0);
    }

    // TreeMap Widget as toplevel window
    FSView w(new Inode());

    QObject::connect(&w, SIGNAL(clicked(TreeMapItem*)),
                     &w, SLOT(selected(TreeMapItem*)));
    QObject::connect(&w, SIGNAL(returnPressed(TreeMapItem*)),
                     &w, SLOT(selected(TreeMapItem*)));
    QObject::connect(&w, SIGNAL(contextMenuRequested(TreeMapItem*,QPoint)),
                     &w, SLOT(contextMenu(TreeMapItem*,QPoint)));

    w.setPath(path);
    w.show();

    return app.exec();
}
