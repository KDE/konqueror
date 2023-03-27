/*
    This file is part of Akregator.

    SPDX-FileCopyrightText: 2004 Teemu Rytilahti <tpr@d5k.net>

    SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-exception
*/

#ifndef AKREGATORFEEDDETECTOR_H
#define AKREGATORFEEDDETECTOR_H

#include <QString>
#include <QList>

class QStringList;

namespace Akregator
{

class FeedDetectorEntry
{
public:
    FeedDetectorEntry() {}
    FeedDetectorEntry(const QString &url, const QString &title)
        : m_url(url), m_title(title) {}

    const QString &url() const
    {
        return m_url;
    }
    const QString &title() const
    {
        return m_title;
    }

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
    static FeedDetectorEntryList extractFromLinkTags(const QString &s);

    /** \brief searches an HTML page for slightly feed-like looking links and catches everything not running away quickly enough.
    Extracts links from @c <a @c href> tags which end with @c xml, @c rss or @c rdf
    @param s the html source to scan (the actual source, no URI)
    @return a list containing the detected feeds
    */
    static QStringList extractBruteForce(const QString &s);

private:
    FeedDetector() {}
};
}

#endif //AKREGATORFEEDDETECTOR_H
