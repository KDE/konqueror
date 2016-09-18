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

#ifndef SUGGESTIONENGINE_H
#define SUGGESTIONENGINE_H

#include <QtCore/QObject>

/**
 * Parent class for all suggestion engines. Each suggestion engine is responsible for
 * parsing the reply from the suggestion provider (host)
 */
class SuggestionEngine : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor.
     * @param engineName the engine name
     */
    SuggestionEngine(const QString &engineName, QObject *parent = 0);

    /**
     * Get the request URL for the suggestion service
     */
    QString requestURL() const;

    /**
     * Get the engine name for this engine
     */
    QString engineName() const;

    /**
     * To be reimplemented by subclass. Parse the suggestion reply to a QStringList
     */
    virtual QStringList parseSuggestion(const QByteArray &response) const = 0;

protected:
    QString m_engineName;
    QString m_requestURL;
};

#endif // SUGGESTIONENGINE_H

