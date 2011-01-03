/*
   Copyright (C) 2003 Antonio Larrosa <larrosa@kde.org>
   Copyright (C) 2008 Matthias Grimrath <maps4711@gmx.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _ARCHIVEDIALOG_H_
#define _ARCHIVEDIALOG_H_

#include <kdialog.h>

#include <qlinkedlist.h>

#include <dom/dom_core.h>
#include <dom/html_document.h>

#include "ui_archiveviewbase.h"

class QWidget;
class KHTMLPart;
class ArchiveViewBase;
class KUrl;
class KTar;
class QTextStream;

class ArchiveViewBase : public QWidget, public Ui::ArchiveViewBase
{
public:
    ArchiveViewBase( QWidget *parent ) : QWidget( parent ) {
        setupUi( this );
    }
};

/// Does all the hard work of downloading, manipulating and storing of
/// HTML files and inlined images, stylesheets ...
class ArchiveDialog : public KDialog
{
   Q_OBJECT
public:
   ArchiveDialog(QWidget *parent, const QString &targetFilename, KHTMLPart *part);
   ~ArchiveDialog();

   void archive();

protected:
    /// Holds attributes that are not #CDATA
    class NonCDataAttr : public QSet<QString> {
    public:
        NonCDataAttr();
    };

    static NonCDataAttr non_cdata_attr;

    KIO::Job *startDownload( const KUrl &url, KHTMLPart *part );

private:

    // Frame handling

    typedef QHash<QString, KHTMLPart *> Name2Part;
    typedef QHash<KUrl,    KHTMLPart *> URL2Part;

    struct PartFrameData {
        Name2Part framesWithName;
        URL2Part  framesWithURLOnly;
    };

    typedef QHash< KHTMLPart *, PartFrameData > FramesInPart;
    typedef QHash< QString, KHTMLPart * >       TarName2Part;
    typedef QHash< KHTMLPart *, QString >       Part2TarName;


    // Stylesheets

    typedef QHash< KUrl, DOM::CSSStyleSheet >            CSSURLSet;
    typedef QHash< QString, KUrl >                       RawHRef2FullURL;
    typedef QHash< DOM::CSSStyleSheet, RawHRef2FullURL > URLsInStyleSheet;
    typedef QHash< DOM::Element, RawHRef2FullURL >       URLsInStyleElement;
    typedef QHash< DOM::Node, DOM::CSSStyleSheet >       Node2StyleSheet;

    // Recursive parsing and processing

    /// Databag to hold information that is gathered during recursive traversal of the DOM tree
    struct RecurseData {
        KHTMLPart *const     part;
        QTextStream *const   textStream;
        PartFrameData *const partFrameData;
        DOM::HTMLDocument    document;
        bool                 baseSeen;

        RecurseData(KHTMLPart *_part, QTextStream *_textStream, PartFrameData *pfd);
    };

    struct DownloadInfo {
        QString tarName;
        KHTMLPart *part;

        DownloadInfo(const QString &_tarName = QString::null, KHTMLPart *_part = 0)
            : tarName(_tarName), part(_part) { }
    };

    typedef QMap< KUrl, DownloadInfo >     UrlTarMap;
    typedef QList< UrlTarMap::Iterator > DownloadList;

    struct AttrElem {
        QString name;
        QString value;

        AttrElem() { }
        AttrElem(const QString &_n, const QString &_v) : name(_n), value(_v) { }
    };
    typedef QLinkedList< AttrElem >  AttrList;

    /**
     * Looks for URL contained in attributes.
     */
    struct ExtractURLs {
        ExtractURLs(const QString &nodeName, const DOM::Element &element);

        AttrList attrList;              /// copy of the attribute of @p element
        AttrList::iterator absURL;      /// for links ala &lt;a href= ... &gt;
        AttrList::iterator transURL;    /// for embedded objects like &lt;img src=...&gt;, favicons, background-images...
        AttrList::iterator frameURL;    /// if @p element contains a frameURL
        AttrList::iterator frameName;   /// if it is frame tag with a name element
        AttrList::iterator cssURL;      /// for URLs that specify CSS
    };

private:
    void downloadObjects();
    void downloadStyleSheets();
    void saveWebpages();
    void finishedArchiving(bool tarerror);

    void endProgressInfo(bool error);

    void obtainURLs();
    void obtainURLsLower(KHTMLPart *part, int level);
    void obtainPartURLsLower(const DOM::Node &pNode, int level, RecurseData &data);
    void obtainStyleSheetURLsLower(DOM::CSSStyleSheet styleSheet, RecurseData &data);

    bool insertTranslateURL( const KUrl &fullURL, RecurseData &data );
    bool insertHRefFromStyleSheet( const QString &hrefRaw, RawHRef2FullURL &raw2full,
            const KUrl &fullURL, RecurseData &data );
    void parseStyleDeclaration(const KUrl &baseURL, DOM::CSSStyleDeclaration decl,
            RawHRef2FullURL &urls, RecurseData &data /*, bool verbose = false*/);


    bool saveTopFrame();
    bool saveFrame(KHTMLPart *part, int level);
    void saveHTMLPart(RecurseData &data);
    void saveHTMLPartLower(const DOM::Node &pNode, int indent, RecurseData &data);


    QString  extractCSSURL(const QString &text);
    QString &changeCSSURLs(QString &text, const RawHRef2FullURL &raw2full);


    static bool hasAttrWithValue(const DOM::Element &elem, const QString &attrName, const QString &attrValue);
    static bool hasChildNode(const DOM::Node &pNode, const QString &nodeName);
    static AttrList::Iterator getAttribute(AttrList &attrList, const QString &attr);




    /**
     * completes a potentially partial URL in a HTML document (like &lt;img href="...")
     * to a fully qualified one.
     *
     * It uses the URL of the document or the URL given in the &lt;base ...&gt;
     * element, depending on if and where a &ltbase ...&gt; appears on the document.
     *
     * Always use this method to get full URLs from href's or similiar.
     *
     * Suppose the URL of the webpage is http://host.nowhere/. The head looks like this
     * <pre>
     * &lt;head&gt;
     *   &lt;link rel="stylesheet" href="style1.css" type="text/css" /&gt;
     *   &lt;base href="http://some.place/" /&gt;
     *   &lt;link rel="stylesheet" href="style2.css" type="text/css" /&gt;
     * &lt;/head&gt;
     * </pre>
     *
     * The full URL of "style1.css" is http://host.nowhere/style1.css, whereas
     * "style2.css" will become http://some.place/style2.css
     *
     * @return fully qualified URL of @p partURL relative to the HTML document in @c data.part
     */
    static KUrl absoluteURL( const QString &partURL, RecurseData &data );

    /**
     * TODO KDE4 is this in KHTML function available now?
     * Functionality taken from khtml/css/csshelper.cpp:parseURL
     *
     * Filters a href in an element inside the HTML body.  This handles
     * quirks in browsers that filter out \\n, \\r in URLs.
     */
    static QString parseURL(const QString &rawurl);

    /**
     * Creates unique filenames to be used in the tar archive
     */
    QString uniqTarName(const QString &suggestion, KHTMLPart *part);

    /**
     * Taken from khtml/misc/loader.cpp DOCLOAD_SECCHECK
     *
     * Would be better on the public interface of KHTMLPart (or similiar)
     *
     * Checks if an embedded link like &lt;img src=&quot;...&quot; should be loaded
     */
    static bool urlCheckFailed(KHTMLPart *part, const KUrl &fullURL);

    /**
     * Escapes HTML characters. Does not forget " as @ref Qt::escape() does.
     */
    QString escapeHTML(QString in);


    /**
     * Adds a suffix that hints at the mimetypes if such a suffix is not
     * present already. If there is no such mimetype in the KDE database
     * @p filename is returned unchanged.
     * 'filename' -> 'filename.gif'
     * 'picture.jpg' -> 'picture.jpg'
     *
     * NOTE This function is rather slow
     */
    QString appendMimeTypeSuffix(QString filename, const QString &mimetype);

private:
    KHTMLPart *      m_top;

    FramesInPart     m_framesInPart;

    UrlTarMap           m_url2tar;
    TarName2Part        m_tarName2part;
    Part2TarName        m_part2tarName;
    CSSURLSet           m_cssURLs;
    URLsInStyleSheet    m_URLsInStyleSheet;
    URLsInStyleElement  m_URLsInStyleElement;
    Node2StyleSheet     m_topStyleSheets;

    KIO::Job *             m_job;
    CSSURLSet::Iterator    m_styleSheets_it;
    DownloadList           m_objects;
    DownloadList::Iterator m_objects_it;
    UrlTarMap::Iterator    m_dlurl2tar_it;

    int              m_uniqId;
    KTar *           m_tarBall;
    time_t           m_archiveTime;
    QString          m_filename;

    ArchiveViewBase *   m_widget;


private slots:
    void slotObjectFinished(KJob *job);
    void slotStyleSheetFinished(KJob *job);
    void slotButtonClicked(int button);
};


#endif // _ARCHIVEDIALOG_H_
