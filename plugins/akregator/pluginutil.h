/*
    This file is part of Akregator.

    Copyright (C) 2004 Teemu Rytilahti <tpr@d5k.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#ifndef PLUGINUTIL_H
#define PLUGINUTIL_H

class QString;
class QStringList;
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
