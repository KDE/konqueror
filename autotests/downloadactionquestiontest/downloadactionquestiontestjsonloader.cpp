//  This file is part of the KDE project
//  SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
// 
//  SPDX-License-Identifier: LGPL-2.0-or-later

#include "downloadactionquestiontestjsonloader.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

using Action = DownloadActionQuestion::Action;
using Actions = DownloadActionQuestion::Actions;
using MaybeAction = std::optional<Action>;

DownloadActionQuestionTestJsonLoader::DownloadActionQuestionTestJsonLoader(const QString &jsonFile)
{
    QFile f(jsonFile);
    if (f.open(QFile::ReadOnly)) {
        m_rootObject = QJsonDocument::fromJson(f.readAll()).object();
        f.close();
    } else {
        qWarning() << "Can't open JSON file" << jsonFile;
        m_isValid = false;
        return;
    }

    if (m_rootObject.isEmpty()) {
        qWarning() << "Empty JSON OBJECT";
        m_isValid = false;
    }
}

void DownloadActionQuestionTestJsonLoader::reset()
{
    m_isValid = !m_rootObject.isEmpty();
}

QList<DownloadActionQuestionTestJsonLoader::RowData> DownloadActionQuestionTestJsonLoader::readData(const QString& name)
{
    if (!m_isValid) {
        return {};
    }

    QJsonValue val = m_rootObject.value(name);
    if (!val.isObject()) {
        qWarning() << "Root object is not an object";
        m_isValid = false;
        return {};
    }
    QJsonObject testBlock = val.toObject();

    QString embedKey = QStringLiteral("embed");
    QString openKey = QStringLiteral("open");
    if (!testBlock.contains(embedKey) && !testBlock.contains(openKey)) {
        qWarning() << "Root object doesn't contain neither the \"embed\" nor the \"open\" entry";
        m_isValid = false;
        return {};
    }

    Data result = readTestSection(testBlock, embedKey);
    if (!m_isValid) {
        return {};
    }

    result.append(readTestSection(testBlock, openKey));
    return m_isValid ? result : Data{};
}

DownloadActionQuestionTestJsonLoader::Data DownloadActionQuestionTestJsonLoader::readTestSection(const QJsonObject& container, const QString& name)
{
    if (!container.contains(name)) {
        return {};
    }

    bool embed = readOpenOrEmbed(name);

    QJsonValue val = container.value(name);
    if (!val.isArray()) {
        qWarning() << "The content of the" << name << "entry is not an array";
        m_isValid = false;
        return {};
    }

    QJsonArray arr = val.toArray();
    auto appendRow = [this, embed](Data rows, const QJsonValue &val){
        rows.append(readRow(val, embed));
        return rows;
    };
    Data res = std::accumulate(arr.constBegin(), arr.constEnd(), Data{}, appendRow);
    return m_isValid ? res : Data{};
}

QString DownloadActionQuestionTestJsonLoader::actionsToString(DownloadActionQuestion::Actions actions)
{
    QString res;
    if (actions & Action::Open) {
        res = QStringLiteral("Open");
    }
    if (actions & Action::Embed) {
        res.append(QStringLiteral("|Embed"));
    }
    if (actions & Action::Save) {
        res.append(QStringLiteral("|Save"));
    }
    if (res.startsWith('|')) {
        res = res.mid(1);
    }
    return res;
}

QString DownloadActionQuestionTestJsonLoader::captionForRow(const RowData &row)
{
    QString autoOpenString = row.openEntry.isEmpty() ? "Ask" : row.openEntry;
    QString autoEmbedString = row.embedEntry.isEmpty() ? "Ask" : row.embedEntry;
    QString actionsString = row.actions ? actionsToString(row.actions) : QStringLiteral("None");
    QString res("AutoOpen=%1, AutoEmbed=%2, Actions=%3, embeddable=%4");
    return res.arg(autoOpenString).arg(autoEmbedString).arg(actionsString).arg(row.embed ? "true" : "false");
}

DownloadActionQuestionTestJsonLoader::RowData DownloadActionQuestionTestJsonLoader::readRow(const QJsonValue& val, bool embed)
{
    if (!val.isArray()) {
        m_isValid = false;
        return {};
    }

    QJsonArray arr = val.toArray();
    //4 means the Force Dialog entry is missing and defaults to false
    int entriesCount = arr.count();
    if (entriesCount != 4 && entriesCount != 5) {
        qWarning() << "Invalid number of entries for data row. Expected 4 or 5 but got" << entriesCount;
        m_isValid = false;
        return {};
    }

    RowData res;
    res.embed = embed;
    std::optional<QString> openEntry = readDontAskEntry(arr.at(0));
    std::optional<QString> embedEntry = readDontAskEntry(arr.at(1));
    if (!(openEntry && embedEntry)) {
        m_isValid = false;
        qWarning() << "Either the embed or the open config settings are missing. Open:" << openEntry << "Embed:" << embedEntry;
        return {};
    }
    res.openEntry = openEntry.value();
    res.embedEntry = embedEntry.value();

    std::optional<Actions> actions = readActions(arr.at(2));
    if (!actions) {
        qWarning() << "Couldn't read actions from" << arr.at(2);
        m_isValid = false;
        return {};
    }
    res.actions = actions.value();

    if (entriesCount == 5) {
        QJsonValue force = arr.at(3);
        if (!force.isBool()) {
            qWarning() << "Invalid value for \"force\" entry: expected a boolean but got" << force;
            m_isValid = false;
            return {};
        }
        res.flag = force.toBool() ? DownloadActionQuestion::ForceDialog : DownloadActionQuestion::InlineDisposition;
    }

    QJsonValue expected = arr.at(entriesCount - 1);
    if (!expected.isString() && !expected.isNull()) {
        m_isValid = false;
        return {};
    }
    res.result = actionFromString(expected.toString());
    res.caption = captionForRow(res);

    return res;
}

MaybeAction DownloadActionQuestionTestJsonLoader::actionFromString(const QString& actionName)
{
    if (actionName == QStringLiteral("Cancel")) {
        return Action::Cancel;
    } else if (actionName == QStringLiteral("Save")) {
        return Action::Save;
    } else if (actionName == QStringLiteral("Open")) {
        return Action::Open;
    } else if (actionName == QStringLiteral("Embed")) {
        return Action::Embed;
    } else {
        return std::nullopt;
    }
}

bool DownloadActionQuestionTestJsonLoader::readOpenOrEmbed(const QString& value)
{
    return value == QStringLiteral("embed");
}

std::optional<QString> DownloadActionQuestionTestJsonLoader::readDontAskEntry(const QJsonValue& value)
{
    if (value.isNull()) {
        return QString();
    } else if (value.isBool()) {
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    } else {
        return std::nullopt;
    }
}

std::optional<Actions> DownloadActionQuestionTestJsonLoader::readActions(const QJsonValue& value)
{
    if (!value.isArray()) {
        return std::nullopt;
    }
    QJsonArray arr = value.toArray();
    QList<MaybeAction> actionList;
    std::transform(arr.constBegin(), arr.constEnd(), std::back_inserter(actionList), [](const QJsonValue &v){return actionFromString(v.toString());});
    std::optional<Actions> res = std::optional<Actions>(Action::Cancel);
    auto accumulator = [](std::optional<Actions> res, MaybeAction a) {
        return (a && res) ? std::make_optional(res.value() | a.value()) : std::nullopt;
    };
    return std::accumulate(actionList.constBegin(), actionList.constEnd(), res, accumulator);
}
