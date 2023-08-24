/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2001, 2002 Joseph Wenninger <jowenn@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "konqsidebarplugin.h"

class KonqSidebarModulePrivate
{
public:
    KonqSidebarModulePrivate()
        : m_copy(false), m_cut(false), m_paste(false) {}
    bool m_copy;
    bool m_cut;
    bool m_paste;
};

KonqSidebarModule::KonqSidebarModule(QObject *parent,
                                     const KConfigGroup &configGroup_)
    : QObject(parent),
      m_configGroup(configGroup_),
      d(new KonqSidebarModulePrivate)
{
}

KonqSidebarModule::~KonqSidebarModule()
{
    delete d;
}

void KonqSidebarModule::openUrl(const QUrl &url)
{
    handleURL(url);
}

void KonqSidebarModule::openPreview(const KFileItemList &items)
{
    handlePreview(items);
}

void KonqSidebarModule::openPreviewOnMouseOver(const KFileItem &item)
{
    handlePreviewOnMouseOver(item);
}

void KonqSidebarModule::handlePreview(const KFileItemList & /*items*/) {}

void KonqSidebarModule::handlePreviewOnMouseOver(const KFileItem & /*items*/) {}

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
                                      KParts::NavigationExtension::PopupFlags flags,
                                      const KParts::NavigationExtension::ActionGroupMap &actionGroups)
{
    emit popupMenu(this, global, items, args, browserArgs, flags, actionGroups);
}
