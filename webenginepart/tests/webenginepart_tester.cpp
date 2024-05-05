/*
    SPDX-FileCopyrightText: 2009 Nokia Corporation and /or its subsidiary(-ies)
    SPDX-FileCopyrightText: 2006 George Staikos <staikos@kde.org>
    SPDX-FileCopyrightText: 2006 Dirk Mueller <mueller@kde.org>
    SPDX-FileCopyrightText: 2006 Zack Rusin <zack@kde.org>
    SPDX-FileCopyrightText: 2006 Simon Hausmann <hausmann@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include <KAboutData>
#include <QDebug>
#include <KUriFilter>
#include <KLineEdit>
#include <KPluginMetaData>
#include <webenginepart.h>
#include <profile.h>

#include <QJsonDocument>
#include <QInputDialog>

//#include <QUiLoader>
//#include <QWebEnginePage>
#include <QWebEngineView>
//#include <QWebFrame>
#include <QWebEngineSettings>
//#include <QWebElement>
//#include <QWebElementCollection>

#if !defined(QT_NO_PRINTER)
#include <QPrintPreviewDialog>
#endif

#include <QAction>
#include <QCompleter>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QProgressBar>
#include <QStatusBar>
#include <QStringListModel>
#include <QToolBar>
#include <QToolTip>
#include <QDir>
#include <QFile>
#include <QVector>
#include <QTextStream>
#include <QApplication>
#include <KLocalizedString>
#include <QCommandLineParser>


class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(const QString& url = QString()): currentZoom(100) {
        QJsonObject jo = QJsonDocument::fromJson(
            "{ \"KPlugin\": {\n"
            " \"Id\": \"webenginepart\",\n"
            " \"Name\": \"WebEngine\",\n"
            " \"Version\": \"0.1\"\n"
            "}\n}").object();
        KPluginMetaData metaData(jo, QString());
        view = new WebEnginePart(this, nullptr, metaData);
        setCentralWidget(view->widget());

        connect(view->view(), &QWebEngineView::loadFinished,
                this, &MainWindow::loadFinished);
        connect(view->view(), SIGNAL(titleChanged(QString)),
                this, SLOT(setWindowTitle(QString)));
        connect(view->view()->page(), SIGNAL(linkHovered(QString)),
                this, SLOT(showLinkHover(QString)));
        connect(view->view()->page(), SIGNAL(windowCloseRequested()), this, SLOT(deleteLater()));

        setupUI();

        QUrl qurl(KUriFilter::self()->filteredUri(url, QStringList() << QStringLiteral("kshorturifilter")));
        if (qurl.isValid()) {
            urlEdit->setText(qurl.toEncoded());
            view->openUrl(qurl);

            // the zoom values are chosen to be like in Mozilla Firefox 3
            zoomLevels << 30 << 50 << 67 << 80 << 90;
            zoomLevels << 100;
            zoomLevels << 110 << 120 << 133 << 150 << 170 << 200 << 240 << 300;
        }
    }

    QWebEnginePage* webPage() const {
        return view->view()->page();
    }

    QWebEngineView* webView() const {
        return view->view();
    }

protected slots:

    void changeLocation() {
        QUrl url (KUriFilter::self()->filteredUri(urlEdit->text(), QStringList() << QStringLiteral("kshorturifilter")));
        view->openUrl(url);
        view->view()->setFocus(Qt::OtherFocusReason);
    }

    void loadFinished() {
        urlEdit->setText(view->url().toString());

        QUrl::FormattingOptions opts;
        opts |= QUrl::RemoveScheme;
        opts |= QUrl::RemoveUserInfo;
        opts |= QUrl::StripTrailingSlash;
        QString s = view->url().toString(opts);
        s = s.mid(2);
        if (s.isEmpty())
            return;

        if (!urlList.contains(s))
            urlList += s;
        urlModel.setStringList(urlList);
    }

    void showLinkHover(const QString &link) {
        statusBar()->showMessage(link);
    }

    void zoomIn() {
        int i = zoomLevels.indexOf(currentZoom);
        Q_ASSERT(i >= 0);
        if (i < zoomLevels.count() - 1)
            currentZoom = zoomLevels[i + 1];

        view->view()->setZoomFactor(qreal(currentZoom)/100.0);
    }

    void zoomOut() {
        int i = zoomLevels.indexOf(currentZoom);
        Q_ASSERT(i >= 0);
        if (i > 0)
            currentZoom = zoomLevels[i - 1];

        view->view()->setZoomFactor(qreal(currentZoom)/100.0);
    }

    void resetZoom()
    {
       currentZoom = 100;
       view->view()->setZoomFactor(1.0);
    }

    void toggleZoomTextOnly(bool b)
    {
        Q_UNUSED(b);
//        view->view()->page()->settings()->setAttribute(QWebEngineSettings::ZoomTextOnly, b);
    }

    void print() {
#if !defined(QT_NO_PRINTER)
        QScopedPointer<QPrintPreviewDialog> dlg (new QPrintPreviewDialog(this));
        connect(dlg.data(), SIGNAL(paintRequested(QPrinter*)),
                view, SLOT(print(QPrinter*)));
        dlg->exec();
#endif
    }

    void setEditable(bool on) {
//        view->view()->page()->setContentEditable(on);
        formatMenuAction->setVisible(on);
    }

    void dumpHtml() {
        view->view()->page()->toHtml([](const QString& text) {
            qDebug() << "HTML: " << text;
        });
    }

    void selectElements() {
        bool ok;
        QString str = QInputDialog::getText(this, i18nc("input dialog window title for selecting html elements", "Select elements"),
                                            i18nc("input dialog text for selecting html elements", "Choose elements"), QLineEdit::Normal,
                                            QStringLiteral("a"), &ok);
        if (ok && !str.isEmpty()) {
            //QWebElementCollection collection = view->page()->mainFrame()->findAllElements(str);
            //const int count = collection.count();
            //for (int i=0; i < count; i++)
            //    collection.at(i).setStyleProperty("background-color", "yellow");
            //statusBar()->showMessage(i18np("%1 element selected", "%1 elements selected", count), 5000);
        }
    }

public slots:

    void newWindow(const QString &url = QString()) {
        MainWindow *mw = new MainWindow(url);
        mw->show();
    }

private:

    QVector<int> zoomLevels;
    int currentZoom;

    // create the status bar, tool bar & menu
    void setupUI() {
        progress = new QProgressBar(this);
        progress->setRange(0, 100);
        progress->setMinimumSize(100, 20);
        progress->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        progress->hide();
        statusBar()->addPermanentWidget(progress);

        connect(view->view(), SIGNAL(loadProgress(int)), progress, SLOT(show()));
        connect(view->view(), SIGNAL(loadProgress(int)), progress, SLOT(setValue(int)));
        connect(view->view(), SIGNAL(loadFinished(bool)), progress, SLOT(hide()));

        urlEdit = new KLineEdit(this);
        urlEdit->setSizePolicy(QSizePolicy::Expanding, urlEdit->sizePolicy().verticalPolicy());
        connect(urlEdit, SIGNAL(returnPressed()),
                SLOT(changeLocation()));
        QCompleter *completer = new QCompleter(this);
        urlEdit->setCompleter(completer);
        completer->setModel(&urlModel);

        QToolBar *bar = addToolBar(QStringLiteral("Navigation"));
        bar->addAction(view->view()->pageAction(QWebEnginePage::Back));
        bar->addAction(view->view()->pageAction(QWebEnginePage::Forward));
        bar->addAction(view->view()->pageAction(QWebEnginePage::Reload));
        bar->addAction(view->view()->pageAction(QWebEnginePage::Stop));
        bar->addWidget(urlEdit);

        QMenu *fileMenu = menuBar()->addMenu(i18n("&File"));
        QAction *newWindow = fileMenu->addAction(i18n("New Window"), this, SLOT(newWindow()));

        fileMenu->addAction(i18n("Print"), this, SLOT(print()));
        fileMenu->addAction(i18n("Close"), this, SLOT(close()));

        QMenu *editMenu = menuBar()->addMenu(i18n("&Edit"));
        editMenu->addAction(view->view()->pageAction(QWebEnginePage::Undo));
        editMenu->addAction(view->view()->pageAction(QWebEnginePage::Redo));
        editMenu->addSeparator();
        editMenu->addAction(view->view()->pageAction(QWebEnginePage::Cut));
        editMenu->addAction(view->view()->pageAction(QWebEnginePage::Copy));
        editMenu->addAction(view->view()->pageAction(QWebEnginePage::Paste));
        editMenu->addSeparator();
        QAction *setEditable = editMenu->addAction(i18n("Set Editable"), this, SLOT(setEditable(bool)));
        setEditable->setCheckable(true);

        QMenu *viewMenu = menuBar()->addMenu(i18n("&View"));
        viewMenu->addAction(view->view()->pageAction(QWebEnginePage::Stop));
        viewMenu->addAction(view->view()->pageAction(QWebEnginePage::Reload));
        viewMenu->addSeparator();
        QAction *zoomIn = viewMenu->addAction(i18n("Zoom &In"), this, SLOT(zoomIn()));
        QAction *zoomOut = viewMenu->addAction(i18n("Zoom &Out"), this, SLOT(zoomOut()));
        QAction *resetZoom = viewMenu->addAction(i18n("Reset Zoom"), this, SLOT(resetZoom()));
        QAction *zoomTextOnly = viewMenu->addAction(i18n("Zoom Text Only"), this, SLOT(toggleZoomTextOnly(bool)));
        zoomTextOnly->setCheckable(true);
        zoomTextOnly->setChecked(false);
        viewMenu->addSeparator();
        viewMenu->addAction(i18n("Dump HTML"), this, SLOT(dumpHtml()));

#if 0
        QMenu *formatMenu = new QMenu(i18n("F&ormat"));
        formatMenuAction = menuBar()->addMenu(formatMenu);
        formatMenuAction->setVisible(false);
        formatMenu->addAction(view->view()->pageAction(QWebEnginePage::ToggleBold));
        formatMenu->addAction(view->view()->pageAction(QWebEnginePage::ToggleItalic));
        formatMenu->addAction(view->view()->pageAction(QWebEnginePage::ToggleUnderline));
        QMenu *writingMenu = formatMenu->addMenu(i18n("Writing Direction"));
        writingMenu->addAction(view->view()->pageAction(QWebEnginePage::SetTextDirectionDefault));
        writingMenu->addAction(view->view()->pageAction(QWebEnginePage::SetTextDirectionLeftToRight));
        writingMenu->addAction(view->view()->pageAction(QWebEnginePage::SetTextDirectionRightToLeft));
#endif
        newWindow->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));
        view->view()->pageAction(QWebEnginePage::Back)->setShortcut(QKeySequence::Back);
        view->view()->pageAction(QWebEnginePage::Stop)->setShortcut(Qt::Key_Escape);
        view->view()->pageAction(QWebEnginePage::Forward)->setShortcut(QKeySequence::Forward);
        view->view()->pageAction(QWebEnginePage::Reload)->setShortcut(QKeySequence::Refresh);
        view->view()->pageAction(QWebEnginePage::Undo)->setShortcut(QKeySequence::Undo);
        view->view()->pageAction(QWebEnginePage::Redo)->setShortcut(QKeySequence::Redo);
        view->view()->pageAction(QWebEnginePage::Cut)->setShortcut(QKeySequence::Cut);
        view->view()->pageAction(QWebEnginePage::Copy)->setShortcut(QKeySequence::Copy);
        view->view()->pageAction(QWebEnginePage::Paste)->setShortcut(QKeySequence::Paste);
        zoomIn->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Plus));
        zoomOut->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Minus));
        resetZoom->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
//        view->view()->pageAction(QWebEnginePage::ToggleBold)->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));
//        view->view()->pageAction(QWebEnginePage::ToggleItalic)->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
//        view->view()->pageAction(QWebEnginePage::ToggleUnderline)->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_U));

        QMenu *toolsMenu = menuBar()->addMenu(i18n("&Tools"));
        toolsMenu->addAction(i18n("Select elements..."), this, SLOT(selectElements()));

    }

    WebEnginePart *view;
    KLineEdit *urlEdit;
    QProgressBar *progress;

    QAction *formatMenuAction;

    QStringList urlList;
    QStringListModel urlModel;
};

class URLLoader : public QObject
{
    Q_OBJECT
public:
    URLLoader(QWebEngineView* view, const QString& inputFileName)
        : m_view(view)
        , m_stdOut(stdout)
    {
        init(inputFileName);
    }

public slots:
    void loadNext()
    {
        QString qstr;
        if (getUrl(qstr)) {
            QUrl url(qstr, QUrl::StrictMode);
            if (url.isValid()) {
                m_stdOut << "Loading " << qstr << " ......" << Qt::endl;
                m_view->load(url);
            } else
                loadNext();
        } else
            disconnect(m_view, nullptr, this, nullptr);
    }

private:
    void init(const QString& inputFileName)
    {
        QFile inputFile(inputFileName);
        if (inputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&inputFile);
            QString line;
            while (true) {
                line = stream.readLine();
                if (line.isNull())
                    break;
                m_urls.append(line);
            }
        } else {
            qDebug() << "Can't open list file";
            exit(0);
        }
        m_index = 0;
        inputFile.close();
    }

    bool getUrl(QString& qstr)
    {
        if (m_index == m_urls.size())
            return false;

        qstr = m_urls[m_index++];
        return true;
    }

private:
    QVector<QString> m_urls;
    int m_index;
    QWebEngineView* m_view;
    QTextStream m_stdOut;
};


int main(int argc, char **argv)
{
    KAboutData about(QStringLiteral("KDELauncher"), i18n("KDELauncher"), QStringLiteral("0.0000013"));
    QApplication app(argc, argv);
    QCommandLineParser parser;
    KAboutData::setApplicationData(about);
    about.setupCommandLine(&parser);
    parser.process(app);
    about.processCommandLine(&parser);

    QString url = QStringLiteral("%1/%2").arg(QDir::homePath()).arg(QStringLiteral("index.html"));

    const QStringList args = app.arguments();

    if (args.contains(QStringLiteral("-r"))) {
        // robotized
        QString listFile = args.at(2);
        if (!(args.count() == 3) && QFile::exists(listFile)) {
            qDebug() << "Usage: KDELauncher -r listfile";
            exit(0);
        }
        MainWindow window;
        QWebEngineView *view = window.webView();
        URLLoader loader(view, listFile);
        QObject::connect(view, SIGNAL(loadFinished(bool)), &loader, SLOT(loadNext()));
        loader.loadNext();
        window.show();
        return app.exec();
    } else {
        if (args.count() > 1)
            url = args.at(1);

        MainWindow window(url);

        // Opens every given urls in new windows
        for (int i = 2; i < args.count(); i++)
            window.newWindow(args.at(i));

        window.show();
        return app.exec();
    }
}

#include "webenginepart_tester.moc"
