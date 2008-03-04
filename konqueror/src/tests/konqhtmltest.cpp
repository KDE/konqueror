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

#include <QScrollArea>
#include <qtest_kde.h>
#include <qtest_gui.h>

#include <konqmainwindow.h>
#include <konqviewmanager.h>
#include <konqview.h>

#include <QObject>

class KonqHtmlTest : public QObject
{
    Q_OBJECT
public:

private Q_SLOTS:
    void loadSimpleHtml()
    {
        KonqMainWindow mainWindow;
        // we specify the mimetype so that we don't have to wait for a KonqRun
        mainWindow.openUrl(0, KUrl("data:text/html, <p>Hello World</p>"), "text/html");
        KonqView* view = mainWindow.currentView();
        QVERIFY(view);
        QVERIFY(view->part());
        QVERIFY(QTest::kWaitForSignal(view, SIGNAL(viewCompleted(KonqView*)), 1000));
        QCOMPARE(view->serviceType(), QString("text/html"));

    }

    void rightClickClose() // #149736
    {
        QPointer<KonqMainWindow> mainWindow = new KonqMainWindow;
        // we specify the mimetype so that we don't have to wait for a KonqRun
        mainWindow->openUrl(0, KUrl(
                "data:text/html, <script type=\"text/javascript\">"
                "function closeMe() { window.close(); } "
                "document.onmousedown = closeMe; "
                "</script>"), QString("text/html"));
        QPointer<KonqView> view = mainWindow->currentView();
        QVERIFY(view);
        QVERIFY(view->part());
        QVERIFY(QTest::kWaitForSignal(view, SIGNAL(viewCompleted(KonqView*)), 5000));
        QWidget* widget = view->part()->widget();
        if ( QScrollArea* scrollArea = qobject_cast<QScrollArea*>(widget))
            widget = scrollArea->widget();
        qDebug() << "Clicking on" << widget;
        QTest::mousePress(widget, Qt::RightButton);
        qApp->processEvents();
        QVERIFY(!view); // deleted
        QVERIFY(!mainWindow); // the whole window gets deleted, in fact
    }

};


QTEST_KDEMAIN_WITH_COMPONENTNAME( KonqHtmlTest, GUI, "konqueror" )

#include "konqhtmltest.moc"
