/* This file is part of the KDE project
   Copyright (C) 1999-2006 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KFMCLIENT_H
#define KFMCLIENT_H

#include <QObject>
class KJob;
class QUrl;
class QString;
class QCommandLineParser;

class ClientApp : public QObject
{
    Q_OBJECT
public:
    ClientApp();

    /** Parse command-line arguments and "do it" */
    bool doIt(const QCommandLineParser &parser);

    /** Make konqueror open a window for @p url */
    bool createNewWindow(const QUrl &url, bool newTab, bool tempFile, const QString &mimetype = QString());

    /** Make konqueror open a window for @p profile, @p url and @p mimetype, deprecated */
    bool openProfile(const QString &profile, const QUrl &url, const QString &mimetype = QString());

private Q_SLOTS:
    void slotResult(KJob *job);

private:

    //Stores the result of parsing the BrowserApplication option
    struct BrowserApplicationParsingResult {
        //The string was parsed successfully
        bool isValid = false;
        //True if the string represents a command and false if it represents a service
        bool isCommand = false;
        //A string describing errors occurred while parsing the option
        QString error;
        //The command executable or service name
        QString commandOrService;
        //The arguments to pass to command. It's empty if isCommand is false
        QStringList args;
    };

    void delayedQuit();

    bool launchExternalBrowser(const BrowserApplicationParsingResult& parseResult, const QUrl &url, bool tempFile);

    //Parses the content of the BrowserApplication option
    static BrowserApplicationParsingResult parseBrowserApplicationString(const QString &str);


private:
    bool m_interactive = true;
};

#endif
