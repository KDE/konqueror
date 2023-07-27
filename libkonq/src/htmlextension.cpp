/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2010 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "htmlextension.h"

#include <KParts/ReadOnlyPart>

class HtmlExtensionPrivate
{
};

HtmlExtension::HtmlExtension(KParts::ReadOnlyPart *parent)
    : QObject(parent)
    , d(nullptr)
{
}

HtmlExtension::~HtmlExtension()
{
}

bool HtmlExtension::hasSelection() const
{
    return false;
}

HtmlExtension *HtmlExtension::childObject(QObject *obj)
{
    return obj->findChild<HtmlExtension *>(QString(), Qt::FindDirectChildrenOnly);
}

#include "moc_htmlextension.cpp"
