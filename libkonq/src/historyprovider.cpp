/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2001 Carsten Pfeiffer <pfeiffer@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "historyprovider.h"

#include <QSet>

class HistoryProviderPrivate
{
public:
    HistoryProviderPrivate()
        : q(nullptr)
    {
    }

    ~HistoryProviderPrivate()
    {
        delete q;
    }

    QSet<QString> dict;
    HistoryProvider *q;
};

Q_GLOBAL_STATIC(HistoryProviderPrivate, historyProviderPrivate)

HistoryProvider *HistoryProvider::self()
{
    if (!historyProviderPrivate()->q) {
        new HistoryProvider;
    }

    return historyProviderPrivate()->q;
}

bool HistoryProvider::exists()
{
    return historyProviderPrivate()->q;
}

HistoryProvider::HistoryProvider(QObject *parent)
    : QObject(parent)
    , d(historyProviderPrivate)
{
    Q_ASSERT(!historyProviderPrivate()->q);
    historyProviderPrivate()->q = this;
    setObjectName(QStringLiteral("history provider"));
}

HistoryProvider::~HistoryProvider()
{
    if (!historyProviderPrivate.isDestroyed() && historyProviderPrivate()->q == this) {
        historyProviderPrivate()->q = nullptr;
    }
}

bool HistoryProvider::contains(const QString &item) const
{
    return d->dict.contains(item);
}

void HistoryProvider::insert(const QString &item)
{
    d->dict.insert(item);
    Q_EMIT inserted(item);
}

void HistoryProvider::remove(const QString &item)
{
    d->dict.remove(item);
}

void HistoryProvider::clear()
{
    d->dict.clear();
    Q_EMIT cleared();
}

#include "moc_historyprovider.cpp"
