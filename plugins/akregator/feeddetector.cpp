/*
    This file is part of Akregator.

    SPDX-FileCopyrightText: 2004 Teemu Rytilahti <tpr@d5k.net>

    SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-exception
*/

#include "feeddetector.h"
#include <QRegExp>
#include <QString>
#include <QStringList>

#include <KCharsets>

#include "akregatorplugindebug.h"

using namespace Akregator;

FeedDetectorEntryList FeedDetector::extractFromLinkTags(const QString &s)
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
    while (matchpos != -1) {
        matchpos = reLinkTag.indexIn(str, pos);
        if (matchpos != -1) {
            linkTags.append(str.mid(matchpos, reLinkTag.matchedLength()));
            pos = matchpos + reLinkTag.matchedLength();
        }
    }

    FeedDetectorEntryList list;

    for (QStringList::Iterator it = linkTags.begin(); it != linkTags.end(); ++it) {
        QString type;
        int pos = reType.indexIn(*it, 0);
        if (pos != -1) {
            type = reType.cap(1).toLower();
        }

        // we accept only type attributes indicating a feed
        if (type != QLatin1String("application/rss+xml") && type != QLatin1String("application/rdf+xml")
                && type != QLatin1String("application/atom+xml") && type != QLatin1String("application/xml")) {
            continue;
        }

        QString title;
        pos = reTitle.indexIn(*it, 0);
        if (pos != -1) {
            title = reTitle.cap(1);
        }

        title = KCharsets::resolveEntities(title);

        QString url;
        pos = reHref.indexIn(*it, 0);
        if (pos != -1) {
            url = reHref.cap(1);
        }

        url = KCharsets::resolveEntities(url);

        // if feed has no title, use the url as preliminary title (until feed is parsed)
        if (title.isEmpty()) {
            title = url;
        }

        if (!url.isEmpty()) {
            qCDebug(AKREGATORPLUGIN_LOG) << "found feed:" << url << title;
            list.append(FeedDetectorEntry(url, title));
        }
    }

    return list;
}

QStringList FeedDetector::extractBruteForce(const QString &s)
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
    while (matchpos != -1) {
        matchpos = reAhrefTag.indexIn(str, pos);
        if (matchpos != -1) {
            QString ahref = str.mid(matchpos, reAhrefTag.matchedLength());
            int hrefpos = reHref.indexIn(ahref, 0);
            if (hrefpos != -1) {
                QString url = reHref.cap(1);

                url = KCharsets::resolveEntities(url);

                if (rssrdfxml.exactMatch(url)) {
                    list.append(url);
                }
            }

            pos = matchpos + reAhrefTag.matchedLength();
        }
    }

    return list;

}
