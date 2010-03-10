/*
    Copyright (C) 2010 Pino Toscano <pino@kde.org>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License or ( at
    your option ) version 3 or, at the discretion of KDE e.V. ( which shall
    act as a proxy as in section 14 of the GPLv3 ), any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef PLACES_MODULE_H
#define PLACES_MODULE_H

#include <konqsidebarplugin.h>

#include <kfileplacesview.h>

// A custom file places view to know which mouse buttons and keyboard modifiers
// were pressed when activating an URL.
class KonqPlacesCustomPlacesView : public KFilePlacesView
{
    Q_OBJECT

public:
    explicit KonqPlacesCustomPlacesView(QWidget *parent = 0);
    virtual ~KonqPlacesCustomPlacesView();

signals:
    void urlChanged(const KUrl &url, Qt::MouseButtons buttons, Qt::KeyboardModifiers);

protected:
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);

private slots:
    void emitUrlChanged(const KUrl &url);

private:
    Qt::MouseButtons m_mouseButtons;
    Qt::KeyboardModifiers m_keyModifiers;
};


class KonqSideBarPlacesModule : public KonqSidebarModule
{
    Q_OBJECT

public:
    KonqSideBarPlacesModule(const KComponentData &componentData, QWidget *parent,
                            const KConfigGroup &configGroup);
    virtual ~KonqSideBarPlacesModule();

    virtual QWidget *getWidget();

private slots:
    void slotPlaceUrlChanged(const KUrl &url, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);

private:
    KFilePlacesView *m_placesView;
};

#endif
