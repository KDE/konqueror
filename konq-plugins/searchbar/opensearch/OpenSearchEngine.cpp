/*
 * Copyright 2009 Jakub Wieczorek <faw217@gmail.com>
 * Copyright 2009 Christian Franke <cfchris6@ts2server.com>
 * Copyright 2009 Fredy Yanardi <fyanardi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "OpenSearchEngine.h"

#include <QtCore/QRegExp>
#include <QtCore/QStringList>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>

OpenSearchEngine::OpenSearchEngine(QObject *)
    : m_scriptEngine(0)
{
}

OpenSearchEngine::~OpenSearchEngine()
{
    if (m_scriptEngine) {
        delete m_scriptEngine;
    }
}

QString OpenSearchEngine::parseTemplate(const QString &searchTerm, const QString &searchTemplate)
{
    QString result = searchTemplate;
    result.replace(QLatin1String("{count}"), QLatin1String("20"));
    result.replace(QLatin1String("{startIndex}"), QLatin1String("0"));
    result.replace(QLatin1String("{startPage}"), QLatin1String("0"));
    // TODO - get setting from KDE
    result.replace(QLatin1String("{language}"), QLatin1String("en-US"));
    result.replace(QLatin1String("{inputEncoding}"), QLatin1String("UTF-8"));
    result.replace(QLatin1String("{outputEncoding}"), QLatin1String("UTF-8"));
    result.replace(QLatin1String("{searchTerms}"), searchTerm);

    return result;
}

QString OpenSearchEngine::name() const
{
    return m_name;
}

void OpenSearchEngine::setName(const QString &name)
{
    m_name = name;
}

QString OpenSearchEngine::description() const
{
    return m_description;
}

void OpenSearchEngine::setDescription(const QString &description)
{
    m_description = description;
}

QString OpenSearchEngine::searchUrlTemplate() const
{
    return m_searchUrlTemplate;
}

void OpenSearchEngine::setSearchUrlTemplate(const QString &searchUrlTemplate)
{
    m_searchUrlTemplate = searchUrlTemplate;
}

KUrl OpenSearchEngine::searchUrl(const QString &searchTerm) const
{
    if (m_searchUrlTemplate.isEmpty()) {
        return KUrl();
    }

    KUrl retVal = KUrl::fromEncoded(parseTemplate(searchTerm, m_searchUrlTemplate).toUtf8());

    QList<Parameter>::const_iterator end = m_searchParameters.constEnd();
    QList<Parameter>::const_iterator i = m_searchParameters.constBegin();
    for (; i != end; ++i) {
        retVal.addQueryItem(i->first, parseTemplate(searchTerm, i->second));
    }

    return retVal;
}

bool OpenSearchEngine::providesSuggestions() const
{
    return !m_suggestionsUrlTemplate.isEmpty();
}

QString OpenSearchEngine::suggestionsUrlTemplate() const
{
    return m_suggestionsUrlTemplate;
}

void OpenSearchEngine::setSuggestionsUrlTemplate(const QString &suggestionsUrlTemplate)
{
    m_suggestionsUrlTemplate = suggestionsUrlTemplate;
}

KUrl OpenSearchEngine::suggestionsUrl(const QString &searchTerm) const
{
    if (m_suggestionsUrlTemplate.isEmpty()) {
        return KUrl();
    }

    KUrl retVal = KUrl::fromEncoded(parseTemplate(searchTerm, m_suggestionsUrlTemplate).toUtf8());

    QList<Parameter>::const_iterator end = m_suggestionsParameters.constEnd();
    QList<Parameter>::const_iterator i = m_suggestionsParameters.constBegin();
    for (; i != end; ++i) {
        retVal.addQueryItem(i->first, parseTemplate(searchTerm, i->second));
    }

    return retVal;
}

QList<OpenSearchEngine::Parameter> OpenSearchEngine::searchParameters() const
{
    return m_searchParameters;
}

void OpenSearchEngine::setSearchParameters(const QList<Parameter> &searchParameters)
{
    m_searchParameters = searchParameters;
}

QList<OpenSearchEngine::Parameter> OpenSearchEngine::suggestionsParameters() const
{
    return m_suggestionsParameters;
}

void OpenSearchEngine::setSuggestionsParameters(const QList<Parameter> &suggestionsParameters)
{
    m_suggestionsParameters = suggestionsParameters;
}

QString OpenSearchEngine::imageUrl() const
{
    return m_imageUrl;
}

void OpenSearchEngine::setImageUrl(const QString &imageUrl)
{
    m_imageUrl = imageUrl;
}

QImage OpenSearchEngine::image() const
{
    return m_image;
}

void OpenSearchEngine::setImage(const QImage &image)
{
    m_image = image;
}

bool OpenSearchEngine::isValid() const
{
    return (!m_name.isEmpty() && !m_searchUrlTemplate.isEmpty());
}

bool OpenSearchEngine::operator==(const OpenSearchEngine &other) const
{
    return (m_name == other.m_name
            && m_description == other.m_description
            && m_imageUrl == other.m_imageUrl
            && m_searchUrlTemplate == other.m_searchUrlTemplate
            && m_suggestionsUrlTemplate == other.m_suggestionsUrlTemplate
            && m_searchParameters == other.m_searchParameters
            && m_suggestionsParameters == other.m_suggestionsParameters);
}

bool OpenSearchEngine::operator<(const OpenSearchEngine &other) const
{
    return (m_name < other.m_name);
}

QStringList OpenSearchEngine::parseSuggestion(const QByteArray &resp)
{
    QString response(resp);
    response = response.trimmed();

    if (response.isEmpty()) {
        return QStringList();
    }

    if (!response.startsWith(QLatin1Char('[')) || !response.endsWith(QLatin1Char(']'))) {
        return QStringList();
    }

    if (!m_scriptEngine) {
        m_scriptEngine = new QScriptEngine();
    }

    // Evaluate the JSON response using QtScript.
    if (!m_scriptEngine->canEvaluate(response)) {
        return QStringList();
    }

    QScriptValue responseParts = m_scriptEngine->evaluate(response);

    if (!responseParts.property(1).isArray()) {
        return QStringList();
    }

    QStringList suggestionsList;
    qScriptValueToSequence(responseParts.property(1), suggestionsList);

    return suggestionsList;
}

