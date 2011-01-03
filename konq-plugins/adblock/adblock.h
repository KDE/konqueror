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

#ifndef KONQ_ADBLOCK_H
#define KONQ_ADBLOCK_H

#include <qpointer.h>
#include <qlist.h>
#include <kparts/plugin.h>
#include <dom/dom_node.h>
#include <kurl.h>
#include <QWeakPointer>

class KHTMLPart;
class KUrlLabel;
class AdElement;
class KActionMenu;
namespace KParts
{
}

namespace DOM
{
    class DOMString;
}

typedef QList<AdElement> AdElementList;

class AdBlock : public KParts::Plugin
{
    Q_OBJECT

public:
    AdBlock(QObject* parent = 0, const QVariantList &args = QVariantList());
    ~AdBlock();

private:
    QPointer<KHTMLPart> m_part;
    QWeakPointer<KUrlLabel> m_label;
    KActionMenu *m_menu;

    void fillBlockableElements();
    void fillWithImages();
    void fillWithHtmlTag(const DOM::DOMString &tagName,
			 const DOM::DOMString &attrName,
			 const QString &category);

private slots:
    void initLabel();
    void slotConfigure();
    void addAdFilter(const QString &url);
    void contextMenu();
    void showKCModule();
    void slotDisableForThisPage();
    void slotDisableForThisSite();

private:
    void disableForUrl(KUrl url);
    void updateFilters();

    AdElementList *m_elements;
};

// ----------------------------------------------------------------------------

class AdElement
{
public:
    AdElement();
    AdElement(const QString &url, const QString &category,
	      const QString &type, bool blocked, const DOM::Node& node);

    AdElement &operator=(const AdElement &);
    bool operator==(const AdElement &e1);

    bool isBlocked() const;
    QString blockedBy() const;
    void setBlocked(bool blocked);
    void setBlockedBy(const QString &by);
    QString url() const;
    QString category() const;
    QString type() const;
    DOM::Node node() const;
private:
    QString m_url;
    QString m_category;
    QString m_type;
    bool m_blocked;
    QString m_blockedBy;
    DOM::Node m_node;
};

#endif
