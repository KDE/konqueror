/*
    This file is part of Akregator.

    SPDX-FileCopyrightText: 2004 Teemu Rytilahti <tpr@d5k.net>

    SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-exception
*/

#ifndef PLUGINUTIL_H
#define PLUGINUTIL_H

#include <QString>
#include <QStringList>

class QUrl;

namespace Akregator
{
namespace PluginUtil
{
    /**
     * Add a list of feeds to aKregator.
     *
     * This will be done via a DBus call if the application is running,
     * or by running it with a command line option if it is not.
     *
     * @param urls List of feed URLs to add
     */
    void addFeeds(const QStringList &urls);

    /**
     * Fix up a URL relative to a base URL.
     *
     * @param s The URL in string form
     * @param baseurl The base URL
     * @return The absolute resolved URL
     */
    QString fixRelativeURL(const QString &s, const QUrl &baseurl);
}
}
#endif
