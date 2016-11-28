/*
    Copyright (c) 2016 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License or ( at
    your option ) version 3 or, at the discretion of KDE e.V. ( which shall
    act as a proxy as in section 14 of the GPLv3 ), any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <webenginepart.h>
#include "webengine_testutils.h"

#include <KIO/Job>
#include <KParts/BrowserExtension>

#include <QTest>
#include <QObject>
#include <QSignalSpy>
#include <QWebEngineView>

class WebEnginePartApiTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void shouldHaveBrowserExtension();
    void shouldEmitStartedAndCompleted();
    void shouldEmitSetWindowCaption();
    void shouldEmitOpenUrlNotifyOnClick();

};

void WebEnginePartApiTest::initTestCase()
{
    qRegisterMetaType<KIO::Job *>(); // for the KParts started signal
}

void WebEnginePartApiTest::shouldHaveBrowserExtension()
{
    // GIVEN
    WebEnginePart part;

    // WHEN
    KParts::BrowserExtension *ext = KParts::BrowserExtension::childObject(&part);

    // THEN
    QVERIFY(ext);
}

void WebEnginePartApiTest::shouldEmitStartedAndCompleted()
{
    // GIVEN
    WebEnginePart part;
    QSignalSpy spyStarted(&part, &KParts::ReadOnlyPart::started);
    QSignalSpy spyCompleted(&part, SIGNAL(completed(bool)));
    QSignalSpy spySetWindowCaption(&part, &KParts::ReadOnlyPart::setWindowCaption);
    KParts::BrowserExtension *ext = KParts::BrowserExtension::childObject(&part);
    QSignalSpy spyOpenUrlNotify(ext, &KParts::BrowserExtension::openUrlNotify);
    const QUrl url(QStringLiteral("data:text/html, <p>Hello World</p>"));

    // WHEN
    part.openUrl(url);

    // THEN
    QVERIFY(spyStarted.wait());
    QVERIFY(spySetWindowCaption.wait());
    QCOMPARE(spySetWindowCaption.at(0).at(0).toUrl().toString(), url.toString());
    QVERIFY(spyCompleted.wait());
    QVERIFY(!spyCompleted.at(0).at(0).toBool());
    QVERIFY(spyOpenUrlNotify.isEmpty());
}

void WebEnginePartApiTest::shouldEmitSetWindowCaption()
{
    // GIVEN
    WebEnginePart part;
    QSignalSpy spyStarted(&part, &KParts::ReadOnlyPart::started);
    QSignalSpy spyCompleted(&part, SIGNAL(completed(bool)));
    QSignalSpy spySetWindowCaption(&part, &KParts::ReadOnlyPart::setWindowCaption);

    // WHEN opening a URL with a title tag
    part.openUrl(QUrl(QStringLiteral("data:text/html, <title>Custom Title</title><p>Hello World</p>")));

    // THEN
    QVERIFY(spyStarted.wait());
    QVERIFY(spyCompleted.wait());
    QVERIFY(!spyCompleted.at(0).at(0).toBool());
    QCOMPARE(spySetWindowCaption.count(), 2);
    QCOMPARE(spySetWindowCaption.at(1).at(0).toUrl().toString(), QStringLiteral("Custom Title"));
}

void WebEnginePartApiTest::shouldEmitOpenUrlNotifyOnClick()
{
    // GIVEN
    WebEnginePart part;
    QSignalSpy spyStarted(&part, &KParts::ReadOnlyPart::started);
    QSignalSpy spyCompleted(&part, SIGNAL(completed(bool)));
    QSignalSpy spySetWindowCaption(&part, &KParts::ReadOnlyPart::setWindowCaption);
    KParts::BrowserExtension *ext = KParts::BrowserExtension::childObject(&part);
    QSignalSpy spyOpenUrlNotify(ext, &KParts::BrowserExtension::openUrlNotify);
    const QString file = QFINDTESTDATA("data/page-with-link.html");
    QVERIFY(!file.isEmpty());
    const QUrl url = QUrl::fromLocalFile(file);
    part.openUrl(url);
    QVERIFY(spyCompleted.wait());
    QVERIFY(spyOpenUrlNotify.isEmpty());
    QWebEnginePage *page = part.view()->page();
    const QPoint pos = elementCenter(page, QStringLiteral("linkid")); // doesn't seem fully correct...
    part.widget()->show();
    spyCompleted.clear();

    // WHEN clicking on the link
    QTest::mouseClick(part.view()->focusProxy(), Qt::LeftButton, Qt::KeyboardModifiers(), pos);

    // THEN
    QVERIFY(spyCompleted.wait());
    QCOMPARE(spyOpenUrlNotify.count(), 1);
    QCOMPARE(part.url().fileName(), QStringLiteral("hello.html"));
}

QTEST_MAIN(WebEnginePartApiTest)
#include "webengine_partapi_test.moc"
