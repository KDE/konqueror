/* This file is part of Webarchiver

    SPDX-FileCopyrightText: 2020 Jonathan Marten <jjm@keelhaul.me.uk>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QApplication>
#include <QCommandLineParser>
#include <QUrl>
#include <QIcon>

#include <kaboutdata.h>
#include <klocalizedstring.h>
#include <kcrash.h>

#include "archivedialog.h"
#include "webarchiverdebug.h"


int main(int argc,char *argv[])
{
    QApplication app(argc, argv);

    KAboutData aboutData(QStringLiteral("kcreatwebearchive"),		// componentName
                         i18n("Web Archiver"),		// displayName
                         i18n("0.0.1"),			// version
                         i18n("Archive a web page"),
                         KAboutLicense::GPL_V3,
                         i18n("Copyright (c) 2020 Jonathan Marten"));
    aboutData.addAuthor(i18n("Jonathan Marten"),
                        QString(),
                        QStringLiteral("jjm@keelhaul.me.uk"),
                        QStringLiteral("http://www.keelhaul.me.uk"));

    KAboutData::setApplicationData(aboutData);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("webarchiver")));
    KCrash::setDrKonqiEnabled(true);

    QCommandLineParser parser;
    parser.setApplicationDescription(aboutData.shortDescription());

    parser.addPositionalArgument(QStringLiteral("url"), i18n("URL of the web page to archive"), i18n("url"));

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    QUrl url;
    QStringList args = parser.positionalArguments();
    if (!args.isEmpty())
    {
        if (args.count()>1)
        {
            qCWarning(WEBARCHIVERPLUGIN_LOG) << "Only one URL argument is accepted";
        }

        url = QUrl::fromUserInput(args.first());
        if (!url.isValid())
        {
            qCCritical(WEBARCHIVERPLUGIN_LOG) << "Invalid URL argument";
            std::exit(EXIT_FAILURE);
        }
    }

    ArchiveDialog *ad = new ArchiveDialog(url);
    ad->show();
    return (app.exec());
}
