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
 
#ifndef AKREGATORFEEDDETECTOR_H
#define AKREGATORFEEDDETECTOR_H

#include <qstring.h>
#include <QList>

class QStringList;

namespace Akregator
{

    class FeedDetectorEntry
    {
        public:
            FeedDetectorEntry() {}
            FeedDetectorEntry(const QString& url, const QString& title) 
                : m_url(url), m_title(title) {}

            const QString& url() const { return m_url; } 
            const QString& title() const { return m_title; }

        private:	
            QString m_url;
            QString m_title;
    };	

    typedef QList<FeedDetectorEntry> FeedDetectorEntryList; 

    /** a class providing functions to detect linked feeds in HTML sources */
    class FeedDetector
    {
        public:
            /** \brief searches an HTML page for feeds listed in @c <link> tags
            @c <link> tags with @c rel attribute values @c alternate or 
            @c service.feed are considered as feeds 
            @param s the html source to scan (the actual source, no URI)
            @return a list containing the detected feeds
            */
            static FeedDetectorEntryList extractFromLinkTags(const QString& s);

            /** \brief searches an HTML page for slightly feed-like looking links and catches everything not running away quickly enough. 
            Extracts links from @c <a @c href> tags which end with @c xml, @c rss or @c rdf
            @param s the html source to scan (the actual source, no URI)
            @return a list containing the detected feeds
            */
            static QStringList extractBruteForce(const QString& s);

        private:
            FeedDetector() {}
    };
}

#endif //AKREGATORFEEDDETECTOR_H
