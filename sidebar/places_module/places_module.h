/*
    SPDX-FileCopyrightText: 2010 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
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
