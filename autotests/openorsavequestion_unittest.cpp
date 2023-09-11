/*
    SPDX-FileCopyrightText: 2009 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include <KApplicationTrader>
#include <browseropenorsavequestion.h>
#include <qtest_widgets.h>

#include <KConfigGroup>
#include <KSharedConfig>
#include <QDebug>

#include <QDialog>
#include <QMenu>
#include <QPushButton>
#include <QWidget>

// SYNC - keep this in sync with browseropenorsavequestion.cpp
static const QString Save = QStringLiteral("saveButton");
static const QString OpenDefault = QStringLiteral("openDefaultButton");
static const QString OpenWith = QStringLiteral("openWithButton");
static const QString Cancel = QStringLiteral("cancelButton");

class OpenOrSaveTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testAutoEmbed()
    {
        // This one should get the fast path, no dialog should show up.
        BrowserOpenOrSaveQuestion questionEmbedHtml(nullptr, QUrl(QStringLiteral("http://www.example.com/")), QString::fromLatin1("text/html"));
        QCOMPARE(questionEmbedHtml.askEmbedOrSave(), BrowserOpenOrSaveQuestion::Embed);
    }
    void testDontAskAgain()
    {
        KSharedConfig::Ptr cfg = KSharedConfig::openConfig(QStringLiteral("filetypesrc"), KConfig::NoGlobals);
        cfg->group("Notification Messages")
            .writeEntry(
                "askSave"
                "text/plain",
                "false");
        BrowserOpenOrSaveQuestion question(nullptr, QUrl(QStringLiteral("http://www.example.com/")), QString::fromLatin1("text/plain"));
        QCOMPARE((int)question.askOpenOrSave(), (int)BrowserOpenOrSaveQuestion::Open);
        cfg->group("Notification Messages")
            .writeEntry(
                "askSave"
                "text/plain",
                "true");
        QCOMPARE((int)question.askOpenOrSave(), (int)BrowserOpenOrSaveQuestion::Save);
        cfg->group("Notification Messages")
            .deleteEntry(
                "askSave"
                "text/plain");
    }

    void testAllChoices_data()
    {
        qRegisterMetaType<QDialog *>("QDialog*");

        QTest::addColumn<QString>("mimetype");
        QTest::addColumn<QString>("buttonName");
        QTest::addColumn<int>("expectedResult");
        QTest::addColumn<bool>("expectedService");

        // For this test, we rely on the fact that:
        // 1. there is at least one app associated with application/zip,
        // 2. there is at least one app associated with text/plain, and
        // 3. there are no apps associated with application/x-zerosize.

        if (KApplicationTrader::queryByMimeType(QStringLiteral("application/zip")).count() > 0) {
            QTest::newRow("(zip) cancel") << "application/zip" << Cancel << (int)BrowserOpenOrSaveQuestion::Cancel << false;
            QTest::newRow("(zip) open default app") << "application/zip" << OpenDefault << (int)BrowserOpenOrSaveQuestion::Open << true;
            QTest::newRow("(zip) open with...") << "application/zip" << OpenWith << (int)BrowserOpenOrSaveQuestion::Open << false;
            QTest::newRow("(zip) save") << "application/zip" << Save << (int)BrowserOpenOrSaveQuestion::Save << false;
        } else {
            qWarning() << "This test relies on the fact that there is at least one app associated with appliation/zip.";
        }

        if (KApplicationTrader::queryByMimeType(QStringLiteral("text/plain")).count() > 0) {
            QTest::newRow("(text) cancel") << "text/plain" << Cancel << (int)BrowserOpenOrSaveQuestion::Cancel << false;
            QTest::newRow("(text) open default app") << "text/plain" << OpenDefault << (int)BrowserOpenOrSaveQuestion::Open << true;
            QTest::newRow("(text) open with...") << "text/plain" << OpenWith << (int)BrowserOpenOrSaveQuestion::Open << false;
            QTest::newRow("(text) save") << "text/plain" << Save << (int)BrowserOpenOrSaveQuestion::Save << false;
        } else {
            qWarning() << "This test relies on the fact that there is at least one app associated with text/plain.";
        }

        if (KApplicationTrader::queryByMimeType(QStringLiteral("application/x-zerosize")).count() == 0) {
            QTest::newRow("(zero) cancel") << "application/x-zerosize" << Cancel << (int)BrowserOpenOrSaveQuestion::Cancel << false;
            QTest::newRow("(zero) open with...") << "application/x-zerosize" << OpenDefault /*Yes, not OpenWith*/ << (int)BrowserOpenOrSaveQuestion::Open
                                                 << false;
            QTest::newRow("(zero) save") << "application/x-zerosize" << Save << (int)BrowserOpenOrSaveQuestion::Save << false;
        } else {
            qWarning() << "This test relies on the fact that there are no apps associated with application/x-zerosize.";
        }
    }

    void testAllChoices()
    {
        QFETCH(QString, mimetype);
        QFETCH(QString, buttonName);
        QFETCH(int, expectedResult);
        QFETCH(bool, expectedService);

        QWidget parent;
        BrowserOpenOrSaveQuestion questionEmbedZip(&parent, QUrl(QStringLiteral("http://www.example.com/")), mimetype);
        questionEmbedZip.setFeatures(BrowserOpenOrSaveQuestion::ServiceSelection);
        QDialog *theDialog = parent.findChild<QDialog *>();
        QVERIFY(theDialog);
        // QMetaObject::invokeMethod(theDialog, "slotButtonClicked", Qt::QueuedConnection, Q_ARG(int, button));
        QMetaObject::invokeMethod(this, "clickButton", Qt::QueuedConnection, Q_ARG(QDialog *, theDialog), Q_ARG(QString, buttonName));
        QCOMPARE((int)questionEmbedZip.askOpenOrSave(), expectedResult);
        QCOMPARE((bool)questionEmbedZip.selectedService(), expectedService);
    }

protected Q_SLOTS: // our own slots, not tests
    void clickButton(QDialog *dialog, const QString &buttonName)
    {
        QPushButton *button = dialog->findChild<QPushButton *>(buttonName);
        Q_ASSERT(button);
        Q_ASSERT(!button->isHidden());
        if (button->menu()) {
            Q_ASSERT(buttonName == OpenWith); // only this one has a menu
            button->menu()->actions().last()->trigger();
        } else {
            button->click();
        }
    }
};

QTEST_MAIN(OpenOrSaveTest)

#include "openorsavequestion_unittest.moc"
