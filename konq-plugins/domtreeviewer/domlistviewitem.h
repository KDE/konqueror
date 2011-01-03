/***************************************************************************
                              domlistviewitem.h
                             -------------------

    author               : Andreas Schlapbach
    email                : schlpbch@iam.unibe.ch
    author               : Harri Porten
    email                : porten@kde.org
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef DOMLISTVIEWITEMS_H
#define DOMLISTVIEWITEMS_H
 
#include <dom/dom_node.h>

#include <qtreewidget.h>
#include <qcolor.h>
#include <qfont.h>

class DOMListViewItem : public QTreeWidgetItem
{

 public:
   DOMListViewItem( const DOM::Node &, QTreeWidget *parent );
   DOMListViewItem( const DOM::Node &, QTreeWidget *parent,
                    QTreeWidgetItem *preceding ); 
   DOMListViewItem( const DOM::Node &, QTreeWidgetItem *parent );
   DOMListViewItem( const DOM::Node &, QTreeWidgetItem *parent,
                    QTreeWidgetItem *preceding );
   virtual ~DOMListViewItem();
 
  void setColor(const QColor &color) { setForeground(0, color); }

  void setFont(const QFont &font) { m_font = font;
                                    QTreeWidgetItem::setFont(0, font); }
  void setItalic(bool b) {m_font.setItalic(b); setFont(m_font);}
  void setBold(bool b) {m_font.setBold(b); setFont(m_font); }
  void setUnderline(bool b) {m_font.setUnderline(b); setFont(m_font); }
  
  bool isClosing() const { return clos; }
  void setClosing(bool s) { clos = s; }

  DOM::Node node() const { return m_node; }

 private:
  void init();
  QFont m_font;
  DOM::Node m_node;
  bool clos;
};
#endif
