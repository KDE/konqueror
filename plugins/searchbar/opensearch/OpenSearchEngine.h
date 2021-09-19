/*
    SPDX-FileCopyrightText: 2009 Jakub Wieczorek <faw217@gmail.com>
    SPDX-FileCopyrightText: 2009 Christian Franke <cfchris6@ts2server.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OPENSEARCHENGINE_H
#define OPENSEARCHENGINE_H

#include <QPair>
#include <QImage>

#include <QUrl>

class QScriptEngine;

class OpenSearchEngine
{
public:
    typedef QPair<QString, QString> Parameter;

    OpenSearchEngine(QObject *parent = nullptr);
    ~OpenSearchEngine();

    QString name() const;
    void setName(const QString &name);

    QString description() const;
    void setDescription(const QString &description);

    QString searchUrlTemplate() const;
    void setSearchUrlTemplate(const QString &searchUrl);
    QUrl searchUrl(const QString &searchTerm) const;

    bool providesSuggestions() const;

    QString suggestionsUrlTemplate() const;
    void setSuggestionsUrlTemplate(const QString &suggestionsUrl);
    QUrl suggestionsUrl(const QString &searchTerm) const;

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
