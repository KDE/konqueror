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

#ifndef PLUGINBASE_H
#define PLUGINBASE_H

class QString;
class QStringList;
class KUrl;

namespace Akregator
{

class PluginBase
{
    public:
        PluginBase();
        ~PluginBase();
    
    public:
        /**
        * Tells you if aKregator is running.
        * @return true when aKregator is running
         */
        bool akregatorRunning();
        /**
         * Adds feed to aKregator via DBUS.
         */
        void addFeedsViaDBUS(const QStringList& urls);
        /**
         * Adds feed to aKregator via command line.
         */
        void addFeedViaCmdLine(const QString &url);
        QString fixRelativeURL(const QString &s, const KUrl &baseurl);
};

}
#endif
