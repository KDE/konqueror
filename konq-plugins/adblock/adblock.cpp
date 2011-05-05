// -*- mode: c++; c-basic-offset: 4 -*-
/*
  Copyright (c) 2008 Laurent Montel <montel@kde.org>
  Copyright (C) 2006 Daniele Galdi <daniele.galdi@gmail.com>

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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/

/* Project related */
#include "adblock.h"
#include "adblockdialog.h"

/* Kde related */
#include <kpluginfactory.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <klibloader.h>
#include <kparts/statusbarextension.h>
#include <khtml_part.h>
#include <khtml_settings.h>
#include <kstatusbar.h>
#include <kurllabel.h>
#include <kurl.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kcmultidialog.h>
#include <klocale.h>
#include <KActionCollection>
#include <KActionMenu>
#include <KMenu>
#include <dom/html_document.h>
#include <dom/html_image.h>
#include <dom/html_inline.h>
#include <dom/html_misc.h>
#include <dom/html_element.h>
#include <dom/dom_doc.h>

using namespace DOM;

#include <qpixmap.h>
#include <qcursor.h>
#include <qregexp.h>


K_PLUGIN_FACTORY( AdBlockFactory, registerPlugin< AdBlock >(); )
K_EXPORT_PLUGIN( AdBlockFactory( "adblock" ) )

AdBlock::AdBlock(QObject *parent, const QVariantList & /*args*/) :
    Plugin(parent),
    m_menu(0), m_elements(0)
{
    m_part = dynamic_cast<KHTMLPart *>(parent);
    if(!m_part)
    {
        kDebug() << "couldn't get KHTMLPart";
        return;
    }
    m_menu = new KActionMenu(KIcon( "preferences-web-browser-adblock" ), i18n("Adblock"),
                           actionCollection() );
    actionCollection()->addAction( "action adblock", m_menu );
    m_menu->setDelayed( false );

    QAction *a = actionCollection()->addAction(  "show_elements");
    a->setText(i18n("Show Blockable Elements..."));
    connect(a, SIGNAL(triggered()), this, SLOT(slotConfigure()));
    m_menu->addAction(a);

    a = actionCollection()->addAction(  "configure");
    a->setText(i18n("Configure Filters..."));
    connect(a, SIGNAL(triggered()), this, SLOT(showKCModule()));
    m_menu->addAction(a);

    a = actionCollection()->addAction( "separator" );
    a->setSeparator( true );
    m_menu->addAction(a);

    a = actionCollection()->addAction(  "disable_for_this_page");
    a->setText(i18n("No blocking for this page"));
    connect(a, SIGNAL(triggered()), this, SLOT(slotDisableForThisPage()));
    m_menu->addAction(a);

    a = actionCollection()->addAction(  "disable_for_this_site");
    a->setText(i18n("No blocking for this site"));
    connect(a, SIGNAL(triggered()), this, SLOT(slotDisableForThisSite()));
    m_menu->addAction(a);

    connect(m_part, SIGNAL(completed()), this, SLOT(initLabel()));
}

AdBlock::~AdBlock()
{
    KParts::StatusBarExtension *statusBarEx = KParts::StatusBarExtension::childObject(m_part);

    if (statusBarEx && m_label)
        statusBarEx->removeStatusBarItem(m_label.data());
    delete m_label.data();
    m_label.clear();
    delete m_menu;
    m_menu = 0;
    delete m_elements;
    m_elements = 0;
}

void AdBlock::initLabel()
{
    if (m_label) return;

    KParts::StatusBarExtension *statusBarEx = KParts::StatusBarExtension::childObject(m_part);

    if (!statusBarEx) {
        kDebug() << "couldn't get KParts::StatusBarExtension";
        return;
    }

    KUrlLabel* label = new KUrlLabel(statusBarEx->statusBar());

    KIconLoader *loader = KIconLoader::global();

    label->setFixedHeight(loader->currentSize(KIconLoader::Small));
    label->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    label->setUseCursor(false);
    label->setPixmap(loader->loadIcon("preferences-web-browser-adblock", KIconLoader::Small));

    statusBarEx->addStatusBarItem(label, 0, false);
    connect(label, SIGNAL(leftClickedUrl()), this, SLOT(slotConfigure()));
    connect(label, SIGNAL(rightClickedUrl()), this, SLOT(contextMenu()));
    
    m_label = label;
}


void AdBlock::disableForUrl(KUrl url)
{
    url.setQuery(QString());
    url.setRef(QString());

    KHTMLSettings *settings = const_cast<KHTMLSettings *>(m_part->settings());
    settings->addAdFilter("@@"+url.url());
}

void AdBlock::slotDisableForThisPage()
{
    disableForUrl(m_part->toplevelURL().url());
}

void AdBlock::slotDisableForThisSite()
{
    KUrl u(m_part->toplevelURL().url());
    u.setPath("/*");
    disableForUrl(u);
}


void AdBlock::slotConfigure()
{
    if (!m_part->settings()->isAdFilterEnabled())
    {
	KMessageBox::error(0,
                           i18n("Please enable Konqueror's Adblock"),
                           i18nc("@title:window", "Adblock disabled"));

	return;
    }

    m_elements = new AdElementList;
    fillBlockableElements();

    AdBlockDlg *dlg = new AdBlockDlg(m_part->widget(), m_elements, m_part);
    connect(dlg, SIGNAL( notEmptyFilter(const QString&) ), this, SLOT( addAdFilter(const QString&) ));
    connect(dlg, SIGNAL( configureFilters() ), this, SLOT( showKCModule() ));
    dlg->exec();
    delete dlg;
}

void AdBlock::showKCModule()
{
    KCMultiDialog* dialogue = new KCMultiDialog(m_part->widget());
    dialogue->addModule("khtml_filter");
    connect(dialogue, SIGNAL( cancelClicked() ), dialogue, SLOT( delayedDestruct() ));
    connect(dialogue, SIGNAL( closeClicked() ), dialogue, SLOT( delayedDestruct() ));
    dialogue->show();
}

void AdBlock::contextMenu()
{
    m_menu->menu()->exec(QCursor::pos());
}



void AdBlock::fillBlockableElements()
{
    fillWithHtmlTag("script", "src", i18n( "script" ));
    fillWithHtmlTag("embed" , "src", i18n( "object" ));
    fillWithHtmlTag("object", "src", i18n( "object" ));
    // TODO: iframe's are not blocked by KHTML yet
    fillWithHtmlTag("iframe", "src", i18n( "frame" ));
    fillWithImages();

    updateFilters();
}

void AdBlock::fillWithImages()
{
    HTMLDocument htmlDoc = m_part->htmlDocument();

    HTMLCollection images = htmlDoc.images();

    for (unsigned int i = 0; i < images.length(); i++)
    {
	HTMLImageElement image = static_cast<HTMLImageElement>( images.item(i) );

	DOMString src = image.src();

	QString url = htmlDoc.completeURL(src).string();
	if (!url.isEmpty() && url != m_part->baseURL().url())
	{
	    AdElement element(url, i18n( "image" ), "IMG", false, image);
	    if (!m_elements->contains( element ))
		m_elements->append( element);
	}
    }
}

void AdBlock::fillWithHtmlTag(const DOMString &tagName,
			      const DOMString &attrName,
			      const QString &category)
{
    Document doc = m_part->document();

    NodeList nodes = doc.getElementsByTagName(tagName);

    for (unsigned int i = 0; i < nodes.length(); i++)
    {
	Node node = nodes.item(i);
	Node attr = node.attributes().getNamedItem(attrName);

	DOMString src = attr.nodeValue();
	if (src.isNull()) continue;

	QString url = doc.completeURL(src).string();
	if (!url.isEmpty() && url != m_part->baseURL().url())
	{
	    AdElement element(url, category, tagName.string().toUpper(), false, attr);
	    if (!m_elements->contains( element ))
		m_elements->append( element);
	}
    }
}

void AdBlock::addAdFilter(const QString &url)
{
    //FIXME hackish
    KHTMLSettings *settings = const_cast<KHTMLSettings *>(m_part->settings());
    settings->addAdFilter(url);
    updateFilters();
}



void AdBlock::updateFilters()
{
    const KHTMLSettings *settings = m_part->settings();

    AdElementList::iterator it;
    for ( it = m_elements->begin(); it != m_elements->end(); ++it )
    {
	AdElement &element = (*it);

        bool isWhitelist;
        QString filter = settings->adFilteredBy(element.url(), &isWhitelist);
        if (!filter.isEmpty())
        {
            if (!isWhitelist)
            {
                element.setBlocked(true);
                element.setBlockedBy(i18n("Blocked by %1",filter));
            }
            else
                element.setBlockedBy(i18n("Allowed by %1",filter));
        }
    }
}



// ----------------------------------------------------------------------------

AdElement::AdElement() :
  m_blocked(false) {}

AdElement::AdElement(const QString &url, const QString &category,
		     const QString &type, bool blocked, const DOM::Node&node) :
  m_url(url), m_category(category), m_type(type), m_blocked(blocked),m_node( node ) {}

AdElement &AdElement::operator=(const AdElement &obj)
{
  m_blocked = obj.m_blocked;
  m_blockedBy = obj.m_blockedBy;
  m_url = obj.m_url;
  m_category = obj.m_category;
  m_type = obj.m_type;
  m_node = obj.m_node;

  return *this;
}

bool AdElement::operator==(const AdElement &obj)
{
    return m_url == obj.m_url;
}

bool AdElement::isBlocked() const
{
  return m_blocked;
}





QString AdElement::blockedBy() const
{
    return m_blockedBy;
}


void AdElement::setBlockedBy(const QString &by)
{
    m_blockedBy = by;
}






void AdElement::setBlocked(bool blocked)
{
    m_blocked = blocked;
}

QString AdElement::url() const
{
  return m_url;
}

QString AdElement::category() const
{
  return m_category;
}

QString AdElement::type() const
{
  return m_type;
}

DOM::Node AdElement::node() const
{
    return m_node;
}

#include "adblock.moc"
