/* This file is part of Webarchiver
 *
 *  Copyright (C) 2020 Jonathan Marten <jjm@keelhaul.me.uk>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 **/

#include <qapplication.h>
#include <qcommandlineparser.h>
#include <qurl.h>
#include <qicon.h>

#include <kaboutdata.h>
#include <klocalizedstring.h>
#include <kcrash.h>

#include "archivedialog.h"
#include "webarchiverdebug.h"


int main(int argc,char *argv[])
{
    KAboutData aboutData("kcreatwebearchive",		// componentName
                         i18n("Web Archiver"),		// displayName
                         i18n("0.0.1"),			// version
                         i18n("Archive a web page"),
                         KAboutLicense::GPL_V3,
                         i18n("Copyright (c) 2020 Jonathan Marten"),
                         "",				// otherText
                         "",				// homePageAddress
                         "");				// bugsEmailAddress
    aboutData.addAuthor(i18n("Jonathan Marten"),
                        "",
                        "jjm@keelhaul.me.uk",
                        "http://www.keelhaul.me.uk");

    QApplication app(argc, argv);
    KAboutData::setApplicationData(aboutData);
    app.setWindowIcon(QIcon::fromTheme("webarchiver"));
    KCrash::setDrKonqiEnabled(true);

    QCommandLineParser parser;
    parser.setApplicationDescription(aboutData.shortDescription());

    parser.addPositionalArgument("url", i18n("URL of the web page to archive"), i18n("url"));

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
