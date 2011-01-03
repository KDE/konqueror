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

#ifndef OPENSEARCHMANAGER_H
#define OPENSEARCHMANAGER_H

#include <QtCore/QObject>
#include <kio/jobclasses.h>

class SuggestionEngine;

class OpenSearchEngine;

/**
 * This class acts as a proxy between the SearchBar plugin and the individual suggestion engine.
 * This class has a map of all available engines, and route the suggestion request to the correct engine
 */
class OpenSearchManager : public QObject
{
    Q_OBJECT

    enum STATE {
        REQ_SUGGESTION,
        REQ_DESCRIPTION,
        IDLE
    };
public:
    /**
     * Constructor
     */
    explicit OpenSearchManager(QObject *parent = 0);

    virtual ~OpenSearchManager();

    void setSearchProvider(const QString &searchProvider);

    /**
     * Check whether a search suggestion engine is available for the given search provider
     * @param searchProvider the queried search provider
     */
    bool isSuggestionAvailable();

    void addOpenSearchEngine(const KUrl &url, const QString &title);

public slots:
    /**
     * Ask the specific suggestion engine to request for suggestion for the search text
     * @param searchProvider the search provider that provides the suggestion service
     * @param searchText the text to be queried to the suggestion service
     */
    void requestSuggestion(const QString &searchProvider);

private slots:
    void dataReceived(KIO::Job *job, const QByteArray &data);
    void jobFinished(KJob *job);

signals:
    void suggestionReceived(const QStringList &suggestion);
    void openSearchEngineAdded(const QString &name, const QString &searchUrl, const QString &fileName);

private:
    QString trimmedEngineName(const QString &engineName) const;

    // QString substitutueSearchText(const QString &searchText, const QString &requestURL) const;
    QByteArray m_jobData;
    QMap<QString, OpenSearchEngine*> m_enginesMap;
    OpenSearchEngine *m_activeEngine;
    STATE m_state;
};

#endif // OPENSEARCHMANAGER_H

