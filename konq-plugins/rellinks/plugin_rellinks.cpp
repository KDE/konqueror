/***************************************************************************
 *   Copyright (C) 2002, Anders Lund <anders@alweb.dk>                     *
 *   Copyright (C) 2003, 2004, Franck Quélain <shift@free.fr>              *
 *   Copyright (C) 2004, Kevin Krammer <kevin.krammer@gmx.at>              *
 *   Copyright (C) 2004, 2006, Olivier Goffart <ogoffart @ kde.org>        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/




// Qt includes
#include <qapplication.h>
#include <qtimer.h>

// KDE include
#include <dom/dom_doc.h>
#include <dom/dom_element.h>
#include <dom/dom_string.h>
#include <dom/html_document.h>
#include <kaction.h>
#include <kdebug.h>
#include <kpluginfactory.h>
#include <khtml_part.h>
#include <khtmlview.h>
#include <kiconloader.h>
#include <kcomponentdata.h>
#include <klocale.h>
#include <kmenu.h>
#include <kshortcut.h>
#include <ktoolbar.h>
#include <kurl.h>
#include <kactionmenu.h>
#include <kactioncollection.h>
// local includes
#include "plugin_rellinks.h"
#include <kicon.h>

/** Rellinks factory */
K_PLUGIN_FACTORY(RelLinksFactory, registerPlugin<RelLinksPlugin>();)
#include <kaboutdata.h>
static const KAboutData aboutdata("rellinks", 0, ki18n("Rellinks") , "1.0" );
K_EXPORT_PLUGIN(RelLinksFactory(aboutdata) )

/** Constructor of the plugin. */
RelLinksPlugin::RelLinksPlugin(QObject *parent, const QVariantList &)
    : KParts::Plugin( parent ),
      m_part(0),
	  m_viewVisible(false)
{

    setComponentData(RelLinksFactory::componentData());

    KAction *a;
    // ------------- Navigation links --------------
    a =  actionCollection()->addAction(  "rellinks_top");
    a->setText(i18n("&Top"));
    a->setIcon(KIcon("go-top"));
    a->setShortcut(KShortcut("Ctrl+Alt+T"));
    a->setWhatsThis( i18n("<p>This link references a home page or the top of some hierarchy.</p>") );
    connect(a, SIGNAL(triggered()), this, SLOT(goHome()));
    kaction_map["home"] = a;

    a =actionCollection()->addAction(  "rellinks_up");
    a->setText(i18n("&Up"));
    a->setIcon(KIcon("go-up"));
    a->setShortcut(KShortcut("Ctrl+Alt+U"));
    a->setWhatsThis( i18n("<p>This link references the immediate parent of the current document.</p>") );
    connect(a, SIGNAL(triggered()), this, SLOT(goUp()));
    kaction_map["up"] = a;

    bool isRTL = QApplication::isRightToLeft();

    a = actionCollection()->addAction( "rellinks_first");
    a->setText(i18n("&First"));
    a->setIcon(KIcon(isRTL ? "go-last" : "go-first"));
    a->setShortcut(KShortcut("Ctrl+Alt+F"));
    a->setWhatsThis( i18n("<p>This link type tells search engines which document is considered by the author to be the starting point of the collection.</p>") );
    connect(a, SIGNAL(triggered()), this, SLOT(goFirst()));
    kaction_map["begin"] = a;

    a = actionCollection()->addAction(  "rellinks_previous");
    a->setText(i18n("&Previous"));
    a->setIcon(KIcon(isRTL ? "go-next" : "go-previous"));
    a->setShortcut(KShortcut("Ctrl+Alt+P"));
    connect(a, SIGNAL(triggered()), this, SLOT(goPrevious()));
    a->setWhatsThis( i18n("<p>This link references the previous document in an ordered series of documents.</p>") );
    kaction_map["prev"] = a;

    a = actionCollection()->addAction(  "rellinks_next");
    a->setText(i18n("&Next"));
    a->setIcon(KIcon(isRTL ? "go-previous" : "go-next"));
    a->setShortcut(KShortcut("Ctrl+Alt+N"));
    a->setWhatsThis( i18n("<p>This link references the next document in an ordered series of documents.</p>") );
    connect(a, SIGNAL(triggered()), this, SLOT(goNext()));
    kaction_map["next"] = a;

    a = actionCollection()->addAction(  "rellinks_last");
    a->setText(i18n("&Last"));
    a->setIcon(KIcon(isRTL ? "go-first" : "go-last"));
    a->setShortcut(KShortcut("Ctrl+Alt+L"));
    connect(a, SIGNAL(triggered()), this, SLOT(goLast()));
    a->setWhatsThis( i18n("<p>This link references the end of a sequence of documents.</p>") );
    kaction_map["last"] = a;

    // ------------ special items --------------------------
    a = actionCollection()->addAction( "rellinks_search");
    a->setText(i18n("&Search"));
    a->setIcon(KIcon("edit-find"));
    a->setShortcut(KShortcut("Ctrl+Alt+S"));
    a->setWhatsThis( i18n("<p>This link references the search.</p>") );
    connect(a, SIGNAL(triggered()), this, SLOT(goSearch()));
    kaction_map["search"] = a;

    // ------------ Document structure links ---------------
    m_document = new KActionMenu( KIcon("go-jump"),i18n("Document"), actionCollection());
    actionCollection()->addAction( "rellinks_document", m_document );
    m_document->setWhatsThis( i18n("<p>This menu contains the links referring the document information.</p>") );
    m_document->setDelayed(false);

    a = actionCollection()->addAction(  "rellinks_toc");
    a->setText(i18n("Table of &Contents"));
    a->setShortcut(KShortcut("Ctrl+Alt+C"));
    a->setWhatsThis( i18n("<p>This link references the table of contents.</p>") );
    connect(a, SIGNAL(triggered()), this,  SLOT(goContents()));
    kaction_map["contents"] = a;
    m_document->addAction(a);

    kactionmenu_map["chapter"] = new KActionMenu( i18n("Chapters"), actionCollection() );
    actionCollection()->addAction( "rellinks_chapters", kactionmenu_map["chapter" ] );

    m_document->addAction(kactionmenu_map["chapter"]);
    connect( kactionmenu_map["chapter"]->menu(), SIGNAL( activated( int ) ), this, SLOT(goChapter(int)));
    kactionmenu_map["chapter"]->setWhatsThis( i18n("<p>This menu references the chapters of the document.</p>") );
    kactionmenu_map["chapter"]->setDelayed(false);

    kactionmenu_map["section"] = new KActionMenu( i18n("Sections"), actionCollection() );
    actionCollection()->addAction( "rellinks_sections", kactionmenu_map["section"] );

    m_document->addAction(kactionmenu_map["section"]);

    connect( kactionmenu_map["section"]->menu(), SIGNAL( activated( int ) ), this, SLOT( goSection( int ) ) );
    kactionmenu_map["section"]->setWhatsThis( i18n("<p>This menu references the sections of the document.</p>") );
    kactionmenu_map["section"]->setDelayed(false);

    kactionmenu_map["subsection"] = new KActionMenu( i18n("Subsections"), actionCollection() );
    m_document->addAction(kactionmenu_map["subsection"]);
    actionCollection()->addAction( "rellinks_subsections", kactionmenu_map["subsection"] );

    connect( kactionmenu_map["subsection"]->menu(), SIGNAL( activated( int ) ), this, SLOT( goSubsection( int ) ) );
    kactionmenu_map["subsection"]->setWhatsThis( i18n("<p>This menu references the subsections of the document.</p>") );
    kactionmenu_map["subsection"]->setDelayed(false);

    kactionmenu_map["appendix"] = new KActionMenu( i18n("Appendix"), actionCollection() );
    actionCollection()->addAction( "rellinks_appendix", kactionmenu_map["appendix"] );

    m_document->addAction(kactionmenu_map["appendix"]);
    connect( kactionmenu_map["appendix"]->menu(), SIGNAL( activated( int ) ), this, SLOT( goAppendix( int ) ) );
    kactionmenu_map["appendix"]->setWhatsThis( i18n("<p>This link references the appendix.</p>") );
    kactionmenu_map["appendix"]->setDelayed(false);

    a = actionCollection()->addAction(  "rellinks_glossary");
    a->setText(i18n("&Glossary"));
    a->setShortcut(KShortcut("Ctrl+Alt+G"));
    connect(a, SIGNAL(triggered()), this, SLOT(goGlossary()));
    a->setWhatsThis( i18n("<p>This link references the glossary.</p>") );
    m_document->addAction(a);
    kaction_map["glossary"] = a;

    a = actionCollection()->addAction(  "rellinks_index");
    a->setText(i18n("&Index"));
    a->setShortcut(KShortcut("Ctrl+Alt+I"));
    a->setWhatsThis( i18n("<p>This link references the index.</p>") );
    connect(a, SIGNAL(triggered()), this, SLOT(goIndex()));
    m_document->addAction(a);
    kaction_map["index"] = a;

    // Other links
    m_more  = new KActionMenu( i18n("More"), actionCollection() );
    actionCollection()->addAction( "rellinks_more", m_more );
    m_more->setWhatsThis( i18n("<p>This menu contains other important links.</p>") );
    m_more->setDelayed(false);

    a = actionCollection()->addAction(  "rellinks_help");
    a->setText(i18n("&Help"));
    a->setIcon(KIcon("help-contents"));
    a->setShortcut(KShortcut("Ctrl+Alt+H"));
    a->setWhatsThis( i18n("<p>This link references the help.</p>") );
    connect(a, SIGNAL(triggered()), this, SLOT(goHelp()));
    m_more->addAction(a);
    kaction_map["help"] = a;

    a = actionCollection()->addAction(  "rellinks_authors");
    a->setText(i18n("&Authors"));
    a->setIcon(KIcon("x-office-contact"));
    a->setShortcut(KShortcut("Ctrl+Alt+A"));
    a->setWhatsThis( i18n("<p>This link references the author.</p>") );
    connect(a, SIGNAL(triggered()), this, SLOT(goAuthor()));
    m_more->addAction(a);
    kaction_map["author"] = a;

    a = actionCollection()->addAction(  "rellinks_copyright");
    a->setText(i18n("Copy&right"));
    a->setIcon(KIcon("help-about"));
    a->setShortcut(KShortcut("Ctrl+Alt+R"));
    a->setWhatsThis( i18n("<p>This link references the copyright.</p>") );
    connect(a, SIGNAL(triggered()), this, SLOT(goCopyright()));
    m_more->addAction(a);
    kaction_map["copyright"] = a;

    kactionmenu_map["bookmark"] = new KActionMenu( KIcon("bookmarks"),i18n("Bookmarks"), actionCollection() );
    actionCollection()->addAction( "rellinks_bookmarks", kactionmenu_map["bookmark"] );
    m_more->addAction(kactionmenu_map["bookmark"]);
    kactionmenu_map["bookmark"]->setWhatsThis( i18n("<p>This menu references the bookmarks.</p>") );
    connect( kactionmenu_map["bookmark"]->menu(), SIGNAL( activated( int ) ), this, SLOT( goBookmark( int ) ) );
    kactionmenu_map["bookmark"]->setDelayed(false);

    kactionmenu_map["alternate"] = new KActionMenu( i18n("Other Versions"), actionCollection() );
    actionCollection()->addAction( "rellinks_other_versions", kactionmenu_map["alternate"] );
    m_more->addAction(kactionmenu_map["alternate"]);
    kactionmenu_map["alternate"]->setWhatsThis( i18n("<p>This link references the alternate versions of this document.</p>") );
    connect( kactionmenu_map["alternate"]->menu(), SIGNAL( activated( int ) ), this, SLOT( goAlternate( int ) ) );
    kactionmenu_map["alternate"]->setDelayed(false);

    // Unclassified menu
    m_links = new KActionMenu( KIcon("rellinks"),i18n("Miscellaneous"), actionCollection());
    actionCollection()->addAction( "rellinks_links", m_links );
    kactionmenu_map["unclassified"] = m_links;
    kactionmenu_map["unclassified"]->setWhatsThis( i18n("<p>Miscellaneous links.</p>") );
    connect( kactionmenu_map["unclassified"]->menu(), SIGNAL( activated( int ) ), this, SLOT( goAllElements( int ) ) );
    kactionmenu_map["unclassified"]->setDelayed(false);

    // We unactivate all the possible actions
    disableAll();

    // When the rendering of the HTML is done, we update the site navigation bar
    m_part = qobject_cast<KHTMLPart *>(parent);
    if (!m_part)
        return;

    connect( m_part, SIGNAL( docCreated() ), this, SLOT( newDocument() ) );
    connect( m_part, SIGNAL( completed() ), this, SLOT( loadingFinished() ) );

    // create polling timer and connect it
    m_pollTimer = new QTimer(this);
    m_pollTimer->setObjectName( "polling timer");
    connect( m_pollTimer, SIGNAL( timeout() ), this, SLOT( updateToolbar() ) );

    // delay access to our part's members until it has finished its initialisation
    QTimer::singleShot(0, this, SLOT(delayedSetup()));
}

/** Destructor */
RelLinksPlugin::~RelLinksPlugin() {
}

bool RelLinksPlugin::eventFilter(QObject *watched, QEvent* event) {
    if (m_part == 0) return false;

    if (watched == 0 || event == 0) return false;

    if (watched == m_view)
    {
        switch (event->type())
        {
            case QEvent::Show:
                m_viewVisible = true;
                updateToolbar();
                break;

            case QEvent::Hide:
                m_viewVisible = false;
                updateToolbar();
                break;

            case QEvent::Close:
                m_pollTimer->stop();
                m_view->removeEventFilter(this);
                break;

            default:
                break;
        }
    }

    // we never filter an event, we just want to know about it
    return false;
}

void RelLinksPlugin::delayedSetup()
{
    if (m_part == 0) return;

    m_view = m_part->view();
    m_view->installEventFilter(this);
    m_viewVisible = m_view->isVisible();
}

void RelLinksPlugin::newDocument() {
    // start calling upateToolbar periodically to get the new links as soon as possible

    m_pollTimer->start(500);
    //kDebug(90210) << "newDocument()";

    updateToolbar();
}

void RelLinksPlugin::loadingFinished() {
    m_pollTimer->stop();
    //kDebug(90210) << "loadingFinished()";
    updateToolbar();
	guessRelations();
}

/* Update the site navigation bar */
void RelLinksPlugin::updateToolbar() {

    // If we have a part
    if (!m_part)
        return;

    // We disable all
    disableAll();

    // get a list of LINK nodes in document
    DOM::NodeList linkNodes = m_part->document().getElementsByTagName( "link" );

    //kDebug(90210) << "Rellinks: Link nodes =" << linkNodes.length();

    bool showBar = false;
    unsigned long nodeLength = linkNodes.length();

    for ( unsigned int i=0; i < nodeLength; i++ ) {
        // create a entry for each one
        DOM::Element e( linkNodes.item( i ) );


        // --- Retrieve of the relation type --

        QString rel = e.getAttribute( "rel" ).string();
        rel = rel.simplified();
        if (rel.isEmpty()) {
            // If the "rel" attribut is null then use the "rev" attribute...
            QString rev = e.getAttribute( "rev" ).string();
            rev = rev.simplified();
            if (rev.isEmpty()) {
                // if "rev" attribut is also empty => ignore
                continue;
            }
            // Determine the "rel" equivalent of "rev" type
            rel =  transformRevToRel(rev);
        }
        // Determin the name used internally
        QString lrel = getLinkType(rel.toLower());
        // relation to ignore
        if (lrel.isEmpty()) continue;
//	kDebug() << "lrel=" << lrel;

        // -- Retrieve of other useful information --

        QString href = e.getAttribute( "href" ).string();
        // if nowhere to go, ignore the link
        if (href.isEmpty()) continue;
        QString title = e.getAttribute( "title" ).string();
        QString hreflang = e.getAttribute( "hreflang" ).string();

        KUrl ref( m_part->url(), href );
        if ( title.isEmpty() )
            title = ref.prettyUrl();

        // escape ampersand before settings as action title, otherwise the menu entry will interpret it as an
        // accelerator
        title.replace('&', "&&");

        // -- Menus activation --

        // Activation of "Document" menu ?
        if (lrel == "contents" || lrel == "glossary" || lrel == "index" || lrel == "appendix") {
            m_document->setEnabled(true);
        }
        // Activation of "More" menu ?
        if (lrel == "help" || lrel == "author" || lrel == "copyright" ) {
            m_more->setEnabled(true);
        }

        // -- Buttons or menu items activation / creation --
        if (lrel == "bookmark" || lrel == "alternate") {
            int id = kactionmenu_map[lrel]->menu()->insertItem( title );
            m_more->setEnabled(true);
            kactionmenu_map[lrel]->setEnabled(true);
            element_map[lrel][id] = e;

        } else if (lrel == "appendix" || lrel == "chapter" || lrel == "section" || lrel == "subsection") {
            int id = kactionmenu_map[lrel]->menu()->insertItem( title );
            m_document->setEnabled(true);
            kactionmenu_map[lrel]->setEnabled(true);
            element_map[lrel][id] = e;

        } else {
            // It is a unique action
            element_map[lrel][0] = e;
            if (kaction_map[lrel]) {
                kaction_map[lrel]->setEnabled(true);
                // Tooltip
                if (hreflang.isEmpty()) {
                    kaction_map[lrel]->setToolTip( title );
                } else {
                    kaction_map[lrel]->setToolTip( title + " [" + hreflang + ']');
                }
            } else {
                // For the moment all the elements are reference in a separated menu
                // TODO : reference the unknown ?
                int id = kactionmenu_map["unclassified"]->menu()->insertItem( lrel + " : " + title );
                kactionmenu_map["unclassified"]->setEnabled(true);
                element_map["unclassified"][id] = e;
            }

        }

        showBar = true;
    }
}


void RelLinksPlugin::guessRelations()
{
	m_part = qobject_cast<KHTMLPart *>(parent());
	if (!m_part || m_part->document().isNull() )
		return;

	//If the page already contains some link, that mean the webmaster is aware
	//of the meaning of <link> so we can consider that if prev/next was possible
	//they are already there.
	if(!element_map.isEmpty())
		return;

	// - The number of didgit may not be more of 3, or this is certenly an id.
	// - We make sure that the number is followed by a dot, a &, or the end, we
	//   don't want to match stuff like that:   page.html?id=A14E12FD
	// - We make also sure the number is not preceded dirrectly by others number
	QRegExp rx("^(.*[=/?&][^=/?&.\\-0-9]*)([\\d]{1,3})([.&][^/0-9]{0,15})?$");


	const QString zeros("0000");
	QString url=m_part->url().url();
	if(rx.indexIn(url)!=-1)
	{
		uint val=rx.cap(2).toUInt();
		int lenval=rx.cap(2).length();
		QString nval_str=QString::number(val+1);
		//prepend by zeros if the original also contains zeros.
		if(nval_str.length() < lenval && rx.cap(2)[0]=='0')
			nval_str.prepend(zeros.left(lenval-nval_str.length()));

		QString href=rx.cap(1)+ nval_str + rx.cap(3);
		KUrl ref( m_part->url(), href );
		QString title = i18n("[Autodetected] %1", ref.prettyUrl());
		DOM::Element e= m_part->document().createElement("link");
		e.setAttribute("href",href);
		element_map["next"][0] = e;
		kaction_map["next"]->setEnabled(true);
		kaction_map["next"]->setToolTip( title );

		if(val>1)
		{
			nval_str=QString::number(val-1);
			if(nval_str.length() < lenval && rx.cap(2)[0]=='0')
				nval_str.prepend(zeros.left(lenval-nval_str.length()));
			QString href=rx.cap(1)+ nval_str + rx.cap(3);
			KUrl ref( m_part->url(), href );
			QString title = i18n("[Autodetected] %1", ref.prettyUrl());
			e= m_part->document().createElement("link");
			e.setAttribute("href",href);
			element_map["prev"][0] = e;
			kaction_map["prev"]->setEnabled(true);
			kaction_map["prev"]->setToolTip( title );
		}
	}
}


/** Menu links */
void RelLinksPlugin::goToLink(const QString & rel, int id) {
    // have the KHTML part open it
    KHTMLPart *part = qobject_cast<KHTMLPart *>(parent());
    if (!part)
        return;

    DOM::Element e = element_map[rel][id];
    QString href = e.getAttribute("href").string();
    KUrl url( part->url(), href );
    QString target = e.getAttribute("target").string();

    // URL arguments
    KParts::OpenUrlArguments arguments;
    KParts::BrowserArguments browserArguments;
    browserArguments.frameName = target;

    // Add base url if not valid
    if (url.isValid()) {
        part->browserExtension()->openUrlRequest(url, arguments, browserArguments);
    } else {
        KUrl baseURL = part->baseURL();
        QString endURL = url.prettyUrl();
        KUrl realURL = KUrl(baseURL, endURL);
        part->browserExtension()->openUrlRequest(realURL, arguments, browserArguments);
    }

}

void RelLinksPlugin::goHome() {
    goToLink("home");
}

void RelLinksPlugin::goUp() {
    goToLink("up");
}

void RelLinksPlugin::goFirst() {
    goToLink("begin");
}

void RelLinksPlugin::goPrevious() {
    goToLink("prev");
}

void RelLinksPlugin::goNext() {
    goToLink("next");
}

void RelLinksPlugin::goLast() {
    goToLink("last");
}

void RelLinksPlugin::goContents() {
    goToLink("contents");
}

void RelLinksPlugin::goIndex() {
    goToLink("index");
}

void RelLinksPlugin::goGlossary() {
    goToLink("glossary");
}

void RelLinksPlugin::goHelp() {
    goToLink("help");
}

void RelLinksPlugin::goSearch() {
    goToLink("search");
}

void RelLinksPlugin::goAuthor() {
    goToLink("author");
}


void RelLinksPlugin::goCopyright() {
    goToLink("copyright");
}

void RelLinksPlugin::goBookmark(int id) {
    goToLink("bookmark", id);
}

void RelLinksPlugin::goChapter(int id) {
    goToLink("chapter", id);
}

void RelLinksPlugin::goSection(int id) {
    goToLink("section", id);
}

void RelLinksPlugin::goSubsection(int id) {
    goToLink("subsection", id);
}

void RelLinksPlugin::goAppendix(int id) {
    goToLink("appendix", id);
}

void RelLinksPlugin::goAlternate(int id) {
    goToLink("alternate", id);
}

void RelLinksPlugin::goAllElements(int id) {
    goToLink("unclassified", id);
}

void RelLinksPlugin::disableAll() {
    element_map.clear();

    // Clear actions
    KActionMap::Iterator it;
    for ( it = kaction_map.begin(); it != kaction_map.end(); ++it ) {
        // If I don't test it crash :(
        if (it.value()) {
            it.value()->setEnabled(false);
            it.value()->setToolTip(it.value()->text().remove('&'));
        }
    }

    // Clear actions
    KActionMenuMap::Iterator itmenu;
    for ( itmenu = kactionmenu_map.begin(); itmenu != kactionmenu_map.end(); ++itmenu ) {
        // If I don't test it crash :(
        if (itmenu.value()) {
            itmenu.value()->menu()->clear();
            itmenu.value()->setEnabled(false);
            itmenu.value()->setToolTip(itmenu.value()->text().remove('&'));
        }
    }

    // Unactivate menus
    m_more->setEnabled(false);
    m_document->setEnabled(false);

}


QString RelLinksPlugin::getLinkType(const QString &lrel) {
    // Relations to ignore...
    if (lrel.contains("stylesheet")
          || lrel == "script"
          || lrel == "icon"
          || lrel == "shortcut icon"
          || lrel == "prefetch" )
        return QString();

    // ...known relations...
    if (lrel == "top" || lrel == "origin" || lrel == "start")
        return "home";
    if (lrel == "parent")
        return "up";
    if (lrel == "first")
        return "begin";
    if (lrel == "previous")
        return "prev";
    if (lrel == "child")
        return "next";
    if (lrel == "end")
        return "last";
    if (lrel == "toc")
        return "contents";
    if (lrel == "find")
        return "search";
    if (lrel == "alternative stylesheet")
        return "alternate stylesheet";
    if (lrel == "authors")
        return "author";
    if (lrel == "toc")
        return "contents";

    //...unknown relations or name that don't need to change
    return lrel;
}

QString RelLinksPlugin::transformRevToRel(const QString &rev) {
    QString altRev = getLinkType(rev);

    // Known relations
    if (altRev == "prev")
        return getLinkType("next");
    if (altRev == "next")
        return getLinkType("prev");
    if (altRev == "made")
        return getLinkType("author");
    if (altRev == "up")
        return getLinkType("child");
    if (altRev == "sibling")
        return getLinkType("sibling");

    //...unknown inverse relation => ignore for the moment
    return QString();
}

#include "plugin_rellinks.moc"
