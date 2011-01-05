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
#include "konqopenurlrequest.h"
class KonqMainWindow;
class KonqView;
class KStandardDirs;
class KSharedConfig;
template <typename T> class KSharedPtr;
typedef KSharedPtr<KSharedConfig> KSharedConfigPtr;

namespace KonqMisc // TODO split into something like KonqWindowFactory or KonqWindowCreator, and KonqGlobal?
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
     * @param req Additional arguments, see KonqOpenURLRequest.
     * @param openUrl If it is false, no url is opened in the new window (and the aboutpage is not shown).
     * The url is used to guess the profile.
     */
    KONQ_TESTS_EXPORT KonqMainWindow * createNewWindow(const KUrl &url,
                                                       const KonqOpenURLRequest& req = KonqOpenURLRequest(),
                                                       bool openUrl = true);

    /**
     * Create a new window from the profile defined by @p profilePath and @p profileFilename.
     * @param profilePath full path to the profile definition, or leave it empty if from the usual profile directory
     * @param profileFilename the filename of the profile definition
     * @param url an optional URL to open in this profile
     * @param req Additional arguments, see KonqOpenURLRequest
     * @param openUrl If false no url is opened
     *
     * Note: the caller must call show()
     */
    KonqMainWindow * createBrowserWindowFromProfile(const QString& profilePath,
                                                    const QString& profileFilename,
                                                    const KUrl &url = KUrl(),
                                                    const KonqOpenURLRequest& req = KonqOpenURLRequest(),
                                                    bool openUrl = true);

    /**
     * Creates a new window from the history of a view, copies the history
     * @param view the History is copied from this view
     * @param steps Restore currentPos() + steps
     */
    KonqMainWindow * newWindowFromHistory( KonqView* view, int steps );

    /**
     * Applies the URI filters to @p url, and convert it to a KUrl.
     *
     * @p parent is used in case of a message box.
     * @p url to be filtered.
     * @p path the absolute path to use, in case the url is relative.
     */
    KUrl konqFilteredURL(KonqMainWindow* parent, const QString& url, const QString& path = QString());

    /**
     * Name of the default profile
     */
    QString defaultProfileName();

    /**
     * Path to the default profile
     */
    QString defaultProfilePath();

    /**
    * These are some helper functions to encode/decode session filenames. The
    * problem here is that windows doesn't like files with ':' inside.
    */

    QString encodeFilename(QString filename);

    QString decodeFilename(QString filename);
}

#endif
