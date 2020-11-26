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


class KonqSideBarPlacesModule : public KonqSidebarModule
{
    Q_OBJECT

public:
    KonqSideBarPlacesModule(QWidget *parent,
                            const KConfigGroup &configGroup);
    ~KonqSideBarPlacesModule() override = default;

    QWidget *getWidget() override;

private slots:
    void slotPlaceUrlChanged(const QUrl &url);

private:
    KFilePlacesView *m_placesView;
};

#endif
