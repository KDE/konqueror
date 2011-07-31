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

#include "OpenSearchManager.h"

#include <QtCore/QFile>

#include <KDebug>
#include <KGlobal>
#include <KStandardDirs>
#include <KUrl>
#include <kio/scheduler.h>

#include "opensearch/OpenSearchEngine.h"
#include "opensearch/OpenSearchReader.h"
#include "opensearch/OpenSearchWriter.h"

OpenSearchManager::OpenSearchManager(QObject *parent)
    : QObject(parent)
    , m_activeEngine(0)
{
    m_state = IDLE;
}

OpenSearchManager::~OpenSearchManager() {
    qDeleteAll(m_enginesMap);
    m_enginesMap.clear();
}

void OpenSearchManager::setSearchProvider(const QString &searchProvider)
{
    m_activeEngine = 0;

    if (!m_enginesMap.contains(searchProvider)) {
        const QString fileName = KGlobal::dirs()->findResource("data", "konqueror/opensearch/" + searchProvider + ".xml");
        if (fileName.isEmpty()) {
            return;
        }
        QFile file(fileName);

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            kWarning(1202) << "Cannot open opensearch description file: " + fileName;
            return;
        }

        OpenSearchReader reader;
        OpenSearchEngine *engine = reader.read(&file);

        if (engine) {
            m_enginesMap.insert(searchProvider, engine);
        }
        else {
            return;
        }
    }

    m_activeEngine = m_enginesMap.value(searchProvider);
}

bool OpenSearchManager::isSuggestionAvailable()
{
    return m_activeEngine != 0;
}

void OpenSearchManager::addOpenSearchEngine(const KUrl &url, const QString &title)
{
    Q_UNUSED(title);

    m_jobData.clear();

    if (m_state != IDLE) {
        // TODO: cancel job
    }

    m_state = REQ_DESCRIPTION;
    KIO::TransferJob *job = KIO::get(url, KIO::NoReload, KIO::HideProgressInfo);
    connect(job, SIGNAL(data(KIO::Job*,QByteArray)),
            this, SLOT(dataReceived(KIO::Job*,QByteArray)));
    connect(job, SIGNAL(result(KJob*)), SLOT(jobFinished(KJob*)));
}

void OpenSearchManager::requestSuggestion(const QString &searchText)
{
    if (!m_activeEngine) {
        return;
    }

    if (m_state != IDLE) {
        // TODO: cancel job
    }
    m_state = REQ_SUGGESTION;

    KUrl url = m_activeEngine->suggestionsUrl(searchText);
    kDebug(1202) << "Requesting for suggestions: " << url.url();
    m_jobData.clear();
    KIO::TransferJob *job = KIO::get(url, KIO::NoReload, KIO::HideProgressInfo);
    connect(job, SIGNAL(data(KIO::Job*,QByteArray)),
            this, SLOT(dataReceived(KIO::Job*,QByteArray)));
    connect(job, SIGNAL(result(KJob*)), SLOT(jobFinished(KJob*)));
}

void OpenSearchManager::dataReceived(KIO::Job *job, const QByteArray &data)
{
    Q_UNUSED(job);
    m_jobData.append(data);
}

void OpenSearchManager::jobFinished(KJob *job)
{
    if (job->error()) {
        return; // just silently return
    }

    if (m_state == REQ_SUGGESTION) {
        const QStringList suggestionsList = m_activeEngine->parseSuggestion(m_jobData);
        kDebug(1202) << "Received suggestion from " << m_activeEngine->name() << ": " << suggestionsList;

        emit suggestionReceived(suggestionsList);
    }
    else if (m_state == REQ_DESCRIPTION) {
        OpenSearchReader reader;
        OpenSearchEngine *engine = reader.read(m_jobData);
        if (engine) {
            m_enginesMap.insert(engine->name(), engine);
            QString path = KGlobal::dirs()->findResource("data", "konqueror/opensearch/");
            QString fileName = trimmedEngineName(engine->name());
            QFile file(path + fileName + ".xml");
            OpenSearchWriter writer;
            writer.write(&file, engine);

            QString searchUrl = OpenSearchEngine::parseTemplate("\\{@}", engine->searchUrlTemplate());
            emit openSearchEngineAdded(engine->name(), searchUrl, fileName);
        }
        else {
            kFatal() << "Error while adding new open search engine";
        }
    }
}

QString OpenSearchManager::trimmedEngineName(const QString &engineName) const
{
    QString trimmed;
    QString::ConstIterator constIter = engineName.constBegin();
    while (constIter != engineName.constEnd()) {
        if (constIter->isSpace()) {
            trimmed.append('-');
        }
        else if (*constIter != '.') {
            trimmed.append(constIter->toLower());
        }
        constIter++;
    }

    return trimmed;
}

#include "OpenSearchManager.moc"

