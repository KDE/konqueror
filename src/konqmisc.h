/* This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _konq_misc_h
#define _konq_misc_h

#include "konqprivate_export.h"

#include <QUrl>

#include <KParts/NavigationExtension>

class KonqMainWindow;
class KonqView;
class KUriFilterData;

/**
 * @brief Namespace for miscellaneous functions
 */
namespace KonqMisc
{
/**
 * @brief Creates a new window from the history of a view and copies the history
 * @param view the view to copy history from
 * @param steps the number of steps of the history element to copy relative to the
 * current one
 *
 * @return the new main window or `nullptr` if the window couldn't be created, for
 * example because there's no history entry at the position given by @p steps
 */
KonqMainWindow *newWindowFromHistory(KonqView *view, int steps);

/**
 * @brief Applies the URI filters to a string representing an URL, and convert it to a QUrl
 *
 * @p parent unused
 * @p url the URL to be filtered.
 * @p currentDirectory the directory to use to resolve relative URLs
 *
 * URLs with the `konqueror` or `data` schemes aren't filtered, as `KUriFilter`
 * doesn't know how to handle them.
 *
 * @return the filtered URL
 */
QUrl konqFilteredURL(KonqMainWindow *parent, const QString &url, const QUrl &currentDirectory = QUrl());

/**
 * @brief Helper function which returns an appropriate URL depending on the results of a call to KUriFilter::filterUri()
 *
 * @param filterSuccess the value returned by the call to KUriFilter::filterUri()
 * @param data the data object passed to KUriFilter::filterUri()
 * @return the URL returned by `data.uri()` if @p filterSuccess is `true` and the filtered URI is not an error and
 * an appropriate `error` URL otherwise
 */
QUrl urlFromURIFilterResult(bool filterSuccess, const KUriFilterData &data);

/**
* These are some helper functions to encode/decode session filenames. The
* problem here is that windows doesn't like files with ':' inside.
*/

/**
 * @brief Replaces all colons in the given string with underscores
 *
 * This is needed for session files on Windows which doesn't allow file names containing
 * colons.
 *
 * @warning This assumes that @p filename doesn't contain any underscore, otherwise
 * using decodeFilename() to recover the original filename won't work correctly.
 *
 * @param filename the original file name
 * @return a string equal to @p filename except that all colons (:) are converted
 * to underscores (_)
 */
QString encodeFilename(QString filename);


/**
 * @brief Replaces all underscores in the given string with colons
 *
 * This is needed for session files on Windows which doesn't allow file names containing
 * colons.
 *
 * @warning This function will only give the correct result if the string passed to
 * encodeFilename() didn't contain any underscore. This is because this function
 * replaces every underscore with a colon.
 *
 * @param filename the original file name
 * @return a string equal to @p filename except that all underscores (_) are converted
 * to colons (:)
 */
QString decodeFilename(QString filename);
}

#endif
