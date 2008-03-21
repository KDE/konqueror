/* This file is part of the KDE project
   Copyright (C) 2008 David Faure <faure@kde.org>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <qtest_kde.h>
#include <konqmainwindow.h>
#include <konqview.h>

class KonqViewTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void textThenHtml()
    {
        // This is test for the bug "embed katepart and then type a website URL -> loaded into katepart"
        // i.e. KonqView::changePart(), KonqView::ensureViewSupports()

        KonqMainWindow mainWindow;
        // we specify the mimetype so that we don't have to wait for a KonqRun
        mainWindow.openUrl(0, KUrl("data:text/plain, Hello World"), "text/plain");
        KonqView* view = mainWindow.currentView();
        QVERIFY(view);
        QVERIFY(view->part());
        QVERIFY(QTest::kWaitForSignal(view, SIGNAL(viewCompleted(KonqView*)), 1000));
        QCOMPARE(view->serviceType(), QString("text/plain"));
        const QString firstService = view->service()->entryPath();
        qDebug() << firstService;
        QVERIFY(view->supportsMimeType("text/html")); // it does, since that's a mimetype subclass

        // Now open HTML, as if we typed a URL in the location bar.
        KonqOpenURLRequest req; req.typedUrl = "http://www.kde.org";
        mainWindow.openUrl(0, KUrl("data:text/html, <p>Hello World</p>"), "text/html", req);
        qDebug() << view->service()->entryPath();
        QVERIFY(view->service()->entryPath() != firstService);
    }

    void changePart()
    {
        // Related to the previous test; ensure we keep the same viewmode when switching between folders
        KonqMainWindow mainWindow;
        mainWindow.openUrl(0, KUrl(QDir::homePath()));
        KonqView* view = mainWindow.currentView();
        QVERIFY(view);
        QPointer<KParts::ReadOnlyPart> part = view->part();
        QVERIFY(view->part());
        QVERIFY(QTest::kWaitForSignal(view, SIGNAL(viewCompleted(KonqView*)), 1000));
        QCOMPARE(view->serviceType(), QString("inode/directory"));
        qDebug() << view->internalViewMode();
        view->setInternalViewMode("details");
        QCOMPARE(view->internalViewMode(), QString("details"));

        mainWindow.openUrl(0, KUrl("applications:/"));
        QCOMPARE(static_cast<KParts::ReadOnlyPart *>(part), view->part());
        QCOMPARE(view->internalViewMode(), QString("details"));
    }

};

QTEST_KDEMAIN_WITH_COMPONENTNAME( KonqViewTest, GUI, "konqueror" )

#include "konqviewtest.moc"
