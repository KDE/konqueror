/*
   SPDX-FileCopyrightText: 2008 Xavier Vello <xavier.vello@gmail.com>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef KIO_BOOKMARKS_H
#define KIO_BOOKMARKS_H

#include <KIO/WorkerBase>
#include <KBookmarkManager>
#include <KConfig>
#include <KConfigGroup>
#include <KImageCache>

class BookmarksProtocol : public KIO::WorkerBase
{
public:
    BookmarksProtocol( const QByteArray &pool, const QByteArray &app );
    ~BookmarksProtocol() override;

    KIO::WorkerResult get( const QUrl& url ) override;

private:
    int columns;
    int indent;
    int totalsize;
    KSharedPixmapCacheMixin< KSharedDataCache >* cache;
    KBookmarkManager* manager;
    KConfig* cfg;
    KConfigGroup config;
    KBookmarkGroup tree;
    void parseTree();
    void flattenTree( const KBookmarkGroup &folder );
    int sizeOfGroup(const KBookmarkGroup &folder, bool real = false);
    int addPlaces();

    // Defined in kde_bookmarks_html.cpp
    void echo( const QString &string );
    QString htmlColor(const QColor &col);
    QString htmlColor(const QBrush &brush);
    void echoIndex();
    void echoHead(const QString &redirect = QString());
    void echoStyle();
    void echoBookmark( const KBookmark &bm);
    void echoSeparator();
    void echoFolder( const KBookmarkGroup &folder );

    // Defined in kde_bookmarks_pixmap.cpp
    void echoImage( const QString &type, const QString &string, const QString &sizestring = QString());
};

#endif
