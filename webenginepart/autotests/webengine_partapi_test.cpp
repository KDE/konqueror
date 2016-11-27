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

#include <QTest>
#include <QObject>
#include <QSignalSpy>
#include <webenginepart.h>
#include <KIO/Job>

class WebEnginePartApiTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void shouldEmitStartedAndCompleted();
    void shouldEmitSetWindowCaption();

};

void WebEnginePartApiTest::initTestCase()
{
    qRegisterMetaType<KIO::Job *>(); // for the KParts started signal
}

void WebEnginePartApiTest::shouldEmitStartedAndCompleted()
{
    // GIVEN
    WebEnginePart part;
    QSignalSpy spyStarted(&part, &KParts::ReadOnlyPart::started);
    QSignalSpy spyCompleted(&part, SIGNAL(completed(bool)));
    QSignalSpy spySetWindowCaption(&part, &KParts::ReadOnlyPart::setWindowCaption);
    const QUrl url(QStringLiteral("data:text/html, <p>Hello World</p>"));

    // WHEN
    part.openUrl(url);

    // THEN
    QVERIFY(spyStarted.wait());
    QVERIFY(spySetWindowCaption.wait());
    QCOMPARE(spySetWindowCaption.at(0).at(0).toUrl().toString(), url.toString());
    QVERIFY(spyCompleted.wait());
    QVERIFY(!spyCompleted.at(0).at(0).toBool());
}

void WebEnginePartApiTest::shouldEmitSetWindowCaption()
{
    // GIVEN
    WebEnginePart part;
    QSignalSpy spyStarted(&part, &KParts::ReadOnlyPart::started);
    QSignalSpy spyCompleted(&part, SIGNAL(completed(bool)));
    QSignalSpy spySetWindowCaption(&part, &KParts::ReadOnlyPart::setWindowCaption);

    // WHEN
    part.openUrl(QUrl(QStringLiteral("data:text/html, <title>Custom Title</title><p>Hello World</p>")));

    // THEN
    QVERIFY(spyStarted.wait());
    QVERIFY(spyCompleted.wait());
    QVERIFY(!spyCompleted.at(0).at(0).toBool());
    QCOMPARE(spySetWindowCaption.count(), 2);
    QCOMPARE(spySetWindowCaption.at(1).at(0).toUrl().toString(), QStringLiteral("Custom Title"));
}


QTEST_MAIN(WebEnginePartApiTest)
#include "webengine_partapi_test.moc"
