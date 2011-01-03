/***************************************************************************
 *   Copyright (C) 2002, Anders Lund <anders@alweb.dk>                     *
 *   Copyright (C) 2003, 2004, Franck Quélain <shift@free.fr>              *
 *   Copyright (C) 2004, Kevin Krammer <kevin.krammer@gmx.at>              *
 *   Copyright (C) 2004, 2005, Oliviet Goffart <ogoffart @ kde.org>        *
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


#ifndef _PLUGIN_RELLINKS_H_
#define _PLUGIN_RELLINKS_H_

/*
   This plugin queries the current HTML document for LINK elements.

   This plugin create a toolbar similar to the Site Navigation Bar of Mozilla
*/

// Qt includes
#include <qmap.h>

// KDE includes
#include <kparts/plugin.h>
#include <dom/dom_string.h>

// type definitions
typedef QMap<int,DOM::Element> DOMElementMap;
typedef QMap<QString, QAction*> KActionMap;
typedef QMap<QString, KActionMenu*> KActionMenuMap;

// forward declarations
class KActionMenu;
class KHTMLPart;
class KHTMLView;
class QTimer;

/**
 * KPart plugin adding document relations toolbar to Konqueror
 * @author Franck Quélain
 * @author Anders Lund
 */
class RelLinksPlugin : public KParts::Plugin {
    Q_OBJECT
public:
    /** Constructor */
    RelLinksPlugin( QObject *parent, const QVariantList & );
    /** Destructor */
    virtual ~RelLinksPlugin();

    bool eventFilter(QObject *watched, QEvent* event);

private slots:
    void delayedSetup();

    /**
     * KHTMLPart has created a new document, disable actions and
     * start polling for links
     */
    void newDocument();

    /**
     * KHTMLPart has finished loading, stop the poller and
     * parse the document a last time.
     */
    void loadingFinished();

    /**
     * Update the toolbar (Parse the document again)
     */
    void updateToolbar();


    void goHome();
    void goUp();
    void goFirst();
    void goPrevious();
    void goNext();
    void goLast();
    void goContents();
    void goIndex();
    void goGlossary();
    void goHelp();
    void goSearch();
    void goCopyright();
    void goAuthor();

    void goBookmark(int id);
    void goChapter(int id);
    void goSection(int id);
    void goSubsection(int id);
    void goAppendix(int id);
    void goAlternate(int id);
    void goAllElements(int id);

private:

    /**
     * Try to guess some relations from the url, if the document doesn't contains relations
	 * example:   http://example.com/page4.html
	 * the "next" relation will be set to page5.html
	 */
	void guessRelations();
	
    /**
     * Function used to get link type of a relation.
     * For example "prev" is of type "previous" and "toc" is of type "contents"
     * If the relation must be ignored return NULL.
     * If the relation is unknow return the input relation type.
     * @param lrel Previous relation name
     * @return New relation name
     */
    QString getLinkType(const QString &lrel);

    /**
     * Function used to return the "rel" equivalent of "rev" link type
     * If the equivalent is not found return NULL
     * @param rev Inverse relation name
     * @return Equivalent relation name
     */
    QString transformRevToRel(const QString &rev) ;

    /**
     * Function used to disable all the item of the toolbar
     */
    void disableAll();

    /**
     * Go to the link
     * @param rel Relation name
     * @param id Identifier of the menu item
     */
    void goToLink(const QString & rel, int id=0);

private:
    KHTMLPart* m_part;
    KHTMLView* m_view;
	bool m_viewVisible;

    KActionMenu *m_document;
    KActionMenu *m_more;
    KActionMenu *m_links;

    /** Map of KAction */
    KActionMap kaction_map;
    /** Map of KActionMenu */
    KActionMenuMap kactionmenu_map;

    /** Map of all the link element which can be managed by rellinks */
    QMap<QString,DOMElementMap> element_map;

    QTimer* m_pollTimer;
};

#endif // _PLUGIN_RELLINKS_H_
