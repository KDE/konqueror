/* This file is part of the KDE project

   Copyright (C) 2002 Patrick Charbonnier <pch@valleeurpe.net>
   Copyright (C) 2010 Matthias Fuchs <mat69@gmx.net>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
*/

#ifndef KGETPLUGIN_H
#define KGETPLUGIN_H

#include <konq_kpart_plugin.h>
#include "interfaces/selectorinterface.h"

#include <QPointer>

class KToggleAction;
class HtmlExtension;

class KGetPlugin : public KonqParts::Plugin
{
    Q_OBJECT
public:
    KGetPlugin(QObject *parent, const QVariantList &);
    ~KGetPlugin() override;

private Q_SLOTS:
    void slotShowDrop();
    void slotShowLinks();
    void slotShowSelectedLinks();
    void slotImportLinks();
    void showPopup();

private:
    void getLinks(bool selectedOnly = false);
    void fillLinkListFromHtml(const QUrl &baseUrl, const QList<KonqInterfaces::SelectorInterface::Element> &elements);

    QStringList m_linkList;
    KToggleAction *m_dropTargetAction;
};

#endif
