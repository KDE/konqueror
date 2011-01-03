// -*- mode: c++; c-basic-offset: 4 -*-
/*
  Copyright (c) 2008 Laurent Montel <montel@kde.org>
  Copyright (C) 2006 Daniele Galdi <daniele.galdi@gmail.com>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 51
  Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "adblock.h"
#include "adblockdialog.h"

#include <kdebug.h>
#include <kmenu.h>
#include <klocale.h>
#include <KTreeWidgetSearchLine>
#include <KRun>
#include <KStandardGuiItem>

#include <qcursor.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qcolor.h>
#include <qfont.h>
#include <qpainter.h>
#include <QTreeWidget>
#include <QApplication>
#include <QClipboard>
#include <QVBoxLayout>

// ----------------------------------------------------------------------------

class ListViewItem : public QTreeWidgetItem
{
public:
    ListViewItem(QTreeWidget *listView, const QStringList& lst,const AdElement *element)
        : QTreeWidgetItem( listView, lst),
          m_element(element),
          m_blocked(false){};

    bool isBlocked() const { return (m_blocked); };
    void setBlocked(bool blocked);
    void setBlockedBy(const QString &reason);
    void setNode( const DOM::Node& node );
    DOM::Node node()const { return (m_node); }
    const AdElement *element() const { return (m_element); }

private:
    const AdElement *m_element;
    bool m_blocked;
    DOM::Node m_node;
};


void ListViewItem::setBlocked(bool blocked)
{
    m_blocked = blocked;
    setData ( 0,Qt::TextColorRole, (blocked ? Qt::red : Qt::black));
    QFont itemFont =  font(0);
    itemFont.setItalic( blocked );
    itemFont.setBold( blocked );
    setData( 0, Qt::FontRole, itemFont );
}

void ListViewItem::setBlockedBy(const QString &reason)
{
    setToolTip( 0, reason );
}


void ListViewItem::setNode( const DOM::Node& node )
{
    m_node = node;
}

// ----------------------------------------------------------------------------

AdBlockDlg::AdBlockDlg(QWidget *parent, const AdElementList *elements, KHTMLPart*part) :
    KDialog( parent ), m_part( part )
{
    setModal( true );
    setCaption( i18n("Blockable items on this page") );
    setButtons( KDialog::User1 | KDialog::User2 | KDialog::Close );
    setDefaultButton( KDialog::User2 );
    setButtonText( KDialog::User1, i18n("Configure Filters...") );
    setButtonIcon( KDialog::User1, KIcon("preferences-web-browser-adblock") );
    setButtonText( KDialog::User2, i18n("Add filter") );
    setButtonIcon( KDialog::User2, KStandardGuiItem::add().icon() );

    QWidget *page = new QWidget(this);
    setMainWidget(page);

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setMargin(0);

    QLabel *l = new QLabel( i18n("Search:"), page);
    layout->addWidget(l);

    KTreeWidgetSearchLine* searchLine = new KTreeWidgetSearchLine( page );
    layout->addWidget(searchLine);
    l->setBuddy(searchLine);

    l = new QLabel( i18n("Blockable items:"), page);
    layout->addWidget(l);

    m_list = new QTreeWidget(page);
    m_list->setAllColumnsShowFocus( true );
    layout->addWidget(m_list);
    l->setBuddy(m_list);

    QStringList lstHeader;
    lstHeader<<i18n("Source")<<i18n("Category")<<i18n("Tag");
    m_list->setHeaderLabels(lstHeader);

    m_list->setColumnWidth(0, 600);
    m_list->setColumnWidth(1, 90);
    m_list->setColumnWidth(2, 65);

    m_list->setRootIsDecorated( false );

    AdElementList::const_iterator it;
    for ( it = elements->constBegin(); it != elements->constEnd(); ++it )
    {
	const AdElement &element = (*it);

        QStringList lst;
        lst << element.url() <<  element.category() << element.type();

        ListViewItem *item = new ListViewItem( m_list, lst, &element );
	item->setBlocked( element.isBlocked() );
	item->setBlockedBy( element.blockedBy() );
        item->setNode( element.node() );
    }

    searchLine->setTreeWidget( m_list );

    layout->addSpacing(KDialog::spacingHint());

    l = new QLabel( i18n("New filter (can use *?[] wildcards, /RE/ for regular expression, prefix with @@ for white list):"), page);
    layout->addWidget(l);

    m_filter = new QLineEdit( page);
    layout->addWidget(m_filter);
    connect(m_filter,SIGNAL(textChanged(const QString &)),SLOT(filterTextChanged(const QString &)));
    l->setBuddy(m_filter);
    filterTextChanged(QString::null);

    connect(this, SIGNAL( user1Clicked() ), this, SLOT( slotConfigureFilters() ));
    connect(this, SIGNAL( user2Clicked() ), this, SLOT( slotAddFilter() ));

    // Use itemActivated() signal instead of itemDoubleClicked() to honour
    // the KDE single/double click setting, and allow keyboard navigation.
    // Activating a item just copies its URL to the "new filter" box and
    // sets focus to there, it doesn't add the filter immediately.
    //connect(m_list, SIGNAL( itemDoubleClicked(QTreeWidgetItem *, int )), this, SLOT(updateFilter(QTreeWidgetItem *)) );
    connect(m_list, SIGNAL( itemActivated(QTreeWidgetItem *, int )), this, SLOT(updateFilter(QTreeWidgetItem *)) );

    m_menu = new KMenu(this);
    m_menu->addAction(i18n("Filter this item"), this, SLOT(filterItem()));
    m_menu->addAction(i18n("Filter all items at same path"), this, SLOT(filterPath()));
    m_menu->addAction(i18n("Filter all items from same host"), this, SLOT(filterHost()));
    m_menu->addAction(i18n("Filter all items from same domain"), this, SLOT(filterDomain()));
    m_menu->addSeparator();
    m_menu->addAction( i18n( "Add this item to white list" ), this, SLOT( addWhiteList() ) );
    m_menu->addSeparator();
    m_menu->addAction( i18n( "Copy Link Address" ),  this,  SLOT( copyLinkAddress() ) );
    //comment for the moment
    //m_menu->addAction( i18n( "Highlight Element" ), this, SLOT( highLightElement() ) );
    m_menu->addAction( i18n( "View item" ), this, SLOT( showElement() ) );
    m_list->setContextMenuPolicy( Qt::CustomContextMenu );
    connect( m_list, SIGNAL( customContextMenuRequested( const QPoint & ) ),
           this, SLOT( showContextMenu(const QPoint & ) ) );

    resize( 800, 400 );
}


AdBlockDlg::~AdBlockDlg()
{
}



void AdBlockDlg::filterTextChanged(const QString &text)
{
    enableButton(KDialog::User2,!text.isEmpty());
}


void AdBlockDlg::setFilterText(const QString &text)
{
    m_filter->setText(text);
    m_filter->setFocus(Qt::OtherFocusReason);
}


void AdBlockDlg::updateFilter(QTreeWidgetItem *selected)
{
    ListViewItem *item = static_cast<ListViewItem *>(selected);

    if (item->isBlocked())
    {
	m_filter->clear();
	return;
    }

    setFilterText( item->text(0) );
}

void AdBlockDlg::slotAddFilter()
{
    const QString text = m_filter->text().trimmed();
    if (text.isEmpty()) return;

    kDebug() << "adding filter" << text;
    emit notEmptyFilter(text);

    for (QTreeWidgetItemIterator it(m_list); (*it)!=0; ++it)
    {
        ListViewItem *item = static_cast<ListViewItem *>(*it);
        item->setBlocked(item->element()->isBlocked());
	item->setBlockedBy( item->element()->blockedBy() );
    }

    enableButton(KDialog::User2,false);			// now that filter has been added
}							// until it is changed again


void AdBlockDlg::slotConfigureFilters()
{
    emit configureFilters();
    delayedDestruct();
}


void AdBlockDlg::showContextMenu(const QPoint & pos)
{
    QPoint newPos = m_list->viewport()->mapToGlobal( pos );
    int column = m_list->columnAt(pos.x() );
    if (column == -1)
        return;
    m_menu->popup(newPos);
}





void AdBlockDlg::filterItem()
{
    QTreeWidgetItem* item = m_list->currentItem();
    setFilterText( item->text(0) );
}

KUrl AdBlockDlg::getItem()
{
    QTreeWidgetItem* item = m_list->currentItem();
    KUrl u(item->text(0));
    u.setQuery(QString());
    u.setRef(QString());
    return (u);
}

void AdBlockDlg::filterPath()
{
    KUrl u(getItem());
    u.setFileName("*");
    setFilterText(u.url());
}

void AdBlockDlg::filterHost()
{
    KUrl u(getItem());
    u.setPath("/*");
    setFilterText(u.url());
}

void AdBlockDlg::filterDomain()
{
    KUrl u(getItem());

    QString host = u.host();
    if (host.isEmpty()) return;
    int idx = host.indexOf('.');
    if (idx<0) return;

    QString t = u.protocol()+"://*"+host.mid(idx)+"/*";
    setFilterText(t);
}

void AdBlockDlg::addWhiteList()
{
    QTreeWidgetItem* item = m_list->currentItem();
    setFilterText( "@@" + item->text(0) );
}

void AdBlockDlg::copyLinkAddress()
{
    QApplication::clipboard()->setText(m_list->currentItem()->text( 0 )  );
}

void AdBlockDlg::highLightElement()
{
    ListViewItem *item = static_cast<ListViewItem *>(m_list->currentItem());
    if ( item )
    {
        DOM::Node handle = item->node();
        kDebug()<<" m_part :"<<m_part;
        if (!handle.isNull()) {
            m_part->setActiveNode(handle);
        }
    }
}

void AdBlockDlg::showElement()
{
    new KRun( m_list->currentItem()->text( 0 ), 0 );
}


#include "adblockdialog.moc"
