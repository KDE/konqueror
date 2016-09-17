/* This file is part of the KDE project
   Copyright (C) 1998, 1999, 2010 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "konqmisc.h"
#include <kparts/browserrun.h>
#include "konqsettingsxt.h"
#include "konqmainwindow.h"
#include "konqviewmanager.h"
#include "konqview.h"
#include "konqmainwindowfactory.h"

#include <kapplication.h>
#include <QDebug>
#include <kurifilter.h>
#include <KLocalizedString>

#include <kprotocolmanager.h>
#include <kiconloader.h>
#include <kconfiggroup.h>
#include <QList>
#include <QStandardPaths>
#include <KSharedConfig>

/**********************************************
 *
 * KonqMisc
 *
 **********************************************/

KonqMainWindow *KonqMisc::createNewWindow(const QUrl &url,
                                          const KonqOpenURLRequest &req)
{
    KonqMainWindow *mainWindow = KonqMainWindowFactory::createEmptyWindow();
    if (!url.isEmpty()) {
        mainWindow->openUrl(Q_NULLPTR, url, QString(), req);
        mainWindow->setInitialFrameName(req.browserArgs.frameName);
    } else {
        // TODO read config, to be able to disable this
        mainWindow->openUrl(Q_NULLPTR, QUrl("about:konqueror"), QStringLiteral("KonqAboutPage"));
        mainWindow->focusLocationBar();
    }
    return mainWindow;
}

KonqMainWindow *KonqMisc::newWindowFromHistory(KonqView *view, int steps)
{
    int oldPos = view->historyIndex();
    int newPos = oldPos + steps;

    const HistoryEntry *he = view->historyAt(newPos);
    if (!he) {
        return Q_NULLPTR;
    }

    KonqMainWindow *mainwindow = KonqMainWindowFactory::createEmptyWindow();
    if (!mainwindow) {
        return Q_NULLPTR;
    }
    KonqView *newView = mainwindow->currentView();

    if (!newView) {
        return Q_NULLPTR;
    }

    newView->copyHistory(view);
    newView->setHistoryIndex(newPos);
    newView->restoreHistory();
    mainwindow->show();
    return mainwindow;
}

QUrl KonqMisc::konqFilteredURL(KonqMainWindow *parent, const QString &_url, const QUrl &currentDirectory)
{
    Q_UNUSED(parent); // Useful if we want to change the error handling again

    if (!_url.startsWith(QLatin1String("about:"))) {     // Don't filter "about:" URLs
        KUriFilterData data(_url);

        if (currentDirectory.isLocalFile()) {
            data.setAbsolutePath(currentDirectory.toLocalFile());
        }

        // We do not want to the filter to check for executables
        // from the location bar.
        data.setCheckForExecutables(false);

        if (KUriFilter::self()->filterUri(data)) {
            if (data.uriType() == KUriFilterData::Error) {
                if (data.errorMsg().isEmpty()) {
                    return KParts::BrowserRun::makeErrorUrl(KIO::ERR_MALFORMED_URL, _url, QUrl(_url));
                } else {
                    return KParts::BrowserRun::makeErrorUrl(KIO::ERR_SLAVE_DEFINED, data.errorMsg(), QUrl(_url));
                }
            } else {
                return data.uri();
            }
        }

        // NOTE: a valid URL like http://kde.org always passes the filtering test.
        // As such, this point could only be reached when _url is NOT a valid URL.
        return KParts::BrowserRun::makeErrorUrl(KIO::ERR_MALFORMED_URL, _url, QUrl(_url));
    }

    const bool isKnownAbout = (_url == QLatin1String("about:blank")
                               || _url == QLatin1String("about:plugins")
                               || _url.startsWith(QLatin1String("about:konqueror")));

    return isKnownAbout ? QUrl(_url) : QUrl("about:");
}

QString KonqMisc::defaultProfileName()
{
    // By default try to open in webbrowser mode. People can use "konqueror ." to get a filemanager.
    return "webbrowsing";
}

QString KonqMisc::defaultProfilePath()
{
    return QStandardPaths::locate(QStandardPaths::GenericDataLocation, QLatin1String("konqueror/profiles/") + defaultProfileName());
}

QString KonqMisc::encodeFilename(QString filename)
{
    return filename.replace(':', '_');
}

QString KonqMisc::decodeFilename(QString filename)
{
    return filename.replace('_', ':');
}

