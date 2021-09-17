/*
    FSView, a simple TreeMap application

    SPDX-FileCopyrightText: 2002 Josef Weidendorfer
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
    QApplication app(argc, argv);
    // KDE compliant startup
    KAboutData aboutData(QStringLiteral("fsview"), i18n("FSView"), QStringLiteral("0.1"),
                         i18n("Filesystem Viewer"),
                         KAboutLicense::GPL,
                         i18n("(c) 2002, Josef Weidendorfer"));
    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("+[folder]"), i18n("View filesystem starting from this folder")));

    KConfigGroup gconfig(KSharedConfig::openConfig(), "General");
    QString path = gconfig.readPathEntry("Path", QStringLiteral("."));

    if (parser.positionalArguments().count() > 0) {
        path = parser.positionalArguments().at(0);
    }

    // TreeMap Widget as toplevel window
    FSView w(new Inode());

    QObject::connect(&w, &TreeMapWidget::clicked, &w, &FSView::selected);
    QObject::connect(&w, &TreeMapWidget::returnPressed, &w, &FSView::selected);
    QObject::connect(&w, &TreeMapWidget::contextMenuRequested, &w, &FSView::contextMenu);

    w.setPath(path);
    w.show();

    return app.exec();
}
