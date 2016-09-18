/*
 * Copyright 2009 Jakub Wieczorek <faw217@gmail.com>
 * Copyright 2009 Christian Franke <cfchris6@ts2server.com>
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

#ifndef OPENSEARCHENGINE_H
#define OPENSEARCHENGINE_H

#include <QtCore/QPair>
#include <QImage>

#include <KUrl>

class QNetworkAccessManager;
class QNetworkReply;
class QScriptEngine;

class OpenSearchEngine
{
public:
    typedef QPair<QString, QString> Parameter;

    OpenSearchEngine(QObject *parent = 0);
    ~OpenSearchEngine();

    QString name() const;
    void setName(const QString &name);

    QString description() const;
    void setDescription(const QString &description);

    QString searchUrlTemplate() const;
    void setSearchUrlTemplate(const QString &searchUrl);
    KUrl searchUrl(const QString &searchTerm) const;

    bool providesSuggestions() const;

    QString suggestionsUrlTemplate() const;
    void setSuggestionsUrlTemplate(const QString &suggestionsUrl);
    KUrl suggestionsUrl(const QString &searchTerm) const;

    QList<Parameter> searchParameters() const;
    void setSearchParameters(const QList<Parameter> &searchParameters);

    QList<Parameter> suggestionsParameters() const;
    void setSuggestionsParameters(const QList<Parameter> &suggestionsParameters);

    QString imageUrl() const;
    void setImageUrl(const QString &url);

    QImage image() const;
    void setImage(const QImage &image);

    bool isValid() const;

    bool operator==(const OpenSearchEngine &other) const;
    bool operator<(const OpenSearchEngine &other) const;

    QStringList parseSuggestion(const QByteArray &response);

    static QString parseTemplate(const QString &searchTerm, const QString &searchTemplate);

private:
    QString m_name;
    QString m_description;

    QString m_imageUrl;
    QImage m_image;

    QString m_searchUrlTemplate;
    QString m_suggestionsUrlTemplate;
    QList<Parameter> m_searchParameters;
    QList<Parameter> m_suggestionsParameters;

    QScriptEngine *m_scriptEngine;
};

#endif // OPENSEARCHENGINE_H
