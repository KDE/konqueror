/*
   Copyright (C) 2001 Andreas Schlapbach <schlpbch@iam.unibe.ch>
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

// The DOM-tree is recursed twice. The first run gathers all URLs while the second
// run writes out all HTML frames and CSS stylesheets. These two distinct runs are
// necessary, because some frames and/or stylesheets may be dropped (for example
// a frame currently not displayed or deemed insecure). In that case an URL that
// points to such a frame/stylesheet has to be removed. Since the URL may be mentioned
// earlier before recursing to the to-be-removed frame, two runs are necessary to get
// a complete list of URLs that should be archived.

// Changelog
// * replace dynamic_cast<> and ->inherits() with qobject_cast<>
// * use QHash instead of QMap; get rid of Ordered<> class
// * fixed crash / assertion on Konqueror exit after a webpage was archived
//   See comment about KHTMLView parent widget in plugin_webarchiver.cpp
// * Using KDE4/Qt4 KUrl::equals() and QUrl::fragment() to compare Urls
// * KHTML stores comment with a trailing '-'. Looks like some off-by-one bug.
// * Add mimetype indicating suffix to downloaded files.

// DONE CSS mentioned in <link> elements that are not parsed by Konqueror did not get their
//      href='' resolved/removed

// TODO if href= etc links in a frameset refer to frames currently displayed, make links relative
//      to archived page instead of absolute
// TODO KDE4 webarchiver: look at m_bPreserveWS
// TODO KDE4 webarchiver: look at closing tags
// TODO check if PartFrameData::framesWithName get a 'KHTMLPart *' if any
// TODO KHTMLPart::frames(): Is it possible to have NULL pointers in returned list?
// TODO If downloaded object need no data conversion, use KIO::file_copy or signal data()
// TODO KDE4 check what KHTMLPart is doing on job->addMetaData()
// TODO KDE4 use HTMLScriptElementImpl::charset() to get charset="" attribute of <link> elements


#include <cassert>

#include <qtextcodec.h>
#include <qtextdocument.h>

#include <kdebug.h>
#include <ktar.h>
#include <kglobal.h>
#include <kauthorized.h>
#include <kcharsets.h>
#include <kmimetype.h>
#include <kmessagebox.h>
#include <kstringhandler.h>
#include <kstandardguiitem.h>

#include <khtml_part.h>

#include <kio/job.h>

#include <dom/css_rule.h>
#include <dom/css_stylesheet.h>
#include <dom/css_value.h>

#include "archivedialog.h"

// Set to true if you have a patched http-io-slave that has
// improved offline-browsing functionality.
static const bool patchedHttpSlave = false;

#define CONTENT_TYPE "<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" />"


//
// Qt 4.x offers a @c foreach pseudo keyword. This is however slightly slower than FOR_ITER
// because @c foreach makes a shared copy of the container.
//
#define FOR_ITER(type,var,it) for (type::iterator it(var.begin()), it##end(var.end()); it != it##end; ++it)
#define FOR_CONST_ITER(type,var,it) for (type::const_iterator it(var.begin()), it##end(var.end()); it != it##end; ++it)
#define FOR_ITER_TEMPLATE(type,var,it) for (typename type::iterator it(var.begin()), it##end(var.end()); it != it##end; ++it)


static const mode_t archivePerms = S_IFREG | 0644;

typedef QList<KParts::ReadOnlyPart *> ROPartList;

//
// functions needed for storing certain DOM elements in a QHash<>
//
namespace DOM {

inline uint qHash(const CSSStyleSheet &a) {
    return ::qHash(static_cast<void *>(a.handle()));
}

inline bool operator==(const DOM::CSSStyleSheet &a, const DOM::CSSStyleSheet &b) {
    return a.handle() == b.handle();
}

inline uint qHash(const Node &a) {
    return ::qHash(static_cast<void *>(a.handle()));
}

}// namespace DOM


//
// elems with 'type' attr: object, param, link, script, style
//

// TODO convert to bsearch? probably more time and memory efficient
ArchiveDialog::NonCDataAttr::NonCDataAttr() {
    static const char * const non_cdata[] = {
        "id", "dir", "shape", "tabindex", "align", "nohref", "clear"
        // Unfinished...
    };
    for (int i=0; i!= (sizeof(non_cdata) / sizeof(non_cdata[0])); ++i)
        insert(non_cdata[i]);
}

// TODO lazy init?
ArchiveDialog::NonCDataAttr ArchiveDialog::non_cdata_attr;




ArchiveDialog::RecurseData::RecurseData(KHTMLPart *_part, QTextStream *_textStream, PartFrameData *pfd)
  : part(_part), textStream(_textStream), partFrameData(pfd), document(_part->htmlDocument()),
    baseSeen(false)
{
    Q_ASSERT( !document.isNull() );
}

static KHTMLPart *isArchivablePart(KParts::ReadOnlyPart *part)
{
    KHTMLPart *cp = qobject_cast<KHTMLPart *>(part);
    if (! cp)
        return NULL;
    DOM::HTMLDocument domdoc( cp->htmlDocument() );
    if (domdoc.isNull())
        return NULL;
    return cp;
}



ArchiveDialog::ArchiveDialog(QWidget *parent, const QString &filename, KHTMLPart *part)
  : KDialog(parent), m_top(part), m_job(NULL), m_uniqId(2), m_tarBall(NULL), m_filename(filename), m_widget(NULL)
{
    setCaption(i18nc("@title:window", "Web Archiver"));
    setButtons(KDialog::Ok | KDialog::Cancel);
    setButtonGuiItem(KDialog::Ok, KStandardGuiItem::close());
    setModal(false);
    enableButtonOk(false);
    setDefaultButton(KDialog::NoDefault);

    m_widget = new ArchiveViewBase(this);
    {
        QTreeWidgetItem *twi = m_widget->progressView->headerItem();
        twi->setText(0, i18n("Status"));
        twi->setText(1, i18n("Url"));
    }
    setMainWidget(m_widget);

    KUrl srcURL = part->url();
    m_widget->urlLabel->setText(QString("<a href=\"") + srcURL.url() + "\">" +
                                KStringHandler::csqueeze( srcURL.prettyUrl(), 80 ) + "</a>");
    m_widget->targetLabel->setText(QString("<a href=\"") + filename + "\">" +
                                   KStringHandler::csqueeze( filename, 80 ) + "</a>");

    //if(part->document().ownerDocument().isNull())
    //   m_document = part->document();
    //else
    //   m_document = part->document().ownerDocument();

    m_tarBall = new KTar(filename, "application/x-gzip");
    m_archiveTime = QDateTime::currentDateTime().toTime_t();
}

ArchiveDialog::~ArchiveDialog() {
    // TODO cancel outstanding download jobs?
    kDebug(90110) << "destroying";
    if (m_job) {
        m_job->kill();
        m_job = NULL;
    }
    delete m_tarBall; m_tarBall = NULL;
}



void ArchiveDialog::archive() {
    if (m_tarBall->open(QIODevice::WriteOnly)) {

        obtainURLs();

        // Assign unique tarname to URLs
        // Split m_url2tar into Stylesheets / non stylesheets
        m_objects.clear();
        assert(static_cast<ssize_t>(m_url2tar.size()) - static_cast<ssize_t>(m_cssURLs.size()) >= 0);
//         m_objects.reserve(m_url2tar.size() - m_cssURLs.size());
        FOR_ITER(UrlTarMap, m_url2tar, u2t_it) {
            const KUrl &url = u2t_it.key();
            DownloadInfo &info = u2t_it.value();

            assert( info.tarName.isNull() );
//             info.tarName = uniqTarName( url.fileName(), 0 );

            // To able to append mimetype hinting suffixes to tarnames, for instance adding '.gif' to a
            // webbug '87626734' adding the name to the url-to-tarname map is deferred.
            // This cannot be done with CSS because CSS may reference each other so when URLS
            // of the first CSS are changed all tarnames need to be there.
            //
            if ( m_cssURLs.find( url ) == m_cssURLs.end() ) {
                m_objects.append( u2t_it );
            } else {
                info.tarName = uniqTarName( url.fileName(), 0 );
            }
        }

        QProgressBar *pb = m_widget->progressBar;
        pb->setMaximum(m_url2tar.count() + 1);
        pb->setValue(0);

        m_objects_it = m_objects.begin();
        downloadObjects();

    } else {
        const QString title = i18nc( "@title:window", "Unable to Open Web-Archive" );
        const QString text = i18n( "Unable to open \n %1 \n for writing." ).arg(m_tarBall->fileName());
        KMessageBox::sorry(NULL, text, title);
    }
}

void ArchiveDialog::downloadObjects() {

    if ( m_objects_it == m_objects.end() ) {

        m_styleSheets_it = m_cssURLs.begin();
        downloadStyleSheets();

    } else {

        m_dlurl2tar_it = (*m_objects_it);
        const KUrl &url    = m_dlurl2tar_it.key();
        DownloadInfo &info = m_dlurl2tar_it.value();
        assert( m_dlurl2tar_it != m_url2tar.end() );

        Q_ASSERT(m_job == NULL);
        m_job = startDownload( url, info.part );
        connect(m_job, SIGNAL(result(KJob*)), SLOT(slotObjectFinished(KJob*)) );
    }
}

void ArchiveDialog::slotObjectFinished( KJob *_job ) {
    KIO::StoredTransferJob *job = qobject_cast<KIO::StoredTransferJob *>(_job);
    Q_ASSERT(job == m_job);
    m_job = NULL;
    const KUrl &url    = m_dlurl2tar_it.key();
    DownloadInfo &info = m_dlurl2tar_it.value();

    assert(info.tarName.isNull());
    bool error = job->error();
    if ( !error ) {
        const QString &mimetype( job->mimetype() );
        info.tarName = uniqTarName( appendMimeTypeSuffix(url.fileName(), mimetype), 0 );

        QByteArray data( job->data() );
        const QString &tarName = info.tarName;

//         kDebug(90110) << "downloaded " << url.prettyUrl() << "size=" << data.size() << "mimetype" << mimetype;
        error = ! m_tarBall->writeFile(tarName, QString::null, QString::null, data.data(), data.size(),
                                       archivePerms, m_archiveTime, m_archiveTime, m_archiveTime);
        if (error) {
            kDebug(90110) << "Error writing to archive file";
            finishedArchiving(true);
            return;
        }
    } else {
        info.tarName.clear();
        kDebug(90110) << "download error for url='" << url.prettyUrl();
    }

    endProgressInfo(error);
    ++m_objects_it;
    downloadObjects();
}


void ArchiveDialog::downloadStyleSheets() {
    if (m_styleSheets_it == m_cssURLs.end()) {

        saveWebpages();

    } else {

//         QTimer::singleShot(3000, this, SLOT(slotDownloadStyleSheetsDelay()));
        const KUrl &url = m_styleSheets_it.key();
        m_dlurl2tar_it = m_url2tar.find( url );
        assert( m_dlurl2tar_it != m_url2tar.end() );
        DownloadInfo &info = m_dlurl2tar_it.value();

        Q_ASSERT(m_job == NULL);
        m_job = startDownload( url, info.part );
        connect(m_job, SIGNAL(result(KJob*)), SLOT(slotStyleSheetFinished(KJob*)) );
    }
}



void ArchiveDialog::slotStyleSheetFinished( KJob *_job ) {
    KIO::StoredTransferJob *job = qobject_cast<KIO::StoredTransferJob *>(_job);
    Q_ASSERT(job == m_job);
    m_job = NULL;
    const KUrl &url    = m_dlurl2tar_it.key();
    DownloadInfo &info = m_dlurl2tar_it.value();

    bool error = job->error();
    if (! error) {
        QByteArray data( job->data() );
        const QString &tarName = info.tarName;

        URLsInStyleSheet::Iterator uss_it = m_URLsInStyleSheet.find( m_styleSheets_it.value() );
        assert( uss_it != m_URLsInStyleSheet.end() );

        DOM::DOMString ds( uss_it.key().charset() );
        QString cssCharSet( ds.string() );
        bool ok;
        QTextCodec *codec = KGlobal::charsets()->codecForName(cssCharSet, ok);
        kDebug(90110) << "translating URLs in CSS" << url.prettyUrl() << "charset=" << cssCharSet << " found=" << ok;
        assert( codec );
        QString css_text = codec->toUnicode( data );
        data.clear();
        // Do *NOT* delete 'codec'!  These are allocated by Qt

        changeCSSURLs( css_text, uss_it.value() );
        data = codec->fromUnicode( css_text );
        css_text.clear();

        error = ! m_tarBall->writeFile(tarName, QString::null, QString::null, data.data(), data.size(),
                                       archivePerms, m_archiveTime, m_archiveTime, m_archiveTime);
        if (error) {
            kDebug(90110) << "Error writing to archive file";
            finishedArchiving(true);
            return;
        }
    } else {
        info.tarName.clear();
        kDebug(90110) << "download error for css url='" << url.prettyUrl();
    }

    endProgressInfo(error);
    ++m_styleSheets_it;
    downloadStyleSheets();
}



KIO::Job *ArchiveDialog::startDownload( const KUrl &url, KHTMLPart *part ) {
    QTreeWidgetItem *twi = new QTreeWidgetItem;
    twi->setText(0, i18n("Downloading"));
    twi->setText(1, url.prettyUrl());
    QTreeWidget *tw = m_widget->progressView;
    tw->insertTopLevelItem(0, twi);

    KIO::Job *job = KIO::storedGet(url, KIO::NoReload, KIO::HideProgressInfo);

    // Use entry from cache only. Avoids re-downloading. Requires modified kio_http slave.
    job->addMetaData("cache", patchedHttpSlave ? "cacheonly" : "cache");

    // This is a duplication of the code in loader.cpp: Loader::servePendingRequests()

    //job->addMetaData("accept", req->object->accept());
    job->addMetaData( "referrer", part->url().url() );
    job->addMetaData( "cross-domain", part->toplevelURL().url() );

    return job;
}

void ArchiveDialog::endProgressInfo(bool error) {
    QTreeWidget *tw = m_widget->progressView;
    tw->topLevelItem(0)->setText(0, error ? i18n("Error") : i18n("OK"));
    QProgressBar *pb = m_widget->progressBar;
    pb->setValue(pb->value() + 1);
}






void ArchiveDialog::saveWebpages() {
    bool error = saveTopFrame();
    if (error) {
        kDebug(90110) << "Error writing to archive file";
        finishedArchiving(true);
        return;
    }
    QProgressBar *pb = m_widget->progressBar;
    pb->setValue(pb->value() + 1);

//     KMessageBox::information(0, i18n( "Archiving webpage completed." ), QString::null, QString::null, false);
    finishedArchiving(false);
}



void ArchiveDialog::finishedArchiving(bool tarerror) {
    if (tarerror) {
        KMessageBox::error(this, i18n("I/O error occurred while writing to web archive file %1.", m_tarBall->fileName()));
    }
    m_tarBall->close();

    m_widget->progressView->sortItems(0, Qt::AscendingOrder);
    setDefaultButton(KDialog::Ok);
    setEscapeButton(KDialog::Ok);
    enableButtonOk(true);
    enableButtonCancel(false);
}

void ArchiveDialog::slotButtonClicked(int) {
    deleteLater();  // Keep memory consumption low
}


// This is the mess you get because C++ lacks a lambda generator
//
// The whole purpose of the Get* classes is to parametrize what
// attribute of a KHTMLPart object should be fetched.
//
// GetName and GetURL are used for the 'class FuncObj' parameter
// class in the template function filterFrameMappings below
struct GetFromPart {
    const KHTMLPart *child;

    GetFromPart(const KHTMLPart *_child) : child(_child) { }
};

struct GetName : public GetFromPart {
    GetName(const KHTMLPart *child) : GetFromPart(child) { }

    operator QString () { return child->objectName(); }
};
struct GetURL : public GetFromPart {
    GetURL(const KHTMLPart *child) : GetFromPart(child) { }

    operator KUrl () { return child->url(); }
};

template< class Id2Part, class FuncObj >
static void filterFrameMappings(KHTMLPart *part, Id2Part &result) {
    Id2Part existing_frames;

    // TODO this can probably be optimized: no temp of existing, directly store to be removed parts.
    ROPartList childParts( part->frames() );
    FOR_ITER(ROPartList, childParts, child_it) {
        // TODO It is not clear from browsing the source code of KHTML if *child_it may be NULL
        Q_ASSERT(*child_it);
        KHTMLPart *cp = isArchivablePart(*child_it);
        if (cp) {
            existing_frames.insert( FuncObj(cp), cp );
        }
    }

    typedef QList< typename Id2Part::Iterator > IdRemoveList;
    IdRemoveList beRemoved;

    FOR_ITER_TEMPLATE(Id2Part, result, it) {
        typename Id2Part::Iterator exists_it = existing_frames.find( it.key() );
        if ( exists_it == existing_frames.end() )
            beRemoved.append( it );
        else
            it.value() = exists_it.value();
    }
    FOR_ITER_TEMPLATE(IdRemoveList, beRemoved, rem_it) {
        kDebug(90110) << "removing insecure(?) frame='" << (*rem_it).key();
        result.erase( (*rem_it) );
    }
}

template void filterFrameMappings< ArchiveDialog::Name2Part, GetName >(KHTMLPart *, ArchiveDialog::Name2Part &);
template void filterFrameMappings< ArchiveDialog::URL2Part, GetURL >(KHTMLPart *, ArchiveDialog::URL2Part &);




/**
 * Recursively traverses the DOM-Tree extracting all URLs that need to be downloaded
 */
void ArchiveDialog::obtainURLs() {
    m_url2tar.clear();
    m_tarName2part.clear();
    m_framesInPart.clear();
    m_cssURLs.clear();
    m_URLsInStyleSheet.clear();
    m_URLsInStyleElement.clear();
    m_topStyleSheets.clear();

    obtainURLsLower(m_top, 0);

    FOR_ITER(FramesInPart, m_framesInPart, fip_it) {
        KHTMLPart     *part = fip_it.key();
        PartFrameData &pfd  = fip_it.value();

        // Remove all frames obtained from the DOM tree parse
        // that do not have a corresponding KHTMLPart as a direct child.

        // Do NOT use KHTMLPart::findFrame()! This one searches recursively all subframes as well!
        filterFrameMappings< Name2Part, GetName >(part, pfd.framesWithName);
        filterFrameMappings< URL2Part, GetURL >(part, pfd.framesWithURLOnly);
   }
   assert(! m_framesInPart.empty());
#if 0
    FOR_ITER(CSSURLSet, m_cssURLs, it) {
        kDebug(90110) << "to be downloaded stylesheet='" << it.key();
    }
    FOR_ITER(URLsInStyleSheet, m_URLsInStyleSheet, ss2u_it) {
        kDebug(90110) << "raw URLs in sheet='" << ss2u_it.key().href();
        FOR_ITER(RawHRef2FullURL, ss2u_it.data(), c2f_it) {
            kDebug(90110) << "   url='" << c2f_it.key() << "' -> '" << c2f_it.data().prettyUrl();
        }
    }
    FOR_ITER(URLsInStyleElement, m_URLsInStyleElement, e2u_it) {
        kDebug(90110) << "raw URLs in style-element:";
        FOR_ITER(RawHRef2FullURL, e2u_it.data(), c2f_it) {
            kDebug(90110) << "   url='" << c2f_it.key() << "' -> '" << c2f_it.data().prettyUrl();
        }
    }
#endif
}


void ArchiveDialog::obtainStyleSheetURLsLower(DOM::CSSStyleSheet css, RecurseData &data) {

    //kDebug(90110) << "stylesheet title='" << styleSheet.title().string() << "' "
    //                "type='" << styleSheet.type().string();

    RawHRef2FullURL &raw2full = m_URLsInStyleSheet.insert( css, RawHRef2FullURL()).value();

    DOM::CSSRuleList crl = css.cssRules();
    for (int j=0; j != static_cast<int>(crl.length()); ++j) {

        DOM::CSSRule cr = crl.item(j);
        switch (cr.type()) {

        case DOM::CSSRule::STYLE_RULE: {
            const DOM::CSSStyleRule &csr = static_cast<DOM::CSSStyleRule &>(cr);

            //kDebug(90110) << "found selector '" << csr.selectorText();
            parseStyleDeclaration( css.baseUrl(), csr.style(), raw2full, data );
        } break;

        case DOM::CSSRule::IMPORT_RULE: {
            const DOM::CSSImportRule &cir = static_cast<DOM::CSSImportRule &>(cr);

            DOM::CSSStyleSheet importSheet = cir.styleSheet();
            if ( importSheet.isNull() ) {

                // Given stylesheet was not downloaded / parsed by KHTML
                // Remove that URL from the stylesheet
                kDebug(90110) << "stylesheet: invalid @import url('" << cir.href() << "')";

                raw2full.insert( cir.href().string(), KUrl() );

            } else {

                kDebug(90110) << "stylesheet: @import url('" << cir.href() << "') found";

                QString href = cir.href().string();
                Q_ASSERT( !href.isNull() );

                KUrl fullURL  = importSheet.baseUrl();
                bool inserted = insertHRefFromStyleSheet( href, raw2full, fullURL, data );
                if ( inserted ) {
                    m_cssURLs.insert( fullURL, importSheet );
                    obtainStyleSheetURLsLower( importSheet, data );
                }
            }
        } break;

        default:
            kDebug(90110) << " unknown/unsupported rule=" << cr.type();
        }
    }
}


void ArchiveDialog::obtainURLsLower(KHTMLPart *part, int level) {
    //QString indent;
    //indent.fill(' ', level*2);

    QString htmlFileName = (level == 0) ? "index.html" : part->url().fileName();

    // Add .html extension if not found already.  This works around problems with frames,
    // where the frame is for example "framead.php".  The http-io-slave gets the mimetype
    // from the webserver, but files in a tar archive do not have such metadata.  The result
    // is that Konqueror asks "save 'adframe.php' to file?" without this measure.
    htmlFileName = appendMimeTypeSuffix(htmlFileName, "text/html");

    // If level == 0, the m_tarName2part map is empty and so uniqTarName will return "index.html" unchanged.
    uniqTarName( htmlFileName, part );

    assert( m_framesInPart.find(part) == m_framesInPart.end() );
    FramesInPart::Iterator fip_it = m_framesInPart.insert( part, PartFrameData() );

    RecurseData data(part, 0, &(fip_it.value()));
    data.document.documentElement();
    obtainPartURLsLower(data.document.documentElement(), 1, data);
    {   // Limit lifetime of @c childParts
        ROPartList childParts( part->frames() );
        FOR_ITER(ROPartList, childParts, child_it) {
            KHTMLPart *cp = isArchivablePart(*child_it);
            if (cp) {
                obtainURLsLower(cp, level+1);
            }
        }
    }

    DOM::StyleSheetList styleSheetList = data.document.styleSheets();
    //kDebug(90110) << "# of stylesheets=" << styleSheetList.length();
    for (int i=0; i != static_cast<int>(styleSheetList.length()); ++i) {
        DOM::StyleSheet ss = styleSheetList.item(i);
        if ( ss.isCSSStyleSheet() ) {
            DOM::CSSStyleSheet &css = static_cast<DOM::CSSStyleSheet &>(ss);

            QString href = css.href().string();
            if (! href.isNull()) {
                QString href  = css.href().string();
                KUrl fullUrl  = css.baseUrl();
                kDebug(90110) << "top-level stylesheet='" << href;
                bool inserted = insertTranslateURL( fullUrl, data);
                if ( inserted )
                    m_cssURLs.insert( fullUrl, css );
            } else {
                DOM::Node node = css.ownerNode();
                if (! node.isNull()) {
                    assert(! m_topStyleSheets.contains(node));
                    kDebug(90110) << "top-level inline stylesheet '" << node.nodeName();
                    // TODO I think there can be more than one <style> area...
                    assert(href.isNull());
                    m_topStyleSheets.insert(node, css);

                } else {
                    kDebug(90110) << "found loose style sheet '" << node.nodeName();
                    assert(0); // FIXME for testing only
                }
            }
            obtainStyleSheetURLsLower( css, data );
        }
    }
}


void ArchiveDialog::obtainPartURLsLower(const DOM::Node &pNode, int level, RecurseData &data) {
    const QString nodeName = pNode.nodeName().string().toUpper();

    QString indent;
    indent.fill(' ', level*2);

    if ( !pNode.isNull() && (pNode.nodeType() == DOM::Node::ELEMENT_NODE) ) {
        const DOM::Element &element = static_cast<const DOM::Element &>(pNode);

        if ( const_cast<DOM::Element &>(element).hasAttribute( "STYLE" ) ) {
            RawHRef2FullURL &raw2full = m_URLsInStyleElement.insert(element, RawHRef2FullURL()).value();
            parseStyleDeclaration( data.part->url(), const_cast<DOM::Element &>(element).style(),
                                   raw2full, data );
        }

        if ( nodeName == "BASE" )
            data.baseSeen = true;

        ExtractURLs eurls(nodeName, element);
        const AttrList::iterator invalid = eurls.attrList.end();

        if (eurls.frameName != invalid) {

            // If a frame tag has a name tag, the src attribute will be overwritten
            // This ensures the current selected frame is saved and not the default
            // frame given by the original 'src' attribute
            data.partFrameData->framesWithName.insert((*eurls.frameName).value, 0);

        } else if (eurls.frameURL != invalid) {

            // URL has no 'name' attribute.  This frame cannot(?) change, so 'src' should
            // identify it unambigously
            KUrl _frameURL = absoluteURL((*eurls.frameURL).value, data );
            if (!urlCheckFailed(data.part, _frameURL))
                data.partFrameData->framesWithURLOnly.insert(_frameURL.url(), 0);

        } else {
            // Ignore empty frame tags
        }

        if (eurls.transURL != invalid) {
            // Kills insecure/invalid links. Frames are treated separately.
            insertTranslateURL(absoluteURL(parseURL((*eurls.transURL).value), data), data);
        }

        // StyleSheet-URLs are compared against the internal stylesheets data structures
        // Treatment is similar to frames
    }

    if (! pNode.isNull()) {
        DOM::Node child = pNode.firstChild();
        while (! child.isNull()) {
            obtainPartURLsLower(child, level+1, data);
            child = child.nextSibling();
        }
    }
}




// Kill insecure/invalid links. Frames are treated separately.

bool ArchiveDialog::insertTranslateURL( const KUrl &fullURL, RecurseData &data ) {
    if ( !urlCheckFailed(data.part, fullURL) ) {
//         kDebug(90110) << "adding '" << fullURL << "' to to-be-downloaded URLs";
        m_url2tar.insert( fullURL, DownloadInfo( QString::null, data.part ) );
        return true;
    } else {
        kDebug(90110) << "URL check failed on '" << fullURL.prettyUrl() << "' -- skipping";
        return false;
    }
}

bool ArchiveDialog::insertHRefFromStyleSheet( const QString &hrefRaw, RawHRef2FullURL &raw2full,
    const KUrl &fullURL, RecurseData &data )
{
    bool inserted = insertTranslateURL( fullURL, data );

#if 0
    if ( inserted ) {
        kDebug(90110) << "stylesheet: found url='"
                       << fullURL.prettyUrl() << "' hrefRaw='" << hrefRaw;
    } else {
        kDebug(90110) << "stylesheet: killing insecure/invalid url='"
                       << fullURL.prettyUrl() << "' hrefRaw='" << hrefRaw;
    }
#endif

    raw2full.insert( hrefRaw, inserted ? fullURL : KUrl() );
    return inserted;
}

void ArchiveDialog::parseStyleDeclaration(const KUrl &baseURL, DOM::CSSStyleDeclaration decl,
    RawHRef2FullURL &raw2full, RecurseData &data /*, bool verbose*/)
{
    for (int k=0; k != static_cast<int>(decl.length()); ++k) {
        DOM::DOMString item  = decl.item(k);
        DOM::DOMString val   = decl.getPropertyValue(item);
        //DOM::CSSValue  csval = decl.getPropertyCSSValue(item);

//         kDebug(90110) << "style declaration " << item << ":" << val << ";";

        QString href = extractCSSURL( val.string() );
        if ( href != QString::null ) {

//             kDebug(90110) << "URL in CSS " << item << ":" << val << ";";

            // TODO Would like to use khtml::parseURL to remove \r, \n and similar
            QString parsedURL = parseURL(href);

//             kDebug(90110) << "found URL='" << val << "' extracted='" << parsedURL << "'";
            insertHRefFromStyleSheet( href, raw2full, KUrl( baseURL, parsedURL ), data );
        }
    }
}







/* Saves all frames, starting from top */

bool ArchiveDialog::saveTopFrame() {
    m_part2tarName.clear();

    FOR_ITER(TarName2Part, m_tarName2part, t2p_it) {
        if ( t2p_it.value() != 0 )
            m_part2tarName.insert( t2p_it.value(), t2p_it.key() );
    }

    return saveFrame(m_top, 0);
}


bool ArchiveDialog::saveFrame(KHTMLPart *part, int level) {

    // Rebuild HTML file from 'part' and write to tar archive

    QByteArray rawtext;
    {
        FramesInPart::Iterator fip_it = m_framesInPart.find(part);
        assert( fip_it != m_framesInPart.end() );
        PartFrameData *pfd = &(fip_it.value());

        //
        // Overloading madness: Note the @c &rawtext : If you accidently write @c rawtext
        // it still compiles but it uses a different ctor that does not write to @c rawtext
        // but initializes @c textStream with @c rawtext
        //
        QTextStream textStream( &rawtext, QIODevice::WriteOnly );
        textStream.setCodec( QTextCodec::codecForMib( 106 ));    // 106 == UTF-8
        RecurseData data(part, &textStream, pfd);
        saveHTMLPart(data);
    } // @c textStream destroyed and flushed

    Part2TarName::Iterator p2tn_it = m_part2tarName.find( part );
    assert( p2tn_it != m_part2tarName.end() );
    const QString &tarName = p2tn_it.value();

    kDebug(90110) << "writing part='" << part->url().prettyUrl() << "' to tarfile='" << tarName
                  << "' size=" << rawtext.size();
    bool error = ! m_tarBall->writeFile(tarName, QString::null, QString::null, rawtext.data(), rawtext.size(),
                                        archivePerms, m_archiveTime, m_archiveTime, m_archiveTime);
    if (error) {
        return true;
    }


    // Recursively handle all frames / subparts
    {   // Limit lifetime of @c childParts
        ROPartList childParts( part->frames() );
        FOR_ITER(ROPartList, childParts, child_it) {
            KHTMLPart *cp = isArchivablePart(*child_it);
            if (cp) {
                error = saveFrame(cp, level+1);
                if (error) {
                    return true;
                }
            }
        }
    }

    return false;
}


// Saves the frame given in @c data.part

void ArchiveDialog::saveHTMLPart(RecurseData &data)
{
    QTextStream &textStream(*data.textStream);
    // Add a doctype
    DOM::DocumentType t( data.document.doctype() );
    if (! t.isNull()) {
        DOM::DOMString name( t.name() );
        DOM::DOMString publicId( t.publicId() );

        if (!name.isEmpty() && !publicId.isEmpty()) {
            textStream << "<!DOCTYPE " << name.string() << " PUBLIC \"" << publicId.string() << "\"";
            DOM::DOMString systemId( t.systemId() );
            if (!systemId.isEmpty())
                textStream << " \"" << systemId.string() << "\"";
            textStream << ">\n";
        }
    }

    textStream << "<!-- saved from: " << data.part->url().prettyUrl() << " -->\n";

    try {
        saveHTMLPartLower(data.document.documentElement(), 1, data);
    } catch (...) {
        kDebug(90110) << "exception";
        Q_ASSERT(0);
    }
}


void ArchiveDialog::saveHTMLPartLower(const DOM::Node &pNode, int level, RecurseData &data)
{
    const QString nodeName(pNode.nodeName().string().toUpper());

    //QString indent;
    //indent.fill(' ', level*2);

    bool skipElement   = false;
    bool fullEmptyTags = false;
    bool hasChildren   = const_cast<DOM::Node &>(pNode).hasChildNodes();
    QString text = "";

    bool isElement = !pNode.isNull() && (pNode.nodeType() == DOM::Node::ELEMENT_NODE);

    //kDebug(90110) << indent << "nodeName=" << nodeName << " toString()='" << pNode.toString() << "'";
    if ( isElement ) {
        const DOM::Element &element = static_cast<const DOM::Element &>(pNode);
        URLsInStyleElement::Iterator style_it = m_URLsInStyleElement.find( element );
        bool hasStyle = ( style_it != m_URLsInStyleElement.end() );

        if ((nodeName == "META") && hasAttrWithValue(element, "HTTP-EQUIV", "CONTENT-TYPE")) {
            // Skip content-type meta tag, we provide our own.
            skipElement = true;
        } else if ((nodeName == "NOFRAMES") && !hasChildren) {
            skipElement = true;
        } else {

            // translate URLs of stylesheets, jscript, images ...

            ExtractURLs eurls(nodeName, element);

            AttrList::Iterator filterOut1 = eurls.attrList.end();
            AttrList::Iterator filterOut2 = eurls.attrList.end();
            const AttrList::Iterator invalid = eurls.attrList.end();


            // make URLs in hyperref links absolute
            if (eurls.absURL != invalid) {
                KUrl baseurl = absoluteURL( "", data );
                KUrl newurl = KUrl(baseurl, parseURL((*eurls.absURL).value));
                if (urlCheckFailed(data.part, newurl)) {
                    (*eurls.absURL).value = "";
                    kDebug(90110) << "removing invalid/insecure href='" << newurl.prettyUrl() << "'";
                } else {
                    //
                    // KUrl::htmlRef() calls internally fragment()->toPercent()->toLatin1()->fromLatin1()->fromPercent()
                    // This is slow of course and there would be only a difference if there is some suburl.
                    // Since we discard any urls with suburls for security reasons QUrl::fragment() is sufficient.
                    //
                    assert(! newurl.hasSubUrl());   // @see urlCheckFailed()
                    if (newurl.hasFragment() && baseurl.equals(newurl, KUrl::CompareWithoutFragment)) {
                        (*eurls.absURL).value = QString("#") + newurl.fragment();
                    } else {
                        (*eurls.absURL).value = newurl.url();
                    }
                }
            }

            // make URLs of embedded objects local to tarfile
            if (eurls.transURL != invalid) {
                // NOTE This is a bit inefficient, because the URL is computed twice, here and when obtaining all
                // URLs first. However it is necessary, because two URLs that look different in the HTML frames (for
                // example absolute and relative) may resolve to the same absolute URL
                KUrl fullURL = absoluteURL( parseURL((*eurls.transURL).value), data );
                UrlTarMap::Iterator it = m_url2tar.find(fullURL);
                if (it == m_url2tar.end()) {

                    (*eurls.transURL).value = "";
                    kDebug(90110) << "removing invalid/insecure link='" << fullURL.prettyUrl() << "'";

                } else {
//                     assert( !it.value().tarName.isNull() );
                    (*eurls.transURL).value = it.value().tarName;
                }
            }

            // Check stylesheet <link>s
            if (eurls.cssURL != invalid) {

                KUrl fullURL = absoluteURL( (*eurls.cssURL).value, data );
                UrlTarMap::Iterator it = m_url2tar.find(fullURL);

                if ( it == m_url2tar.end() ) {

                    kDebug(90110) << "removing invalid/insecure CSS link='" << fullURL.prettyUrl() << "'";
                    (*eurls.cssURL).value = "";

                } else {
//                     assert( !it.value().tarName.isNull() );
                    (*eurls.cssURL).value = it.value().tarName;
                }
            }

            // Check for a frame with a name
            if (eurls.frameName != invalid) {
                Name2Part &n2f = data.partFrameData->framesWithName;
                Name2Part::Iterator n2f_part = n2f.find((*eurls.frameName).value);

                if (n2f_part == n2f.end()) {

                    // KHTML ignores this frame tag, so remove it here
                    filterOut1 = eurls.frameName;
                    filterOut2 = eurls.frameURL;

                    kDebug(90110) << "emptying frame=" << (*eurls.frameName).value;

                } else {

                    // Always add a 'src' attribute.  If it's not there, add one
                    if (eurls.frameURL == invalid) {
                        eurls.attrList.prepend(AttrElem(QString("src"), QString::null));
                        eurls.frameURL = eurls.attrList.begin();

                        // NOTE Now that we changed the list, pray the older iterators of 'attrList' still work...
                    }
                    Part2TarName::Iterator p2tn_it = m_part2tarName.find( n2f_part.value() );
                    Q_ASSERT( p2tn_it != m_part2tarName.end() );
                    (*eurls.frameURL).value = p2tn_it.value();

                    kDebug(90110) << "setting frame='" << (*eurls.frameName).value << "' to src='"
                                  << (*eurls.frameURL).value;
                }

            } else if (eurls.frameURL != invalid) {

                URL2Part &u2f = data.partFrameData->framesWithURLOnly;
                KUrl fullURL = absoluteURL( (*eurls.frameURL).value, data );
                URL2Part::Iterator u2f_part = u2f.find( fullURL );

                if (u2f_part == u2f.end()) {

                    // KHTML ignores this frame tag, so remove it here
                    filterOut1 = eurls.frameURL;

                    kDebug(90110) << "emptying frame='" << (*eurls.frameURL).value << "'";

                } else {

                    Part2TarName::Iterator p2tn_it = m_part2tarName.find( u2f_part.value() );
                    Q_ASSERT( p2tn_it != m_part2tarName.end() );
                    (*eurls.frameURL).value = p2tn_it.value();

                    kDebug(90110) << "setting frame='" << fullURL.prettyUrl() << "' to src='"
                                  <<  (*eurls.frameURL).value;
                }
            }


            // Remove <base href=... > attribute
            if (nodeName == "BASE") {
                filterOut1 = getAttribute( eurls.attrList, "href" );
                data.baseSeen = true;
            }


            // Insert <head> tag if not found
            if (nodeName == "HTML") {
                if (!hasChildNode(pNode, "HEAD"))
                    text += "<head>" CONTENT_TYPE "</head>\n";
                fullEmptyTags = true;
                // Always write out full closing tags for some tags
            } else if (nodeName == "HEAD" || nodeName == "FRAME" || nodeName == "IFRAME" || nodeName == "A" ||
                       nodeName == "DIV" || nodeName == "SPAN")
            {
                fullEmptyTags = true;
            }

            text += "<" + nodeName.toLower();

            // Write attributes
            for (AttrList::ConstIterator i = eurls.attrList.begin(); i != eurls.attrList.end(); ++i) {
                QString attr  = (*i).name.toLower();
                QString value = (*i).value;
                if ((i != filterOut1) && (i != filterOut2)) {
                    if (hasStyle && (attr == "style")) {
//                         kDebug(90110) << "translating URLs in element:";
//                         kDebug(90110) << "value=" << value;
                        changeCSSURLs( value, style_it.value() );
//                         kDebug(90110) << "value=" << value;
                    }
                    if (non_cdata_attr.find(attr) == non_cdata_attr.end()) {
                        value = escapeHTML(value);
                    }
                    text += " " + attr + "=\"" + value + "\"";
                }
            }

            // Take care for self-contained tags like <hr />.  This code is needed to close such
            // tags later with '/>'. 'fullEmptyTags == true' means to always write an explicit
            // closing tag, e.g. <script></script>
            if (fullEmptyTags || hasChildren)
                text += ">";

            if (nodeName == "HEAD") {
                text += CONTENT_TYPE "\n";
            }
        }
    } else {
        const QString &nodeValue(pNode.nodeValue().string());
        if (!(nodeValue.isEmpty())) {
            // Don't escape < > in JS or CSS
            DOM::Node parentNode = pNode.parentNode();
            QString parentNodeName = parentNode.nodeName().string().toUpper();
            if (parentNodeName == "STYLE") {
                text = pNode.nodeValue().string(); //analyzeInternalCSS(baseURL, pNode.nodeValue().string());

                Node2StyleSheet::Iterator topcss_it = m_topStyleSheets.find(parentNode);
                if ( topcss_it != m_topStyleSheets.end() ) {
                    URLsInStyleSheet::ConstIterator uss_it = m_URLsInStyleSheet.constFind( *topcss_it );
                    m_topStyleSheets.erase(topcss_it);  // for safety
                    assert( uss_it != m_URLsInStyleSheet.constEnd() );

                    kDebug(90110) << "translating URLs in <style> area.";
                    changeCSSURLs(text, uss_it.value() );

                } else {
                    kDebug(90110) << "found style area '" << nodeName << "', but KHMTL didn't feel like parsing it";
                }

            } else if (parentNodeName == "SCRIPT") {
                text = pNode.nodeValue().string();
            } else {
                if (pNode.nodeType() == DOM::Node::COMMENT_NODE) {
                    text = "<!--";
                    text += Qt::escape(nodeValue);  // No need to escape " as well
                    text += "-->";
                } else {
                    text = escapeHTML(nodeValue);
                }
            }
        }
    }


    (*data.textStream) << text;


    if (! pNode.isNull()) {
        DOM::Node child = pNode.firstChild();
        while (! child.isNull()) {
            saveHTMLPartLower(child, level+1, data);
            child = child.nextSibling();
        }
    }

    if (isElement && !skipElement) {
        if (fullEmptyTags || hasChildren) {
            text = "</" + nodeName.toLower() + ">";
        } else {
            text = " />"; // close self-contained tags
        }
        (*data.textStream) << text;
    }
}





QString ArchiveDialog::extractCSSURL(const QString &text) {
    if (text.startsWith("url(") && text.endsWith(")")) {
        return text.mid( 4, text.length() - 5 );
    } else {
        return QString::null;
    }
}


QString &ArchiveDialog::changeCSSURLs(QString &text, const RawHRef2FullURL &raw2full) {
    FOR_CONST_ITER(RawHRef2FullURL, raw2full, r2f_it) {
        const QString &raw  = r2f_it.key();
        const KUrl &fullURL = r2f_it.value();
        if (fullURL.isValid()) {
            UrlTarMap::Iterator utm_it = m_url2tar.find(fullURL);
            if (utm_it != m_url2tar.end() ) {
                const QString &tarName = utm_it.value().tarName;
//                 assert(! tarName.isNull());

                kDebug(90110) << "changeCSSURLs: url=" << raw << " -> " << tarName;
                text.replace( raw, tarName );
            } else {
                kDebug(90110) << "changeCSSURLs: raw URL not found in tar map";
                text.replace( raw, "" );
            }
        } else {
            kDebug(90110) << "changeCSSURLs: emptying invalid raw URL";
            text.replace( raw, "" );
        }
    }
    return text;
}







ArchiveDialog::ExtractURLs::ExtractURLs(const QString &nodeName, const DOM::Element &element) {

    DOM::NamedNodeMap attrs = element.attributes();
    int lmap = static_cast<int>(attrs.length());    // More than 2^31 attributes? hardly...
    for (int j = 0; j != lmap; ++j) {
        DOM::Attr attr = static_cast<DOM::Attr>(attrs.item(j));
        attrList.append( AttrElem(attr.name().string(), attr.value().string()) );
    }

    AttrList::Iterator rel        = attrList.end();
    AttrList::Iterator href       = attrList.end();
    AttrList::Iterator src        = attrList.end();
    AttrList::Iterator name       = attrList.end();
    AttrList::Iterator background = attrList.end();
    AttrList::Iterator invalid    = attrList.end();
    for (AttrList::Iterator i = attrList.begin(); i != attrList.end(); ++i) {
        QString attrName  = (*i).name.toUpper();
        if (attrName == "REL")
            rel = i;
        else if (attrName == "HREF")
            href = i;
        else if (attrName == "BACKGROUND")
            background = i;
        else if (attrName == "SRC")
            src = i;
        else if (attrName == "NAME")
            name = i;
    }

    //
    // Check attributes
    //
    transURL  =
    absURL    =
    frameURL  =
    frameName =
    cssURL    = attrList.end();
    if ((nodeName == "A") && (href != invalid)) {
        absURL = href;
    } else if ((nodeName == "LINK") && (rel != invalid) && (href != invalid)) {
        QString relUp = (*rel).value.toUpper();
        if (relUp == "STYLESHEET") {
            cssURL = href;
        } else if (relUp == "SHORTCUT ICON") {
            transURL = href;
        } else {
            absURL = href;
     }
    } else if (nodeName == "FRAME" || nodeName == "IFRAME") {
        if (src != invalid)
            frameURL = src;
        if (name != invalid)
            frameName = name;
    } else if ( (nodeName == "IMG" || nodeName == "INPUT" || nodeName == "SCRIPT") && (src != invalid) ) {
        transURL = src;
    } else if ( (nodeName == "BODY" || nodeName == "TABLE" || nodeName == "TH" || nodeName == "TD") &&
                (background != invalid))
    {
        kDebug() << "found background URL " << (*background).value;
        transURL = background;
    }
}


bool ArchiveDialog::hasAttrWithValue(const DOM::Element &elem, const QString &attrName, const QString &attrValue)
{
    DOM::Attr attr = const_cast<DOM::Element &>(elem).getAttributeNode( attrName );

    if ( !attr.isNull() ) {
        return attr.value().string().toUpper() == attrValue;
    } else
        return false;
}


bool ArchiveDialog::hasChildNode(const DOM::Node &pNode, const QString &nodeName)
{
    DOM::Node child;
    try {
        // We might throw a DOM exception
        child = pNode.firstChild();
    } catch (...) {
        // No children, stop recursion here
        child = DOM::Node();
   }

    while(!child.isNull()) {
        if (child.nodeName().string().toUpper() == nodeName)
            return true;
        child = child.nextSibling();
    }
    return false;
}


ArchiveDialog::AttrList::Iterator ArchiveDialog::getAttribute(AttrList &attrList, const QString &attr) {
    FOR_ITER(AttrList, attrList, it) {
        if ( (*it).name == attr )
            return it;
   }
    return attrList.end();
}









KUrl ArchiveDialog::absoluteURL( const QString &partURL, RecurseData &data ) {
    if ( data.baseSeen ) {
        return KUrl( data.document.completeURL( partURL ).string() );
    } else {
        return KUrl( data.part->url(), partURL );
    }
}


// TODO Should be khtml::parseURL
QString ArchiveDialog::parseURL(const QString &rawurl) {
    QString result = rawurl;
    return result.replace( QRegExp( "[\\x0000-\\x000D]" ), "" );
}


QString ArchiveDialog::uniqTarName(const QString &suggestion, KHTMLPart *part) {

    QString result = suggestion;

    // Name clash -> add unique id
    while (result.isEmpty() || m_tarName2part.contains(result))
        result = QString::number(m_uniqId++) + suggestion;
    m_tarName2part.insert( result, part );

    return result;
}

bool ArchiveDialog::urlCheckFailed(KHTMLPart *part, const KUrl &fullURL) {
    if (!fullURL.isValid())
        return true;
//     kDebug() << fullURL.prettyUrl() << " hasSubURL()=" << fullURL.hasSubUrl();
    if (fullURL.hasSubUrl())
        return true;

    QString prot = fullURL.protocol();
    bool protFile = (prot == "file");
    if (part->onlyLocalReferences() && !protFile)
        return true;

    bool protHttp = (prot == "http") || (prot == "https");
    if (!protFile && !protHttp)
        return true;

    if (! KAuthorized::authorizeUrlAction("redirect", part->url(), fullURL) ||
        ! KAuthorized::authorizeUrlAction("open", part->url(), fullURL))
    {
        return true;
    }

    return false;
}


QString ArchiveDialog::escapeHTML(QString in) {
    return Qt::escape(in).replace('"', "&quot;");
}

QString ArchiveDialog::appendMimeTypeSuffix(QString filename, const QString &mimetype) {
    KMimeType::Ptr mimeType = KMimeType::mimeType(mimetype, KMimeType::ResolveAliases);
    if (mimeType.isNull() || (mimeType == KMimeType::defaultMimeTypePtr())) {
        kDebug(90110) << "mimetype" << mimetype << "unknown here, returning unchanged";
        return filename;
    }
    const QStringList &patterns = mimeType->patterns();
    FOR_CONST_ITER(QStringList, patterns, pat_it) {
        // Lets hope all patterns are '*.xxx'
        QString suffix( *pat_it );
        int pos = suffix.lastIndexOf('*');
        if (pos < 0) {
            kDebug(90110) << "Illegal mime pattern '" << suffix << "for" << mimeType;
            Q_ASSERT(0);
            continue;
        }
        suffix = suffix.mid(pos + 1);
        if (filename.endsWith(suffix, Qt::CaseInsensitive)) {
//             kDebug(90110) << filename << "has already good suffix" << suffix;
            return filename;    // already has good suffix
        }
    }
    //
    // @c filename has no known suffix, append one
    //
    if (! patterns.isEmpty()) {
        QString suffix( *patterns.constBegin() );
        suffix.replace('*', QString::null);
        filename += suffix;
        kDebug(90110) << "appended missing mimetype suffix, returning" << filename;
    } else {
        kDebug(90110) << "mimetype" << mimetype << " has no pattern list, this is bad";
        Q_ASSERT(0);
    }
    return filename;
}

#include "archivedialog.moc"
