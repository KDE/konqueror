/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "profile.h"

#include <QStringLiteral>
#include <QApplication>

using namespace KonqWebEnginePart;

Profile::Profile(const QString &storageName, QObject* parent) : QWebEngineProfile(storageName, parent)
{
}

Profile::~Profile()
{
}

Profile* Profile::defaultProfile()
{
    static Profile *s_profile = new Profile(QStringLiteral("Default"), qApp);
    return s_profile;
}
