/***************************************************************************
                             domlistviewitem.cpp
                             -------------------

    author               : Andreas Schlapbach
    email                : schlpbch@iam.unibe.ch
    author               : Harri Porten
    email                : porten@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "domlistviewitem.h"

#include <qpainter.h>
#include <qapplication.h>

#include <kglobalsettings.h>

DOMListViewItem::DOMListViewItem( const DOM::Node &node, QTreeWidget *parent )
  : QTreeWidgetItem( parent ), m_node(node)
{
  init();
}

DOMListViewItem::DOMListViewItem( const DOM::Node &node, QTreeWidget *parent, QTreeWidgetItem *preceding)
  : QTreeWidgetItem( parent, preceding ), m_node(node)
{
  init();
}

DOMListViewItem::DOMListViewItem( const DOM::Node &node, QTreeWidgetItem *parent )
  : QTreeWidgetItem( parent ), m_node(node)
{
  init();
}

DOMListViewItem::DOMListViewItem( const DOM::Node &node, QTreeWidgetItem *parent, QTreeWidgetItem *preceding)
  : QTreeWidgetItem( parent, preceding ), m_node(node)
{
  init();
}

DOMListViewItem::~DOMListViewItem()
{
  //NOP
}

void DOMListViewItem::init()
{
  setColor(QApplication::palette().color(QPalette::Active, QPalette::Text));
  m_font = KGlobalSettings::generalFont();
  QTreeWidgetItem::setFont(0, m_font);
  clos = false;
}
