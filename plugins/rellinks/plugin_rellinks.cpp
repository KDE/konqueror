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

// local includes
#include "plugin_rellinks.h"

// Qt includes
#include <QApplication>
#include <QTimer>
#include <QKeySequence>
#include <QActionGroup>

// KDE include
#include <dom/dom_doc.h>
#include <dom/dom_element.h>
#include <dom/dom_string.h>
#include <dom/html_document.h>

#include <kpluginfactory.h>
#include <khtml_part.h>
#include <khtmlview.h>
#include <kiconloader.h>
#include <KLocalizedString>
#include <QMenu>
#include <ktoolbar.h>
#include <kactionmenu.h>
#include <kactioncollection.h>

/** Rellinks factory */
K_PLUGIN_FACTORY(RelLinksFactory, registerPlugin<RelLinksPlugin>();)
#include <kaboutdata.h>

static QUrl resolvedUrl(const QUrl &base, const QString& rel) {
    return QUrl(base).resolved(QUrl(rel));
}

Q_DECLARE_METATYPE(DOM::Element);

/** Constructor of the plugin. */
RelLinksPlugin::RelLinksPlugin(QObject *parent, const QVariantList &)
    : KParts::Plugin(parent),
      m_part(nullptr),
      m_viewVisible(false),
      m_linksFound(false)
{
    setComponentData(KAboutData(QStringLiteral("rellinks"), i18n("Rellinks"), QStringLiteral("1.0")));

    QActionGroup *grp = new QActionGroup(this);
    connect(grp, SIGNAL(triggered(QAction*)), this, SLOT(actionTriggered(QAction*)));

    QAction *a;
    // ------------- Navigation links --------------
    a =  actionCollection()->addAction(QStringLiteral("rellinks_home"));
    a->setText(i18n("&Top"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("go-top")));
    actionCollection()->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+Alt+T")));
    a->setWhatsThis(i18n("<p>This link references a home page or the top of some hierarchy.</p>"));
    grp->addAction(a);

    a = actionCollection()->addAction(QStringLiteral("rellinks_up"));
    a->setText(i18n("&Up"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("go-up")));
    actionCollection()->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+Alt+U")));
    a->setWhatsThis(i18n("<p>This link references the immediate parent of the current document.</p>"));
    grp->addAction(a);

    bool isRTL = QApplication::isRightToLeft();

    a = actionCollection()->addAction(QStringLiteral("rellinks_begin"));
    a->setText(i18n("&First"));
    a->setIcon(QIcon::fromTheme(isRTL ? "go-last" : "go-first"));
    actionCollection()->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+Alt+F")));
    a->setWhatsThis(i18n("<p>This link type tells search engines which document is considered by the author to be the starting point of the collection.</p>"));
    grp->addAction(a);

    a = actionCollection()->addAction(QStringLiteral("rellinks_prev"));
    a->setText(i18n("&Previous"));
    a->setIcon(QIcon::fromTheme(isRTL ? "go-next" : "go-previous"));
    actionCollection()->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+Alt+P")));
    a->setWhatsThis(i18n("<p>This link references the previous document in an ordered series of documents.</p>"));
    grp->addAction(a);

    a = actionCollection()->addAction(QStringLiteral("rellinks_next"));
    a->setText(i18n("&Next"));
    a->setIcon(QIcon::fromTheme(isRTL ? "go-previous" : "go-next"));
    actionCollection()->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+Alt+N")));
    a->setWhatsThis(i18n("<p>This link references the next document in an ordered series of documents.</p>"));
    grp->addAction(a);

    a = actionCollection()->addAction(QStringLiteral("rellinks_last"));
    a->setText(i18n("&Last"));
    a->setIcon(QIcon::fromTheme(isRTL ? "go-first" : "go-last"));
    actionCollection()->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+Alt+L")));
    a->setWhatsThis(i18n("<p>This link references the end of a sequence of documents.</p>"));
    grp->addAction(a);

    // ------------ special items --------------------------
    a = actionCollection()->addAction(QStringLiteral("rellinks_search"));
    a->setText(i18n("&Search"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("edit-find")));
    actionCollection()->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+Alt+S")));
    a->setWhatsThis(i18n("<p>This link references the search.</p>"));
    grp->addAction(a);

    // ------------ Document structure links ---------------
    m_document = new KActionMenu(QIcon::fromTheme(QStringLiteral("go-jump")), i18n("Document"), actionCollection());
    actionCollection()->addAction(QStringLiteral("rellinks_document"), m_document);
    m_document->setWhatsThis(i18n("<p>This menu contains the links referring the document information.</p>"));
    m_document->setDelayed(false);

    a = actionCollection()->addAction(QStringLiteral("rellinks_contents"));
    a->setText(i18n("Table of &Contents"));
    actionCollection()->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+Alt+C")));
    a->setWhatsThis(i18n("<p>This link references the table of contents.</p>"));
    m_document->addAction(a);
    grp->addAction(a);

    kactionmenu_map[QStringLiteral("chapter")] = new KActionMenu(i18n("Chapters"), actionCollection());
    actionCollection()->addAction(QStringLiteral("rellinks_chapters"), kactionmenu_map[QStringLiteral("chapter") ]);

    m_document->addAction(kactionmenu_map[QStringLiteral("chapter")]);
    connect(kactionmenu_map[QStringLiteral("chapter")]->menu(), &QMenu::triggered, this, &RelLinksPlugin::actionTriggered);
    kactionmenu_map[QStringLiteral("chapter")]->setWhatsThis(i18n("<p>This menu references the chapters of the document.</p>"));
    kactionmenu_map[QStringLiteral("chapter")]->setDelayed(false);

    kactionmenu_map[QStringLiteral("section")] = new KActionMenu(i18n("Sections"), actionCollection());
    actionCollection()->addAction(QStringLiteral("rellinks_sections"), kactionmenu_map[QStringLiteral("section")]);

    m_document->addAction(kactionmenu_map[QStringLiteral("section")]);

    connect(kactionmenu_map[QStringLiteral("section")]->menu(), &QMenu::triggered, this, &RelLinksPlugin::actionTriggered);
    kactionmenu_map[QStringLiteral("section")]->setWhatsThis(i18n("<p>This menu references the sections of the document.</p>"));
    kactionmenu_map[QStringLiteral("section")]->setDelayed(false);

    kactionmenu_map[QStringLiteral("subsection")] = new KActionMenu(i18n("Subsections"), actionCollection());
    m_document->addAction(kactionmenu_map[QStringLiteral("subsection")]);
    actionCollection()->addAction(QStringLiteral("rellinks_subsections"), kactionmenu_map[QStringLiteral("subsection")]);

    connect(kactionmenu_map[QStringLiteral("subsection")]->menu(), &QMenu::triggered, this, &RelLinksPlugin::actionTriggered);
    kactionmenu_map[QStringLiteral("subsection")]->setWhatsThis(i18n("<p>This menu references the subsections of the document.</p>"));
    kactionmenu_map[QStringLiteral("subsection")]->setDelayed(false);

    kactionmenu_map[QStringLiteral("appendix")] = new KActionMenu(i18n("Appendix"), actionCollection());
    actionCollection()->addAction(QStringLiteral("rellinks_appendix"), kactionmenu_map[QStringLiteral("appendix")]);

    m_document->addAction(kactionmenu_map[QStringLiteral("appendix")]);
    connect(kactionmenu_map[QStringLiteral("appendix")]->menu(), &QMenu::triggered, this, &RelLinksPlugin::actionTriggered);
    kactionmenu_map[QStringLiteral("appendix")]->setWhatsThis(i18n("<p>This link references the appendix.</p>"));
    kactionmenu_map[QStringLiteral("appendix")]->setDelayed(false);

    a = actionCollection()->addAction(QStringLiteral("rellinks_glossary"));
    a->setText(i18n("&Glossary"));
    actionCollection()->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+Alt+G")));
    a->setWhatsThis(i18n("<p>This link references the glossary.</p>"));
    m_document->addAction(a);
    grp->addAction(a);

    a = actionCollection()->addAction(QStringLiteral("rellinks_index"));
    a->setText(i18n("&Index"));
    actionCollection()->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+Alt+I")));
    a->setWhatsThis(i18n("<p>This link references the index.</p>"));
    m_document->addAction(a);
    grp->addAction(a);

    // Other links
    m_more  = new KActionMenu(i18n("More"), actionCollection());
    actionCollection()->addAction(QStringLiteral("rellinks_more"), m_more);
    m_more->setWhatsThis(i18n("<p>This menu contains other important links.</p>"));
    m_more->setDelayed(false);

    a = actionCollection()->addAction(QStringLiteral("rellinks_help"));
    a->setText(i18n("&Help"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("help-contents")));
    actionCollection()->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+Alt+H")));
    a->setWhatsThis(i18n("<p>This link references the help.</p>"));
    m_more->addAction(a);
    grp->addAction(a);

    a = actionCollection()->addAction(QStringLiteral("rellinks_author"));
    a->setText(i18n("&Authors"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("x-office-contact")));
    actionCollection()->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+Alt+A")));
    a->setWhatsThis(i18n("<p>This link references the author.</p>"));
    m_more->addAction(a);
    grp->addAction(a);

    a = actionCollection()->addAction(QStringLiteral("rellinks_copyright"));
    a->setText(i18n("Copy&right"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("help-about")));
    actionCollection()->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+Alt+R")));
    a->setWhatsThis(i18n("<p>This link references the copyright.</p>"));
    m_more->addAction(a);
    grp->addAction(a);

    kactionmenu_map[QStringLiteral("bookmark")] = new KActionMenu(QIcon::fromTheme(QStringLiteral("bookmarks")), i18n("Bookmarks"), actionCollection());
    actionCollection()->addAction(QStringLiteral("rellinks_bookmarks"), kactionmenu_map[QStringLiteral("bookmark")]);
    m_more->addAction(kactionmenu_map[QStringLiteral("bookmark")]);
    kactionmenu_map[QStringLiteral("bookmark")]->setWhatsThis(i18n("<p>This menu references the bookmarks.</p>"));
    connect(kactionmenu_map[QStringLiteral("bookmark")]->menu(), &QMenu::triggered, this, &RelLinksPlugin::actionTriggered);
    kactionmenu_map[QStringLiteral("bookmark")]->setDelayed(false);

    kactionmenu_map[QStringLiteral("alternate")] = new KActionMenu(i18n("Other Versions"), actionCollection());
    actionCollection()->addAction(QStringLiteral("rellinks_other_versions"), kactionmenu_map[QStringLiteral("alternate")]);
    m_more->addAction(kactionmenu_map[QStringLiteral("alternate")]);
    kactionmenu_map[QStringLiteral("alternate")]->setWhatsThis(i18n("<p>This link references the alternate versions of this document.</p>"));
    connect(kactionmenu_map[QStringLiteral("alternate")]->menu(), &QMenu::triggered, this, &RelLinksPlugin::actionTriggered);
    kactionmenu_map[QStringLiteral("alternate")]->setDelayed(false);

    // Unclassified menu
    m_links = new KActionMenu(QIcon::fromTheme(QStringLiteral("rellinks")), i18n("Miscellaneous"), actionCollection());
    actionCollection()->addAction(QStringLiteral("rellinks_links"), m_links);
    kactionmenu_map[QStringLiteral("unclassified")] = m_links;
    kactionmenu_map[QStringLiteral("unclassified")]->setWhatsThis(i18n("<p>Miscellaneous links.</p>"));
    connect(kactionmenu_map[QStringLiteral("unclassified")]->menu(), &QMenu::triggered, this, &RelLinksPlugin::actionTriggered);
    kactionmenu_map[QStringLiteral("unclassified")]->setDelayed(false);

    // We unactivate all the possible actions
    disableAll();

    // When the rendering of the HTML is done, we update the site navigation bar
    m_part = qobject_cast<KHTMLPart *>(parent);
    if (!m_part) {
        return;
    }

    connect(m_part, SIGNAL(docCreated()), this, SLOT(newDocument()));
    connect(m_part, SIGNAL(completed()), this, SLOT(loadingFinished()));

    // create polling timer and connect it
    m_pollTimer = new QTimer(this);
    m_pollTimer->setObjectName(QStringLiteral("polling timer"));
    connect(m_pollTimer, &QTimer::timeout, this, &RelLinksPlugin::updateToolbar);

    // delay access to our part's members until it has finished its initialisation
    QTimer::singleShot(0, this, SLOT(delayedSetup()));

}

/** Destructor */
RelLinksPlugin::~RelLinksPlugin()
{
}

bool RelLinksPlugin::eventFilter(QObject *watched, QEvent *event)
{
    if (m_part == nullptr) {
        return false;
    }

    if (watched == nullptr || event == nullptr) {
        return false;
    }

    if (watched == m_view) {
        switch (event->type()) {
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
    if (m_part == nullptr) {
        return;
    }

    m_view = m_part->view();
    m_view->installEventFilter(this);
    m_viewVisible = m_view->isVisible();
}

void RelLinksPlugin::newDocument()
{
    // start calling upateToolbar periodically to get the new links as soon as possible

    m_pollTimer->start(500);
    //kDebug(90210) << "newDocument()";

    updateToolbar();
}

void RelLinksPlugin::loadingFinished()
{
    m_pollTimer->stop();
    //kDebug(90210) << "loadingFinished()";
    updateToolbar();
    guessRelations();
}

/* Update the site navigation bar */
void RelLinksPlugin::updateToolbar()
{

    // If we have a part
    if (!m_part) {
        return;
    }

    // We disable all
    disableAll();

    // get a list of LINK nodes in document
    DOM::NodeList linkNodes = m_part->document().getElementsByTagName("link");

    //kDebug(90210) << "Rellinks: Link nodes =" << linkNodes.length();

    bool showBar = false;
    unsigned long nodeLength = linkNodes.length();
    m_linksFound = nodeLength > 0;

    for (unsigned int i = 0; i < nodeLength; i++) {
        // create a entry for each one
        DOM::Element e(linkNodes.item(i));

        // --- Retrieve of the relation type --

        QString rel = e.getAttribute("rel").string();
        rel = rel.simplified();
        if (rel.isEmpty()) {
            // If the "rel" attribute is null then use the "rev" attribute...
            QString rev = e.getAttribute("rev").string();
            rev = rev.simplified();
            if (rev.isEmpty()) {
                // if "rev" attribut is also empty => ignore
                continue;
            }
            // Determine the "rel" equivalent of "rev" type
            rel =  transformRevToRel(rev);
        }
        // Determine the name used internally
        QString lrel = getLinkType(rel.toLower());
        // relation to ignore
        if (lrel.isEmpty()) {
            continue;
        }
//  kDebug() << "lrel=" << lrel;

        // -- Retrieve of other useful information --

        QString href = e.getAttribute("href").string();
        // if nowhere to go, ignore the link
        if (href.isEmpty()) {
            continue;
        }
        QString title = e.getAttribute("title").string();
        QString hreflang = e.getAttribute("hreflang").string();

        QUrl ref(resolvedUrl(m_part->url(), href));
        if (title.isEmpty()) {
            title = ref.toDisplayString();
        }

        // escape ampersand before settings as action title, otherwise the menu entry will interpret it as an
        // accelerator
        title.replace('&', QLatin1String("&&"));

        // -- Menus activation --

        // Activation of "Document" menu ?
        if (lrel == QLatin1String("contents") || lrel == QLatin1String("glossary") || lrel == QLatin1String("index") || lrel == QLatin1String("appendix")) {
            m_document->setEnabled(true);
        }
        // Activation of "More" menu ?
        if (lrel == QLatin1String("help") || lrel == QLatin1String("author") || lrel == QLatin1String("copyright")) {
            m_more->setEnabled(true);
        }

        // -- Buttons or menu items activation / creation --
        if (lrel == QLatin1String("bookmark") || lrel == QLatin1String("alternate")) {
            QAction *a = kactionmenu_map[lrel]->menu()->addAction(title);
            a->setData(QVariant::fromValue(e));
            m_more->setEnabled(true);
            kactionmenu_map[lrel]->setEnabled(true);

        } else if (lrel == QLatin1String("appendix") || lrel == QLatin1String("chapter") || lrel == QLatin1String("section") || lrel == QLatin1String("subsection")) {
            QAction *a = kactionmenu_map[lrel]->menu()->addAction(title);
            m_document->setEnabled(true);
            kactionmenu_map[lrel]->setEnabled(true);
            a->setData(QVariant::fromValue<DOM::Element>(e));

        } else {
            // It is a unique action
            QAction *a = actionCollection()->action("rellinks_" + lrel);
            if (a) {
                a->setData(QVariant::fromValue<DOM::Element>(e));
                a->setEnabled(true);
                // Tooltip
                if (hreflang.isEmpty()) {
                    a->setToolTip(title);
                } else {
                    a->setToolTip(title + " [" + hreflang + ']');
                }
            } else {
                // For the moment all the elements are reference in a separated menu
                // TODO : reference the unknown ?
                QAction *a = kactionmenu_map[QStringLiteral("unclassified")]->menu()->addAction(lrel + " : " + title);
                kactionmenu_map[QStringLiteral("unclassified")]->setEnabled(true);
                a->setData(QVariant::fromValue<DOM::Element>(e));
            }

        }

        showBar = true;
    }
}

void RelLinksPlugin::guessRelations()
{
    m_part = qobject_cast<KHTMLPart *>(parent());
    if (!m_part || m_part->document().isNull()) {
        return;
    }

    //If the page already contains some link, that mean the webmaster is aware
    //of the meaning of <link> so we can consider that if prev/next was possible
    //they are already there.
    if (m_linksFound) {
        return;
    }

    // - The number of digits may not be more of 3, or this is certainly an id.
    // - We make sure that the number is followed by a dot, a &, or the end, we
    //   don't want to match stuff like that:   page.html?id=A14E12FD
    // - We make also sure the number is not preceded directly by others number
    QRegExp rx("^(.*[=/?&][^=/?&.\\-0-9]*)([\\d]{1,3})([.&][^/0-9]{0,15})?$");

    const QString zeros(QStringLiteral("0000"));
    QString url = m_part->url().url();
    if (rx.indexIn(url) != -1) {
        uint val = rx.cap(2).toUInt();
        int lenval = rx.cap(2).length();
        QString nval_str = QString::number(val + 1);
        //prepend by zeros if the original also contains zeros.
        if (nval_str.length() < lenval && rx.cap(2)[0] == '0') {
            nval_str.prepend(zeros.left(lenval - nval_str.length()));
        }

        QString href = rx.cap(1) + nval_str + rx.cap(3);
        QUrl ref(resolvedUrl(m_part->url(), href));
        QString title = i18n("[Autodetected] %1", ref.toDisplayString());
        DOM::Element e = m_part->document().createElement("link");
        e.setAttribute("href", href);
        QAction *a = actionCollection()->action(QStringLiteral("rellinks_next"));
        a->setEnabled(true);
        a->setToolTip(title);
        a->setData(QVariant::fromValue(e));

        if (val > 1) {
            nval_str = QString::number(val - 1);
            if (nval_str.length() < lenval && rx.cap(2)[0] == '0') {
                nval_str.prepend(zeros.left(lenval - nval_str.length()));
            }
            QString href = rx.cap(1) + nval_str + rx.cap(3);
            QUrl ref(resolvedUrl(m_part->url(), href));
            QString title = i18n("[Autodetected] %1", ref.toDisplayString());
            e = m_part->document().createElement("link");
            e.setAttribute("href", href);
            QAction *a = actionCollection()->action(QStringLiteral("rellinks_prev"));
            a->setEnabled(true);
            a->setToolTip(title);
            a->setData(QVariant::fromValue(e));
        }
    }
}

void RelLinksPlugin::goToLink(DOM::Element e)
{
    // have the KHTML part open it
    KHTMLPart *part = qobject_cast<KHTMLPart *>(parent());
    if (!part) {
        return;
    }
    QString href = e.getAttribute("href").string();
    QUrl url(resolvedUrl(part->url(), href));
    QString target = e.getAttribute("target").string();

    // URL arguments
    KParts::OpenUrlArguments arguments;
    KParts::BrowserArguments browserArguments;
    browserArguments.frameName = target;

    // Add base url if not valid
    if (url.isValid()) {
        part->browserExtension()->openUrlRequest(url, arguments, browserArguments);
    } else {
        QUrl baseURL = part->baseURL();
        QString endURL = url.toDisplayString();
        QUrl realURL = resolvedUrl(baseURL, endURL);
        part->browserExtension()->openUrlRequest(realURL, arguments, browserArguments);
    }

}

void RelLinksPlugin::disableAll()
{
    m_linksFound = false;

    foreach (QAction *a, actionCollection()->actionGroups()[0]->actions()) {
        a->setEnabled(false);
        a->setToolTip(a->text().remove('&'));
    }

    // Clear actions
    KActionMenuMap::Iterator itmenu;
    for (itmenu = kactionmenu_map.begin(); itmenu != kactionmenu_map.end(); ++itmenu) {
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

QString RelLinksPlugin::getLinkType(const QString &lrel)
{
    // Relations to ignore...
    if (lrel.contains(QLatin1String("stylesheet"))
            || lrel == QLatin1String("script")
            || lrel == QLatin1String("icon")
            || lrel == QLatin1String("shortcut icon")
            || lrel == QLatin1String("prefetch")) {
        return QString();
    }

    // ...known relations...
    if (lrel == QLatin1String("top") || lrel == QLatin1String("origin") || lrel == QLatin1String("start")) {
        return QStringLiteral("home");
    }
    if (lrel == QLatin1String("parent")) {
        return QStringLiteral("up");
    }
    if (lrel == QLatin1String("first")) {
        return QStringLiteral("begin");
    }
    if (lrel == QLatin1String("previous")) {
        return QStringLiteral("prev");
    }
    if (lrel == QLatin1String("child")) {
        return QStringLiteral("next");
    }
    if (lrel == QLatin1String("end")) {
        return QStringLiteral("last");
    }
    if (lrel == QLatin1String("toc")) {
        return QStringLiteral("contents");
    }
    if (lrel == QLatin1String("find")) {
        return QStringLiteral("search");
    }
    if (lrel == QLatin1String("alternative stylesheet")) {
        return QStringLiteral("alternate stylesheet");
    }
    if (lrel == QLatin1String("authors")) {
        return QStringLiteral("author");
    }
    if (lrel == QLatin1String("toc")) {
        return QStringLiteral("contents");
    }

    //...unknown relations or name that don't need to change
    return lrel;
}

QString RelLinksPlugin::transformRevToRel(const QString &rev)
{
    QString altRev = getLinkType(rev);

    // Known relations
    if (altRev == QLatin1String("prev")) {
        return getLinkType(QStringLiteral("next"));
    }
    if (altRev == QLatin1String("next")) {
        return getLinkType(QStringLiteral("prev"));
    }
    if (altRev == QLatin1String("made")) {
        return getLinkType(QStringLiteral("author"));
    }
    if (altRev == QLatin1String("up")) {
        return getLinkType(QStringLiteral("child"));
    }
    if (altRev == QLatin1String("sibling")) {
        return getLinkType(QStringLiteral("sibling"));
    }

    //...unknown inverse relation => ignore for the moment
    return QString();
}

void RelLinksPlugin::actionTriggered(QAction *action)
{
    if (action->data().isValid()) {
        goToLink(action->data().value<DOM::Element>());
    }
}

#include "plugin_rellinks.moc"
