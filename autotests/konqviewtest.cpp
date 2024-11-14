/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2008 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <qtest_gui.h>
#include <QSignalSpy>
#include <konqmainwindow.h>
#include <konqview.h>

class KonqViewTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
    }

    void textThenHtml()
    {
        // This is test for the bug "embed katepart and then type a website URL -> loaded into katepart"
        // i.e. KonqView::changePart(), KonqView::ensureViewSupports()

        KonqMainWindow mainWindow;
        // we specify the mimetype so that we don't have to wait for a KonqRun
        KonqOpenURLRequest req; req.forceEmbed();
        mainWindow.openUrl(nullptr, QUrl(QStringLiteral("data:text/plain, Hello World")), QStringLiteral("text/plain"), req);
        KonqView *view = mainWindow.currentView();
        QVERIFY(view);
        QVERIFY(view->part());
        QSignalSpy spyCompleted(view, SIGNAL(viewCompleted(KonqView*)));
        QVERIFY(spyCompleted.wait(10000));
        QCOMPARE(view->type().mimetype().value(), QString("text/plain"));
        const QString firstService = view->service().pluginId();
        qDebug() << firstService;
        QVERIFY(view->supportsMimeType("text/html")); // it does, since that's a mimetype subclass

        // Now open HTML, as if we typed a URL in the location bar.
        KonqOpenURLRequest req2; req2.typedUrl = QStringLiteral("http://www.kde.org");
        mainWindow.openUrl(nullptr, QUrl(QStringLiteral("data:text/html, <p>Hello World</p>")), QStringLiteral("text/html"), req2);
        qDebug() << view->service().pluginId();
        QVERIFY(view->service().pluginId() != firstService);
    }

};

QTEST_MAIN(KonqViewTest)

#include "konqviewtest.moc"
