/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2010 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "textextension.h"

#include <KParts/ReadOnlyPart>

class TextExtensionPrivate
{
};

TextExtension::TextExtension(KParts::ReadOnlyPart *parent)
    : QObject(parent)
    , d(nullptr)
{
}

TextExtension::~TextExtension()
{
}

bool TextExtension::hasSelection() const
{
    return false;
}

QString TextExtension::selectedText(Format) const
{
    return QString();
}

QString TextExtension::completeText(Format) const
{
    return QString();
}

TextExtension *TextExtension::childObject(QObject *obj)
{
    return obj->findChild<TextExtension *>(QString(), Qt::FindDirectChildrenOnly);
}

int TextExtension::pageCount() const
{
    return 0;
}

int TextExtension::currentPage() const
{
    return 0;
}

QString TextExtension::pageText(Format) const
{
    return QString();
}

bool TextExtension::findText(const QString &, KFind::SearchOptions) const
{
    return false;
}

#include "moc_textextension.cpp"
