/*
    SPDX-FileCopyrightText: 2009 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "downloadactionquestiontest.h"

#include <KApplicationTrader>
#include <downloadactionquestion.h>
#include <qtest_widgets.h>

#include "konqembedsettings.h"

#include <KConfigGroup>
#include <KSharedConfig>
#include <KParts/PartLoader>

#include <QDebug>
#include <QDialog>
#include <QMenu>
#include <QPushButton>
#include <QWidget>
#include <QStandardPaths>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// SYNC - keep this in sync with downloadactionquestion.cpp
static const QString Save = QStringLiteral("saveButton");
static const QString OpenDefault = QStringLiteral("openDefaultButton");
static const QString OpenWith = QStringLiteral("openWithButton");
static const QString Cancel = QStringLiteral("cancelButton");
static const QString EmbedDefault = QStringLiteral("embedDefaultButton");

using Action = DownloadActionQuestion::Action;
using Actions = DownloadActionQuestion::Actions;
using MaybeAction = std::optional<Action>;
using DetermineActionResultMap = QHash<std::pair<QString, QString>, MaybeAction>;
using PartList = QList<KPluginMetaData>;

bool operator==(const KService& s1, const KService& s2)
{
    return s1.storageId() == s2.storageId();
}

VisibilityChangeDetector::VisibilityChangeDetector(QWidget *target, QObject *parent) : QObject(parent)
{
    if (target) {
        target->installEventFilter(this);
    }
}

bool VisibilityChangeDetector::eventFilter(QObject * watched, QEvent * event) {
    QWidget *w = qobject_cast<QWidget*>(watched);
    if (!w) {
        return false;
    }
    switch (event->type()) {
        case QEvent::Show:
            emit visibilityChanged(w, true);
            break;
        case QEvent::Hide:
            emit visibilityChanged(w, false);
            break;
        default:
            break;
    }
    return false;
}

void DownloadActionQuestionTest::initMain()
{
    QStandardPaths::setTestModeEnabled(true);
}

DownloadActionQuestionTest::DownloadActionQuestionTest(QObject *parent) : QObject(parent),
    m_dataLoader(QStringLiteral(":downloadactionquestion_testdata.json")),
    m_config(KSharedConfig::openConfig("filetypesrc", KConfig::NoGlobals))
{
}

QString DownloadActionQuestionTest::dontAskAgainOpenConfigKey(const QString &mimetype)
{
    return QLatin1String("askSave") + mimetype;
}

QString DownloadActionQuestionTest::dontAskAgainEmbedConfigKey(const QString &mimetype)
{
    return QLatin1String("askEmbedOrSave") + mimetype;
}

KConfigGroup DownloadActionQuestionTest::dontAskAgainConfigGroup() const
{
    return m_config->group("Notification Messages");
}

KConfigGroup DownloadActionQuestionTest::writeDontAskAgainConfigEntries(const QString &mimetype, const QString& open, const QString& embed)
{
    KConfigGroup grp = dontAskAgainConfigGroup();
    auto writeOrDeleteEntry = [&grp] (const QString &entry, const QString &value) {
        if (value.isEmpty()) {
            grp.deleteEntry(entry);
        } else {
            grp.writeEntry(entry, value);
        }
    };

    writeOrDeleteEntry(dontAskAgainOpenConfigKey(mimetype), open);
    writeOrDeleteEntry(dontAskAgainEmbedConfigKey(mimetype), embed);
    m_config->sync();
    return grp;
}

QString DownloadActionQuestionTest::embeddableMimeType() {
    return QStringLiteral("application/zip"); //This is embeddable by default
}

QString DownloadActionQuestionTest::notEmbeddableMimeType() {
    return QStringLiteral("application/pdf"); //This is not embeddable by default
}

VisibilityChangeDetector* DownloadActionQuestionTest::makeDialogAutoClosing(QDialog* dlg)
{
    VisibilityChangeDetector *detector = new VisibilityChangeDetector(dlg, dlg);

    //Automatically close the dialog, if it appears, simulating the Escape key
    auto closeDialog = [dlg] (QWidget*, bool visible) {
        if (visible) {
            QTest::keyClick(dlg, Qt::Key_Escape);
        }
    };
    connect(detector, &VisibilityChangeDetector::visibilityChanged, this, closeDialog, Qt::QueuedConnection);
    return detector;
}

QSignalSpy DownloadActionQuestionTest::spyDialogVisibility(QDialog* dlg)
{
    VisibilityChangeDetector *detector = dlg->findChild<VisibilityChangeDetector*>();
    if (!detector) {
        detector = makeDialogAutoClosing(dlg);
    }
    return QSignalSpy(detector, &VisibilityChangeDetector::visibilityChanged);
}

void DownloadActionQuestionTest::addDetermineActionTestDataFromJson(const QString& key)
{
    DownloadActionQuestionTestJsonLoader::Data rows = m_dataLoader.readData(key);
    if (!m_dataLoader.isValid()) {
        QSKIP("Invalid JSON data");
    }

    for (auto r : rows) {
        QTest::newRow(r.caption.toUtf8().constData()) << r.embed << r.openEntry << r.embedEntry << r.actions << r.flag << r.result;
    }
}

void DownloadActionQuestionTest::performDetermineActionTest(bool localUrl, const std::optional<KService::List> &apps, const std::optional<QList<KPluginMetaData>> &parts)
{
    QFETCH(bool, embeddable);
    QFETCH(QString, openEntry);
    QFETCH(QString, embedEntry);
    QFETCH(Actions, actions);
    QFETCH(DownloadActionQuestion::EmbedFlags, flag);
    QFETCH(MaybeAction, expected);

    const QString mimeType = embeddable ? embeddableMimeType() : notEmbeddableMimeType();

    auto partsToUse = parts ? parts.value() : m_parts;
    auto appsToUse = apps ? apps.value() : m_services;

    KConfigGroup grp = writeDontAskAgainConfigEntries(mimeType, openEntry, embedEntry);

    QUrl url = localUrl ? QUrl::fromLocalFile(QStringLiteral("/tmp/test")) : QUrl(QStringLiteral("https://www.example.com/test"));
    DownloadActionQuestion question(nullptr, url, mimeType, false);

    question.setApps(appsToUse);
    question.setParts(partsToUse);
    question.setupDialog(actions);
    MaybeAction res = question.determineAction(actions, flag, grp);
    QCOMPARE(res, expected);
}

void DownloadActionQuestionTest::init()
{
    QJsonObject obj;
    obj.insert("KPlugin", QJsonObject{{"Id", "part1"}, {"MimeTypes", QString("[%1]").arg(embeddableMimeType())}, {"Name", "part1"}});
    m_parts.append(KPluginMetaData(obj, {}));
    obj = {};
    obj.insert("KPlugin", QJsonObject{{"Id", "part2"}, {"MimeTypes", QString("[%1]").arg(embeddableMimeType())}, {"Name", "part2"}});
    m_parts.append(KPluginMetaData(obj, {}));

    KService::Ptr service = QExplicitlySharedDataPointer(new KService("service1", "/tmp/test1", {}));
    m_services.append(service);
    service = QExplicitlySharedDataPointer(new KService("service2", "/tmp/test2", {}));
    m_services.append(service);

    KConfigGroup grp = KonqFMSettings::settings()->fileTypesConfig()->group("EmbedSettings");
    grp.writeEntry(QLatin1String("embed-") + embeddableMimeType(), true);
    grp.writeEntry(QLatin1String("embed-") + notEmbeddableMimeType(), false);
    grp.sync();
    KonqFMSettings::settings()->reparseConfiguration();
    QVERIFY(KonqFMSettings::settings()->shouldEmbed(embeddableMimeType()) == true);
    QVERIFY(KonqFMSettings::settings()->shouldEmbed(notEmbeddableMimeType()) == false);
}

void DownloadActionQuestionTest::cleanup()
{
    m_dataLoader.reset();
    m_parts.clear();
    m_services.clear();

    QString config = QStandardPaths::locate(QStandardPaths::ConfigLocation, "filetypesrc");
    if (QFile::exists(config)) {
        QFile::remove(config);
    }
    m_config->reparseConfiguration();
}

void DownloadActionQuestionTest::testAutoEmbed()
{
    // This one should get the fast path, no dialog should show up.
    DownloadActionQuestion questionEmbedHtml(nullptr, QUrl(QStringLiteral("http://www.example.com/")), QString::fromLatin1("text/html"));
    questionEmbedHtml.setParts(m_parts);
    questionEmbedHtml.setApps(m_services);
    QCOMPARE(questionEmbedHtml.askEmbedOrSave(), Action::Embed);
}

void DownloadActionQuestionTest::createDetermineActionTestColumns()
{
    QTest::addColumn<bool>("embeddable");
    QTest::addColumn<QString>("openEntry");
    QTest::addColumn<QString>("embedEntry");
    QTest::addColumn<Actions>("actions");
    QTest::addColumn<DownloadActionQuestion::EmbedFlags>("flag");
    QTest::addColumn<MaybeAction>("expected");
}

void DownloadActionQuestionTest::testDetermineActionForceDialog_data()
{
    createDetermineActionTestColumns();
    addDetermineActionTestDataFromJson(QStringLiteral("testDetermineActionForceDialog"));
}

void DownloadActionQuestionTest::testDetermineActionForceDialog()
{
    performDetermineActionTest(false);
    performDetermineActionTest(true);
}

void DownloadActionQuestionTest::testDetermineActionNoAvailableAction_data()
{
    createDetermineActionTestColumns();
    addDetermineActionTestDataFromJson(QStringLiteral("testDetermineActionNoAvailableAction"));
}

void DownloadActionQuestionTest::testDetermineActionNoAvailableAction()
{
    performDetermineActionTest(false);
    performDetermineActionTest(true);
}

void DownloadActionQuestionTest::testDetermineActionAutoSave_data()
{
    createDetermineActionTestColumns();
    addDetermineActionTestDataFromJson(QStringLiteral("testDetermineActionAutoSave"));
}

void DownloadActionQuestionTest::testDetermineActionAutoSave()
{
    performDetermineActionTest(false);
}

void DownloadActionQuestionTest::testDetermineActionAutoSaveLocalUrl_data()
{
    createDetermineActionTestColumns();
    addDetermineActionTestDataFromJson(QStringLiteral("testDetermineActionAutoSaveLocalUrl"));
}

void DownloadActionQuestionTest::testDetermineActionAutoSaveLocalUrl()
{
    performDetermineActionTest(true);
}

void DownloadActionQuestionTest::testDetermineActionAutoDisplay_data()
{
    createDetermineActionTestColumns();
    addDetermineActionTestDataFromJson(QStringLiteral("testDetermineActionAutoDisplay"));
}

void DownloadActionQuestionTest::testDetermineActionAutoDisplay()
{
    performDetermineActionTest(false);
    performDetermineActionTest(true);
}

void DownloadActionQuestionTest::testDetermineActionAsk_data()
{
    createDetermineActionTestColumns();
    addDetermineActionTestDataFromJson(QStringLiteral("testDetermineActionAsk"));
}

void DownloadActionQuestionTest::testDetermineActionAsk()
{
    performDetermineActionTest(false);
}

void DownloadActionQuestionTest::testDetermineActionNoPartsOrApps_data()
{
    createDetermineActionTestColumns();
    addDetermineActionTestDataFromJson(QStringLiteral("testDetermineActionNoPartsOrApps"));
}

void DownloadActionQuestionTest::testDetermineActionNoPartsOrApps()
{
    performDetermineActionTest(false, std::make_optional(KService::List{}), std::make_optional(PartList{}));
    performDetermineActionTest(true, std::make_optional(KService::List{}), std::make_optional(PartList{}));
}

void DownloadActionQuestionTest::testAsk_data()
{
    QTest::addColumn<bool>("embeddable");
    QTest::addColumn<QString>("mimeType");
    QTest::addColumn<QString>("autoAction");
    QTest::addColumn<Action>("expectedResult");

    QString mimeType = embeddableMimeType();
    PartList parts = KParts::PartLoader::partsForMimeType(mimeType);
    if (!parts.isEmpty()) {
        QTest::newRow("Embed") << true << mimeType << QStringLiteral("false") << Action::Embed;
        QTest::newRow("Save") << true << mimeType << QStringLiteral("true") << Action::Save;
        QTest::newRow("Dialog") << true << mimeType << QStringLiteral() << Action::Cancel;
    } else {
        QSKIP((QString("This test assumes there's at least one part for %1").arg(embeddableMimeType())).toUtf8().constData());
    }

    mimeType = notEmbeddableMimeType();
    KService::List apps = KApplicationTrader::queryByMimeType(mimeType);
    if (!apps.isEmpty()) {
        QTest::newRow("Open") << false << mimeType << QStringLiteral("false") << Action::Open;
    } else {
        QSKIP((QString("This test assumes there's at least one application for %1").arg(notEmbeddableMimeType())).toUtf8().constData());
    }
}

void DownloadActionQuestionTest::testAsk()
{
    QFETCH(bool, embeddable);
    QFETCH(QString, mimeType);
    QFETCH(QString, autoAction);
    QFETCH(Action, expectedResult);

    KPluginMetaData expectedPart = expectedResult == Action::Embed ? m_parts.first() : KPluginMetaData{};
    KService::Ptr expectedApp = expectedResult == Action::Open ? m_services.first() : KService::Ptr{};

    writeDontAskAgainConfigEntries(mimeType, embeddable ? QString() : autoAction, embeddable ? autoAction : QString());
    DownloadActionQuestion question(nullptr, QUrl(QStringLiteral("https://www.example.com/test")), mimeType);
    question.setApps(m_services);
    question.setParts(m_parts);
    makeDialogAutoClosing(question.dialog());
    QSignalSpy spy = spyDialogVisibility(question.dialog());
    Action res = question.ask();
    QCOMPARE(res, expectedResult);
    if (expectedResult != Action::Cancel) {
        QVERIFY2(spy.count() == 0, "The dialog shouldn't have been shown but it was");
    } else {
        QCOMPARE_GE(spy.count(), 1);
    }
    QCOMPARE(question.selectedPart(), expectedPart);

    KService::Ptr selectedApp = question.selectedService();
    if (expectedApp && selectedApp) {
        QCOMPARE(*selectedApp, *expectedApp);
    } else {
        QCOMPARE(question.selectedService(), expectedApp);
    }
}

QTEST_MAIN(DownloadActionQuestionTest)
