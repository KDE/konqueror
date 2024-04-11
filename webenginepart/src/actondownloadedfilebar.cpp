//  This file is part of the KDE project
//  SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
// 
//  SPDX-License-Identifier: LGPL-2.0-or-later

#include "actondownloadedfilebar.h"
#include "webenginepart.h"

#include <KLocalizedString>
#include <KFileItemActions>
#include <KParts/PartLoader>

#include <QTimer>
#include <QMimeDatabase>
#include <QMenu>

using namespace WebEngine;

ActOnDownloadedFileBar::ActOnDownloadedFileBar(const QUrl &url, const QUrl& downloadUrl, WebEnginePart *part) :
    KMessageWidget(i18nc("@label location where a remote URL was downloaded", "%1 was saved in %2", url.toDisplayString(), downloadUrl.toDisplayString()), part->widget()),
    m_part{part},
    m_downloadUrl{downloadUrl},
    m_openAction{new QAction(this)},
    m_embedActionHere{new QAction(i18n("Show file in this tab"), this)},
    m_embedActionNewTab{new QAction(i18n("Show file in a new tab in Konqueror"), this)},
    m_timer{new QTimer(this)}
{
    setMessageType(KMessageWidget::Positive);
    setCloseButtonVisible(true);
    addAction(m_openAction);
    addAction(m_embedActionNewTab);
    addAction(m_embedActionHere);
    connect(m_openAction, &QAction::triggered, this, [this](){actOnChoice(Open, true, {});});
    connect(m_embedActionNewTab, &QAction::triggered, this, [this](){actOnChoice(Embed, true, {});});
    connect(m_embedActionHere, &QAction::triggered, this, [this](){actOnChoice(Embed, false, {});});

    QMimeDatabase db;
    m_mimeType = db.mimeTypeForFile(downloadUrl.path()).name();
    setupOpenAction();
    setupEmbedAction(m_embedActionHere);
    setupEmbedAction(m_embedActionNewTab);

    connect(m_timer, &QTimer::timeout, this, [this](){animatedHide();});
    m_timer->setSingleShot(true);
    m_timer->start(5000);
}

ActOnDownloadedFileBar::~ActOnDownloadedFileBar() noexcept
{
}

void WebEngine::ActOnDownloadedFileBar::setupOpenAction()
{
    KService::List apps = KFileItemActions::associatedApplications(QStringList{m_mimeType});
    QMenu *menu = createOpenWithMenu(apps);
    connect(menu, &QMenu::triggered, this, [this](QAction *action){actOnChoice(Open, true, action ? action->data() : QVariant());});
    m_openAction->setMenu(menu);
    if (apps.isEmpty()) {
        m_openAction->setText(i18n("Open in external application"));
        m_openAction->setIcon(QIcon::fromTheme(QStringLiteral("document-open")));
    } else {
        const KService::Ptr service = apps.at(0);
        QString name = service->name().replace(QLatin1Char('&'), QLatin1String("&&"));
        m_openAction->setText(i18n("Open in %1", name));
        m_openAction->setIcon(QIcon::fromTheme(service->icon()));
    }
}

QMenu* WebEngine::ActOnDownloadedFileBar::createOpenWithMenu(const KService::List &apps)
{
    auto createAction = [this](const KService::Ptr service) {
        QString actionName(service->name().replace(QLatin1Char('&'), QLatin1String("&&")));
        actionName = i18nc("@action:inmenu", "Open &with %1", actionName);

        QAction *act = new QAction(this);
        act->setIcon(QIcon::fromTheme(service->icon()));
        act->setText(actionName);
        act->setData(service->storageId());
        return act;
    };

    QList<QAction*> actions;
    std::transform(apps.constBegin(), apps.constEnd(), std::back_inserter(actions), createAction);
    QMenu *menu = createMenu(actions);
    return menu;
}

void WebEngine::ActOnDownloadedFileBar::setupEmbedAction(QAction* embedAction)
{
#if QT_VERSION_MAJOR < 6
    QList<KPluginMetaData> parts = KParts::PartLoader::partsForMimeType(m_mimeType).toList();
#else
    QList<KPluginMetaData> parts = KParts::PartLoader::partsForMimeType(m_mimeType);
#endif
    QMenu *menu = createEmbedWithMenu(parts);
    bool newTab = embedAction == m_embedActionNewTab;
    connect(menu, &QMenu::triggered, this, [this, newTab](QAction *action){actOnChoice(Embed, newTab, action ? action->data() : QVariant());});
    embedAction->setMenu(menu);
    if (parts.isEmpty()) {
        embedAction->setText(newTab ? i18nc("@action:button", "Show in new tab") : i18nc("@action:button", "Show here"));
    } else {
        const KPluginMetaData md = parts.at(0);
        QString name = md.name().replace(QLatin1Char('&'), QLatin1String("&&"));
        embedAction->setText(newTab ? i18nc("@action:button", "Show in new tab using %1", name) : i18nc("@action:button", "Show here using %1", name));
        embedAction->setIcon(QIcon::fromTheme(md.iconName()));
    }
}

QMenu* WebEngine::ActOnDownloadedFileBar::createEmbedWithMenu(const QList<KPluginMetaData> &parts)
{
    auto createAction = [this](const KPluginMetaData &md) {
        QString actionName(md.name().replace(QLatin1Char('&'), QLatin1String("&&")));
        actionName = i18nc("@action:inmenu", "Open &with %1", actionName);
        QAction *act = new QAction(this);
        act->setIcon(QIcon::fromTheme(md.iconName()));
        act->setText(actionName);
        act->setData(md.pluginId());
        return act;
    };
    QList<QAction*> actions;
    std::transform(parts.constBegin(), parts.constEnd(), std::back_inserter(actions), createAction);
    QMenu *menu = createMenu(actions);
    connect(menu, &QMenu::triggered, this, [this](QAction *action){actOnChoice(Embed, true, action ? action->data() : QVariant());});
    return menu;
}

QMenu* WebEngine::ActOnDownloadedFileBar::createMenu(const QList<QAction*> &actions)
{
    if (actions.length() < 2) {
        return nullptr;
    }

    QMenu *menu = new QMenu(this);
    menu->addActions(actions);
    connect(menu, &QMenu::aboutToShow, this, [this](){m_timer->stop();});
    connect(menu, &QMenu::aboutToHide, this, [this](){m_timer->start();});
    return menu;
}

void WebEngine::ActOnDownloadedFileBar::actOnChoice(Choice choice, bool newTab, const QVariant &data)
{
    if (!m_part) {
        return;
    }

    KParts::OpenUrlArguments args;
    args.setMimeType(m_mimeType);
    QString choiceName = choice == Embed ? QStringLiteral("embed") : QStringLiteral("open");
    if (data.isValid()) {
        QString key = choice == Embed ? QStringLiteral("embed-with") : QStringLiteral("open-with");
        args.metaData().insert(key, data.toString());
    }
    args.metaData().insert(QStringLiteral("action"), choiceName);
    BrowserArguments bargs;
    bargs.setForcesNewWindow(newTab);
    bargs.setNewTab(newTab);
#if QT_VERSION_MAJOR < 6
    m_part->browserExtension()->openUrlRequest(m_downloadUrl, args, bargs);
#else
    m_part->browserExtension()->browserOpenUrlRequest(m_downloadUrl, args, bargs);
#endif
    animatedHide();
    deleteLater();
}
