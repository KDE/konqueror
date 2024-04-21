/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2003 George Staikos <staikos@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "web_module.h"
#include <QAction>
#include <KIO/FavIconRequestJob>

#include <QFileInfo>
#include <QTimer>

#include <dom/html_inline.h>
#include <kdialog.h>
#include <kglobal.h>
#include <KLocalizedString>
#include <kpluginfactory.h>
#include <KParts/NavigationExtension>
#include <knameandurlinputdialog.h>

#include <QHBoxLayout>
#include <knuminput.h>

KHTMLSideBar::KHTMLSideBar()
    : KHTMLPart()
{
    setStatusMessagesEnabled(false);
    setMetaRefreshEnabled(true);
    setJavaEnabled(false);
    setPluginsEnabled(false);

    setFormNotification(KHTMLPart::Only);
    connect(this,
            SIGNAL(formSubmitNotification(const char*,QString,QByteArray,QString,QString,QString)),
            this,
            SLOT(formProxy(const char*,QString,QByteArray,QString,QString,QString))
           );

    _linkMenu = new KMenu(widget());

    QAction *openLinkAction = new QAction(i18n("&Open Link"), this);
    _linkMenu->addAction(openLinkAction);
    connect(openLinkAction, SIGNAL(triggered()), this, SLOT(loadPage()));

    QAction *openWindowAction = new QAction(i18n("Open in New &Window"), this);
    _linkMenu->addAction(openWindowAction);
    connect(openWindowAction, SIGNAL(triggered()), this, SLOT(loadNewWindow()));

    _menu = new KMenu(widget());

    QAction *reloadAction = new QAction(i18n("&Reload"), this);
    reloadAction->setIcon(QIcon::fromTheme("view-refresh"));
    _menu->addAction(reloadAction);
    connect(reloadAction, SIGNAL(triggered()), this, SIGNAL(reload()));

    QAction *autoReloadAction = new QAction(i18n("Set &Automatic Reload"), this);
    autoReloadAction->setIcon(QIcon::fromTheme("view-refresh"));
    _menu->addAction(autoReloadAction);
    connect(autoReloadAction, SIGNAL(triggered()), this, SIGNAL(setAutoReload()));

    connect(this, SIGNAL(popupMenu(QString,QPoint)),
            this, SLOT(showMenu(QString,QPoint)));

}

////

KonqSideBarWebModule::KonqSideBarWebModule(QWidget *parent, const KConfigGroup &configGroup)
    : KonqSidebarModule(parent, configGroup)
{
    _htmlPart = new KHTMLSideBar();
    _htmlPart->setAutoDeletePart(false);
    connect(_htmlPart, SIGNAL(reload()), this, SLOT(reload()));
    connect(_htmlPart, SIGNAL(completed()), this, SLOT(pageLoaded()));
    connect(_htmlPart,
            SIGNAL(setWindowCaption(QString)),
            this,
            SLOT(setTitle(QString)));
    connect(_htmlPart,
            SIGNAL(openUrlRequest(QString,KParts::OpenUrlArguments,BrowserArguments)),
            this,
            SLOT(urlClicked(QString,KParts::OpenUrlArguments,BrowserArguments)));
    connect(_htmlPart->browserExtension(),
            SIGNAL(openUrlRequest(QUrl,KParts::OpenUrlArguments,BrowserArguments)),
            this,
            SLOT(formClicked(QUrl,KParts::OpenUrlArguments,BrowserArguments)));
    connect(_htmlPart,
            SIGNAL(setAutoReload()), this, SLOT(setAutoReload()));
    connect(_htmlPart,
            SIGNAL(openUrlNewWindow(QString,KParts::OpenUrlArguments,BrowserArguments,KParts::WindowArgs)),
            this,
            SLOT(urlNewWindow(QString,KParts::OpenUrlArguments,BrowserArguments,KParts::WindowArgs)));
    connect(_htmlPart,
            SIGNAL(submitFormRequest(const char*,QString,QByteArray,QString,QString,QString)),
            this,
            SIGNAL(submitFormRequest(const char*,QString,QByteArray,QString,QString,QString)));

    reloadTimeout = configGroup.readEntry("Reload", 0);
    _url = QUrl(configGroup.readPathEntry("URL", QString()));
    _htmlPart->openUrl(_url);
    // Must load this delayed
    QTimer::singleShot(0, this, SLOT(loadFavicon()));
}

KonqSideBarWebModule::~KonqSideBarWebModule()
{
    delete _htmlPart;
    _htmlPart = 0L;
}

QWidget *KonqSideBarWebModule::getWidget()
{
    return _htmlPart->widget();
}

void KonqSideBarWebModule::setAutoReload()
{
    KDialog dlg(0);
    dlg.setModal(true);
    dlg.setCaption(i18nc("@title:window", "Set Refresh Timeout (0 disables)"));
    dlg.setButtons(KDialog::Ok | KDialog::Cancel);

    QWidget *hbox = new QWidget(&dlg);
    QHBoxLayout *hboxHBoxLayout = new QHBoxLayout(hbox);
    hboxHBoxLayout->setContentsMargins(0, 0, 0, 0);
    dlg.setMainWidget(hbox);

    KIntNumInput *mins = new KIntNumInput(hbox);
    hboxHBoxLayout->addWidget(mins);
    mins->setRange(0, 120);
    mins->setSuffix(ki18np(" minute", " minutes"));
    KIntNumInput *secs = new KIntNumInput(hbox);
    hboxHBoxLayout->addWidget(secs);
    secs->setRange(0, 59);
    secs->setSuffix(ki18np(" second", " seconds"));

    if (reloadTimeout > 0)  {
        int seconds = reloadTimeout / 1000;
        secs->setValue(seconds % 60);
        mins->setValue((seconds - secs->value()) / 60);
    }

    if (dlg.exec() == KDialog::Accepted) {
        int msec = (mins->value() * 60 + secs->value()) * 1000;
        reloadTimeout = msec;
        configGroup().writeEntry("Reload", reloadTimeout);
        reload();
    }
}

void KonqSideBarWebModule::handleURL(const QUrl &)
{
}

void KonqSideBarWebModule::urlNewWindow(const QString &url, const KParts::OpenUrlArguments &args, const BrowserArguments &browserArgs, const KParts::WindowArgs &windowArgs)
{
    emit createNewWindow(QUrl(url), args, browserArgs, windowArgs);
}

void KonqSideBarWebModule::urlClicked(const QString &url, const KParts::OpenUrlArguments &args, const BrowserArguments &browserArgs)
{
    emit openUrlRequest(QUrl(url), args, browserArgs);
}

void KonqSideBarWebModule::formClicked(const QUrl &url, const KParts::OpenUrlArguments &args, const BrowserArguments &browserArgs)
{
    _htmlPart->setArguments(args);
    _htmlPart->browserExtension()->setBrowserArguments(browserArgs);
    _htmlPart->openUrl(url);
}

void KonqSideBarWebModule::loadFavicon()
{
    const QString icon = KIO::favIconForUrl(_url);
    if (icon.isEmpty()) {
        KIO::FavIconRequestJob *job = new KIO::FavIconRequestJob(_url);
        connect(job, &KIO::FavIconRequestJob::result, this, [this, job](KJob *) {
            if (!job->error()) {
                loadFavicon();
            }
        });
        return;
    }

    emit setIcon(icon);

    if (icon != configGroup().readEntry("Icon", QString())) {
        configGroup().writeEntry("Icon", icon);
    }
}

void KonqSideBarWebModule::reload()
{
    _htmlPart->openUrl(_url);
}

void KonqSideBarWebModule::setTitle(const QString &title)
{
    qCDebug(SIDEBAR_LOG) << title;
    if (!title.isEmpty()) {
        emit setCaption(title);

        if (title != configGroup().readEntry("Name", QString())) {
            configGroup().writeEntry("Name", title);
        }
    }
}

void KonqSideBarWebModule::pageLoaded()
{
    if (reloadTimeout > 0) {
        QTimer::singleShot(reloadTimeout, this, SLOT(reload()));
    }
}

bool KHTMLSideBar::urlSelected(const QString &url, int button,
                               int state, const QString &_target,
                               const KParts::OpenUrlArguments &args,
                               const BrowserArguments &browserArgs)
{
    if (button == Qt::LeftButton) {
        if (_target.toLower() == "_self") {
            openUrl(completeURL(url));
        } else if (_target.toLower() == "_blank") {
            emit openUrlNewWindow(completeURL(url).url(), args);
        } else { // isEmpty goes here too
            emit openUrlRequest(completeURL(url).url(), args);
        }
        return true;
    }
    if (button == Qt::MiddleButton) {
        emit openUrlNewWindow(completeURL(url).url(),
                              args);
        return true;
    }
    // A refresh
    if (button == 0 && _target.toLower() == "_self") {
        openUrl(completeURL(url));
        return true;
    }
    return KHTMLPart::urlSelected(url, button, state, _target, args, browserArgs);
}

class KonqSidebarWebPlugin : public KonqSidebarPlugin
{
public:
    KonqSidebarWebPlugin(QObject *parent, const QVariantList &args)
        : KonqSidebarPlugin(parent, args) {}
    virtual ~KonqSidebarWebPlugin() {}

    virtual KonqSidebarModule *createModule(QWidget *parent,
                                            const KConfigGroup &configGroup,
                                            const QString &desktopname,
                                            const QVariant &unused)
    {
        Q_UNUSED(unused);
        Q_UNUSED(desktopname);
        return new KonqSideBarWebModule(parent, configGroup);
    }

    virtual QList<QAction *> addNewActions(QObject *parent,
                                           const QList<KConfigGroup> &existingModules,
                                           const QVariant &unused)
    {
        Q_UNUSED(unused);
        Q_UNUSED(existingModules);
        QAction *action = new QAction(parent);
        action->setText(i18nc("@action:inmenu Add", "Web Sidebar Module"));
        action->setIcon(QIcon::fromTheme("internet-web-browser"));
        return QList<QAction *>() << action;
    }

    virtual QString templateNameForNewModule(const QVariant &actionData,
            const QVariant &unused) const
    {
        Q_UNUSED(actionData);
        Q_UNUSED(unused);
        return QString::fromLatin1("websidebarplugin%1.desktop");
    }

    virtual bool createNewModule(const QVariant &actionData, KConfigGroup &configGroup,
                                 QWidget *parentWidget,
                                 const QVariant &unused)
    {
        Q_UNUSED(actionData);
        Q_UNUSED(unused);

        KNameAndUrlInputDialog dlg(i18nc("@label", "Name:"), i18nc("@label", "Path or URL:"), QUrl(), parentWidget);
        dlg.setWindowTitle(i18nc("@title:window", "Add web sidebar module"));
        if (!dlg.exec()) {
            return false;
        }

        configGroup.writeEntry("Type", "Link");
        configGroup.writeEntry("Icon", "internet-web-browser");
        configGroup.writeEntry("Name", dlg.name());
        configGroup.writeEntry("URL", dlg.url().url());
        configGroup.writeEntry("X-KDE-KonqSidebarModule", "konqsidebar_web");
        return true;
    }
};

K_PLUGIN_FACTORY(KonqSidebarWebPluginFactory, registerPlugin<KonqSidebarWebPlugin>();)

#include "web_module.moc"

