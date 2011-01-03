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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef KONQ_ADBLOCKDLG_H
#define KONQ_ADBLOCKDLG_H

#include <kdialog.h>
#include <QTreeWidgetItem>
#include <khtml_part.h>
class QLineEdit;
class KMenu;
class QTreeWidget;

class AdBlockDlg : public KDialog
{
    Q_OBJECT
public:
    AdBlockDlg(QWidget *parent, const AdElementList *elements, KHTMLPart*part);
    ~AdBlockDlg();

private slots:
    void slotAddFilter();
    void slotConfigureFilters();

    void updateFilter(QTreeWidgetItem *item);
    void showContextMenu(const QPoint&);
    void filterItem();
    void filterPath();
    void filterHost();
    void filterDomain();
    void addWhiteList();
    void copyLinkAddress();
    void highLightElement();
    void showElement();
    void filterTextChanged(const QString &text);

signals:
    void notEmptyFilter(const QString &url);
    void configureFilters();

private:
    KUrl getItem();
    void setFilterText(const QString &text);

    QLineEdit *m_filter;
    QTreeWidget *m_list;
    KMenu *m_menu;
    KHTMLPart *m_part;
};

#endif
