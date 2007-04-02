/*  This file is part of the KDE libraries
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; version 2
    of the License.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
#ifndef _KCUSTOMMENUEDITOR_H_
#define _KCUSTOMMENUEDITOR_H_

#include <kdialog.h>

class K3ListView;
class KConfigBase;

 /*
  * Dialog for editing custom menus.
  *
  * @author Waldo Bastian (bastian@kde.org)
  */
class KCustomMenuEditor : public KDialog
{
    Q_OBJECT
public:
    /**
     * Create a dialog for editing a custom menu
     */
    KCustomMenuEditor(QWidget *parent);
    ~KCustomMenuEditor();
    /**
     * load the custom menu
     */
    void load(KConfigBase *);

    /**
     * save the custom menu
     */
    void save(KConfigBase *);

public Q_SLOTS:
    void slotNewItem();
    void slotRemoveItem();
    void slotMoveUp();
    void slotMoveDown();
    void refreshButton();

protected:
    class Item;
    K3ListView *m_listView;

    class KCustomMenuEditorPrivate;
    KCustomMenuEditorPrivate* const d;
};

#endif
