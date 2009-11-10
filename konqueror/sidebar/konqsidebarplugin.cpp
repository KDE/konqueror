/* This file is part of the KDE project
   Copyright (C) 2001,2002 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konqsidebarplugin.h"
#include "konqsidebarplugin.moc"
#include <kdebug.h>

class KonqSidebarModulePrivate
{
public:
    KonqSidebarModulePrivate()
        : m_copy(false), m_cut(false), m_paste(false) {}
    bool m_copy;
    bool m_cut;
    bool m_paste;
};

KonqSidebarModule::KonqSidebarModule(const KComponentData &componentData,
                                     QObject *parent,
                                     const KConfigGroup& configGroup_)
    : QObject(parent),
      m_parentComponentData(componentData),
      m_configGroup(configGroup_),
      d(new KonqSidebarModulePrivate)
{
}

KonqSidebarModule::~KonqSidebarModule()
{
    delete d;
}

const KComponentData &KonqSidebarModule::parentComponentData() const { return m_parentComponentData; }

void KonqSidebarModule::openUrl(const KUrl& url)
{
    handleURL(url);
}

void KonqSidebarModule::openPreview(const KFileItemList& items)
{
    handlePreview(items);
}

void KonqSidebarModule::openPreviewOnMouseOver(const KFileItem& item)
{
    handlePreviewOnMouseOver(item);
}

void KonqSidebarModule::handlePreview(const KFileItemList& /*items*/) {}

void KonqSidebarModule::handlePreviewOnMouseOver(const KFileItem& /*items*/) {}

KConfigGroup KonqSidebarModule::configGroup()
{
    return m_configGroup;
}

void KonqSidebarModule::enableCopy(bool enabled)
{
    d->m_copy = enabled;
    emit enableAction(this, "copy", enabled);
}

void KonqSidebarModule::enableCut(bool enabled)
{
    d->m_cut = enabled;
    emit enableAction(this, "cut", enabled);
}

void KonqSidebarModule::enablePaste(bool enabled)
{
    d->m_paste = enabled;
    emit enableAction(this, "paste", enabled);
}

bool KonqSidebarModule::isCopyEnabled() const
{
    return d->m_copy;
}

bool KonqSidebarModule::isCutEnabled() const
{
    return d->m_cut;
}

bool KonqSidebarModule::isPasteEnabled() const
{
    return d->m_paste;
}

void KonqSidebarModule::showPopupMenu(const QPoint &global, const KFileItemList &items,
                                      const KParts::OpenUrlArguments &args,
                                      const KParts::BrowserArguments &browserArgs,
                                      KParts::BrowserExtension::PopupFlags flags,
                                      const KParts::BrowserExtension::ActionGroupMap& actionGroups)
{
    emit popupMenu(this, global, items, args, browserArgs, flags, actionGroups);
}
