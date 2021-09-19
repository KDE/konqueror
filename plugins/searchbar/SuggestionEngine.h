/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2009 Fredy Yanardi <fyanardi@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SUGGESTIONENGINE_H
#define SUGGESTIONENGINE_H

#include <QObject>

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
    SuggestionEngine(const QString &engineName, QObject *parent = nullptr);

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

