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

namespace KonqMisc
{
/**
 * Creates a new window from the history of a view, copies the history
 * @param view the History is copied from this view
 * @param steps Restore currentPos() + steps
 */
KonqMainWindow *newWindowFromHistory(KonqView *view, int steps);

/**
 * Applies the URI filters to @p url, and convert it to a QUrl.
 *
 * @p parent is used in case of a message box.
 * @p url to be filtered.
 * @p currentDirectory the directory to use, in case the url is relative.
 */
QUrl konqFilteredURL(KonqMainWindow *parent, const QString &url, const QUrl &currentDirectory = QUrl());

/**
* These are some helper functions to encode/decode session filenames. The
* problem here is that windows doesn't like files with ':' inside.
*/

QString encodeFilename(QString filename);

QString decodeFilename(QString filename);
}

#endif
