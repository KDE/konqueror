/* This file is part of the KDE project
 * Copyright (C) 2009 Fredy Yanardi <fyanardi@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "SuggestionEngine.h"

#include <KDebug>
#include <KUrl>
#include <kservice.h>

#include <QXmlStreamReader>

SuggestionEngine::SuggestionEngine(const QString &engineName, QObject *parent)
    : QObject(parent),
    m_engineName(engineName)
{
    // First get the suggestion request URL for this engine
    KService::Ptr service = KService::serviceByDesktopPath(QString("searchproviders/%1.desktop").arg(m_engineName));

    if (service) {
        const QString suggestionURL = service->property("Suggest").toString();
        if (!suggestionURL.isNull() && !suggestionURL.isEmpty()) {
            m_requestURL = suggestionURL;
        }
        else {
            kWarning(1202) << "Missing property [Suggest] for suggestion engine: " + m_engineName;
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

#include "SuggestionEngine.moc"

