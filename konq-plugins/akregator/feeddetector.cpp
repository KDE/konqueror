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

#include <qregexp.h>
#include <qstring.h>
#include <qstringlist.h>
#include <kcharsets.h>

#include "feeddetector.h"
#include <kdebug.h>


using namespace Akregator;

FeedDetectorEntryList FeedDetector::extractFromLinkTags(const QString& s)
{
    //reduce all sequences of spaces, newlines etc. to one space:
    QString str = s.simplified();

    // extracts <link> tags
    QRegExp reLinkTag("<[\\s]?LINK[^>]*REL[\\s]?=[\\s]?\\\"[^\\\"]*(ALTERNATE|FEED|SERVICE\\.FEED)[^\\\"]*\\\"[^>]*>", Qt::CaseInsensitive);

    // extracts the URL (href="url")
    QRegExp reHref("HREF[\\s]?=[\\s]?\\\"([^\\\"]*)\\\"", Qt::CaseInsensitive);
    // extracts type attribute
    QRegExp reType("TYPE[\\s]?=[\\s]?\\\"([^\\\"]*)\\\"", Qt::CaseInsensitive);
    // extracts the title (title="title")
    QRegExp reTitle("TITLE[\\s]?=[\\s]?\\\"([^\\\"]*)\\\"", Qt::CaseInsensitive);

    int pos = 0;
    int matchpos = 0;

    // get all <link> tags
    QStringList linkTags;
    //int strlength = str.length();
    while ( matchpos != -1 )
    {
        matchpos = reLinkTag.indexIn(str, pos);
        if (matchpos != -1)
        {
            linkTags.append( str.mid(matchpos, reLinkTag.matchedLength()) );
            pos = matchpos + reLinkTag.matchedLength();
        }
    }

    FeedDetectorEntryList list;

    for ( QStringList::Iterator it = linkTags.begin(); it != linkTags.end(); ++it )
    {
        QString type;
        int pos = reType.indexIn(*it, 0);
        if (pos != -1)
            type = reType.cap(1).toLower();

        // we accept only type attributes indicating a feed
        if ( type != "application/rss+xml" && type != "application/rdf+xml"
	      && type != "application/atom+xml" && type != "application/xml" )
            continue;

        QString title;
        pos = reTitle.indexIn(*it, 0);
        if (pos != -1)
        title = reTitle.cap(1);

        title = KCharsets::resolveEntities(title);

        QString url;
        pos = reHref.indexIn(*it, 0);
        if (pos != -1)
            url = reHref.cap(1);

        url = KCharsets::resolveEntities(url);

        // if feed has no title, use the url as preliminary title (until feed is parsed)
        if ( title.isEmpty() )
            title = url;

        if ( !url.isEmpty() ) {
            kDebug() << "found feed:" << url << title;
            list.append(FeedDetectorEntry(url, title) );
        }
    }


    return list;
}

QStringList FeedDetector::extractBruteForce(const QString& s)
{
    QString str = s.simplified();

    QRegExp reAhrefTag("<[\\s]?A[^>]?HREF=[\\s]?\\\"[^\\\"]*\\\"[^>]*>", Qt::CaseInsensitive);

    // extracts the URL (href="url")
    QRegExp reHref("HREF[\\s]?=[\\s]?\\\"([^\\\"]*)\\\"", Qt::CaseInsensitive);

    QRegExp rssrdfxml(".*(RSS|RDF|XML)", Qt::CaseInsensitive);

    int pos = 0;
    int matchpos = 0;

    // get all <a href> tags and capture url
    QStringList list;
    //int strlength = str.length();
    while ( matchpos != -1 )
    {
        matchpos = reAhrefTag.indexIn(str, pos);
        if ( matchpos != -1 )
        {
            QString ahref = str.mid(matchpos, reAhrefTag.matchedLength());
            int hrefpos = reHref.indexIn(ahref, 0);
            if ( hrefpos != -1 )
            {
                QString url = reHref.cap(1);

                url = KCharsets::resolveEntities(url);

                if ( rssrdfxml.exactMatch(url) )
                    list.append(url);
            }

            pos = matchpos + reAhrefTag.matchedLength();
        }
    }

    return list;

}
