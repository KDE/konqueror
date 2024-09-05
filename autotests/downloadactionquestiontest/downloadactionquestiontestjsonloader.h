//  This file is part of the KDE project
//  SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
// 
//  SPDX-License-Identifier: LGPL-2.0-or-later

#ifndef DOWNLOADACTIONQUESTIONTESTJSONLOADER_H
#define DOWNLOADACTIONQUESTIONTESTJSONLOADER_H

#include "downloadactionquestion.h"

#include <QString>
#include <QJsonObject>

class QJsonValue;

/*
 * Helper test class which reads the data for the tests for DownloadActionQuestion::determineAction from a JSON file
 * and converts it in a format suitable for QTest
 */
class DownloadActionQuestionTestJsonLoader
{
public:
    DownloadActionQuestionTestJsonLoader(const QString &jsonFile);
    struct RowData {
        QString openEntry;
        QString embedEntry;
        bool embed;
        DownloadActionQuestion::Actions actions;
        DownloadActionQuestion::EmbedFlags flag = DownloadActionQuestion::InlineDisposition;
        std::optional<DownloadActionQuestion::Action> result;
        QString caption;
    };

    using Data = QList<RowData>;
    bool isValid() const {return m_isValid;}

    /*
     * JSON data should have the following format:
     * {
     *   "caption": {
     *     "open": [
     *       [openEntry, embedEntry, actions, forceDialog, result],
     *       [openEntry, embedEntry, actions, forceDialog, result],
     *     ],
     *     "embed": [
     *       [openEntry, embedEntry, actions, forceDialog, result],
     *       [openEntry, embedEntry, actions, forceDialog, result],
     *     ],
     *   }
     * }
     *
     * Where:
     * - openEntry and embedEntry can be: true, false or null (null means no entry)
     * - actions is an array with the strings Cancel, Save, Open, Embed
     * - forceDialog can be true or false. It's optional and if it doesn't exist, it's considered false
     * - result can be one of the strings Save, Open, Embed or null (meaning std::nullopt)
     *
     * At least one of "open" or "embed" must exist
     */
    Data readData(const QString &name);

    void reset();

private:
    //NOTE: this assumes that value only contains the value "embed" or "open"
    static bool readOpenOrEmbed(const QString &value);
    static std::optional<QString> readDontAskEntry(const QJsonValue &value);
    static std::optional<DownloadActionQuestion::Action> actionFromString(const QString &actionName);
    static std::optional<DownloadActionQuestion::Actions> readActions(const QJsonValue &value);
    static QString actionsToString(DownloadActionQuestion::Actions actions);
    static QString captionForRow(const RowData &row);

    //A section means the "embed" or the "open" part
    Data readTestSection(const QJsonObject &container, const QString &name);
    RowData readRow(const QJsonValue &val, bool embed);

private:
    QJsonObject m_rootObject;
    bool m_isValid = true;
};

#endif // DOWNLOADACTIONQUESTIONTESTJSONLOADER_H
