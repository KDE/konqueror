/*
    SPDX-FileCopyrightText: 2009 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef DOWNLOADACTIONQUESTIONTEST_H
#define DOWNLOADACTIONQUESTIONTEST_H

#include "downloadactionquestiontestjsonloader.h"

#include <KSharedConfig>
#include <KService>
#include <KPluginMetaData>

#include <QObject>

class QDialog;
class VisibilityChangeDetector;
class QSignalSpy;

class DownloadActionQuestionTest : public QObject
{
    Q_OBJECT
public:
    DownloadActionQuestionTest(QObject *parent=nullptr);
    ~DownloadActionQuestionTest() = default;
    static void initMain();

private:
    static QString dontAskAgainOpenConfigKey(const QString &mimetype);
    static QString dontAskAgainEmbedConfigKey(const QString &mimetype);
    static QString embeddableMimeType();
    static QString notEmbeddableMimeType();

    KConfigGroup dontAskAgainConfigGroup() const;
    KConfigGroup writeDontAskAgainConfigEntries(const QString &mimetype, const QString &open, const QString &embed);

    VisibilityChangeDetector* makeDialogAutoClosing(QDialog *dlg);

    QSignalSpy spyDialogVisibility(QDialog *dlg);

    void createDetermineActionTestColumns();
    void addDetermineActionTestDataFromJson(const QString &key);

    using ModifyQuestion = std::function<void(DownloadActionQuestion &question)>;
    void performDetermineActionTest(bool localUrl, const std::optional<KService::List> &apps = std::nullopt, const std::optional<QList<KPluginMetaData>> &parts = std::nullopt);

private Q_SLOTS:
    void init();
    void cleanup();

    void testAutoEmbed();

    void testDetermineActionForceDialog_data();
    void testDetermineActionForceDialog();

    void testDetermineActionNoAvailableAction_data();
    void testDetermineActionNoAvailableAction();

    void testDetermineActionAutoSave_data();
    void testDetermineActionAutoSave();

    void testDetermineActionAutoSaveLocalUrl_data();
    void testDetermineActionAutoSaveLocalUrl();

    void testDetermineActionAutoDisplay_data();
    void testDetermineActionAutoDisplay();

    void testDetermineActionAsk_data();
    void testDetermineActionAsk();

    void testDetermineActionNoPartsOrApps_data();
    void testDetermineActionNoPartsOrApps();

    void testAsk_data();
    void testAsk();

private:
    DownloadActionQuestionTestJsonLoader m_dataLoader;
    KSharedConfig::Ptr m_config;
    //Fake parts available for embeddable mimetype
    QList<KPluginMetaData> m_parts;
    //Fake services available for not embeddable mimetype
    KService::List m_services;
};

class VisibilityChangeDetector : public QObject
{
    Q_OBJECT
public:
    VisibilityChangeDetector(QWidget *target, QObject *parent = nullptr);
    bool eventFilter(QObject * watched, QEvent * event) override;

Q_SIGNALS:
    void visibilityChanged(QWidget *w, bool visible);
};

bool operator==(const KService &s1, const KService &s2);

#endif // DOWNLOADACTIONQUESTIONTEST_H
