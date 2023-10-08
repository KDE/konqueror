/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2009 Fredy Yanardi <fyanardi@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "SuggestionEngine.h"

#include <kservice.h>

#include "searchbar_debug.h"

SuggestionEngine::SuggestionEngine(const QString &engineName, QObject *parent)
    : QObject(parent),
      m_engineName(engineName)
{
    // First get the suggestion request URL for this engine
    KService::Ptr service = KService::serviceByDesktopPath(QStringLiteral("searchproviders/%1.desktop").arg(m_engineName));

    if (service) {
#if QT_VERSION_MAJOR < 6
        const QString suggestionURL = service->property(QStringLiteral("Suggest")).toString();
#else
        const QString suggestionURL = service->property<QString>(QStringLiteral("Suggest"));
#endif
        if (!suggestionURL.isNull() && !suggestionURL.isEmpty()) {
            m_requestURL = suggestionURL;
        } else {
            qCWarning(SEARCHBAR_LOG) << "Missing property [Suggest] for suggestion engine: " + m_engineName;
        }
    }
}

QString SuggestionEngine::requestURL() const
{
    return m_requestURL;
}

QString SuggestionEngine::engineName() const
{
    return m_engineName;
}

