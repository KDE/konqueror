/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>

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

#ifndef _konq_misc_h
#define _konq_misc_h

#include "konqprivate_export.h"

#include <krun.h>
#include <kparts/browserextension.h>
class KonqMainWindow;
class KonqView;
class KStandardDirs;
class KSharedConfig;
template <typename T> class KSharedPtr;
typedef KSharedPtr<KSharedConfig> KSharedConfigPtr;

namespace KonqMisc // TODO rename to KonqGlobal?
{
    /**
     * Stop full-screen mode in all windows.
     */
    void abortFullScreenMode();

    /**
     * Create a new window with a single view, showing @p url, using @p args
     */
    KonqMainWindow * createSimpleWindow( const KUrl &url, const KParts::OpenUrlArguments &args,
                                         const KParts::BrowserArguments& browserArgs = KParts::BrowserArguments(),
                                         bool tempFile = false);

    /**
     * Create a new window for @p url using @p args and the appropriate profile for this URL.
     * @param forbidUseHTML internal. True when called by "Find Files"
     * @param openUrl If it is false, no url is openend in the new window. The url is used to guess the profile
     */
    KonqMainWindow * createNewWindow( const KUrl &url,
                                      const KParts::OpenUrlArguments &args = KParts::OpenUrlArguments(),
                                      const KParts::BrowserArguments& browserArgs = KParts::BrowserArguments(),
                                      bool forbidUseHTML = false,
                                      const QStringList &filesToSelect = QStringList(),
                                      bool tempFile = false,
                                      bool openUrl = true);

    /**
     * Create a new window from the profile defined by @p filename and @p path.
     * @param url an optional URL to open in this profile.
     * @param forbidUseHTML internal. True when called by "Find Files"
     * @param openUrl If false no url is opened
     */
    KonqMainWindow * createBrowserWindowFromProfile( const QString &path,
                                                     const QString &filename,
                                                     const KUrl &url = KUrl(),
                                                     const KParts::OpenUrlArguments &args = KParts::OpenUrlArguments(),
                                                     const KParts::BrowserArguments& browserArgs = KParts::BrowserArguments(),
                                                     bool forbidUseHTML = false,
                                                     const QStringList& filesToSelect = QStringList(),
                                                     bool tempFile = false,
                                                     bool openUrl = true);

    /**
     * Creates a new window from the history of a view, copies the history
     * @param view the History is copied from this view
     * @param steps Restore currentPos() + steps
     */
    KonqMainWindow * newWindowFromHistory( KonqView* view, int steps );

    /**
     * Applies the URI filters to @p url.
     *
     * @p parent is used in case of a message box.
     * @p _url to be filtered.
     * @p _path the absolute path to append to the url before filtering it.
     */
    QString konqFilteredURL( QWidget* /*parent*/, const QString& /*_url*/, const QString& _path = QString() );

    /**
     * Name of the default profile
     */
    QString defaultProfileName();

    /**
     * Path to the default profile
     */
    QString defaultProfilePath();

}

#endif
