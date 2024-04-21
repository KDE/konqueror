//  This file is part of the KDE project
//   SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>
// 
//   SPDX-License-Identifier: LGPL-2.0-or-later
//

#include "asyncselectorinterface.h"

AsyncSelectorInterface::~AsyncSelectorInterface()
{
}

AsyncSelectorInterface::QueryMethods AsyncSelectorInterface::supportedAsyncQueryMethods() const
{
    return None;
}

//Code for this class copied from kparts/selectorinterface.cpp (KF 5.110) written by David Faure <faure@kde.org>
class Q_DECL_HIDDEN AsyncSelectorInterface::ElementPrivate : public QSharedData
{
public:
    QString tag;
    QHash<QString, QString> attributes;
};

AsyncSelectorInterface::Element::Element()
    : d(new ElementPrivate)
{
}

AsyncSelectorInterface::Element::Element(const AsyncSelectorInterface::Element &other)
    : d(other.d)
{
}

AsyncSelectorInterface::Element::~Element()
{
}

bool AsyncSelectorInterface::Element::isNull() const
{
    return d->tag.isNull();
}

void AsyncSelectorInterface::Element::setTagName(const QString &tag)
{
    d->tag = tag;
}

QString AsyncSelectorInterface::Element::tagName() const
{
    return d->tag;
}

void AsyncSelectorInterface::Element::setAttribute(const QString &name, const QString &value)
{
    d->attributes[name] = value; // insert or replace
}

QStringList AsyncSelectorInterface::Element::attributeNames() const
{
    return d->attributes.keys();
}

QString AsyncSelectorInterface::Element::attribute(const QString &name, const QString &defaultValue) const
{
    return d->attributes.value(name, defaultValue);
}

bool AsyncSelectorInterface::Element::hasAttribute(const QString &name) const
{
    return d->attributes.contains(name);
}
