//  This file is part of the KDE project
//  SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
// 
//  SPDX-License-Identifier: LGPL-2.0-or-later

#include "actondownloadedfilebar.h"
#include "webenginepart.h"
#include "konq_urlactions.h"

#include <KLocalizedString>
#include <KFileItemActions>
#include <KParts/PartLoader>
#include <KStringHandler>

#include <QTimer>
#include <QMimeDatabase>
#include <QMenu>
#include <QAbstractButton>
#include <QResizeEvent>
#include <QLayout>

using namespace WebEngine;
using namespace Konq;

ActOnDownloadedFileBar::ActOnDownloadedFileBar(const QUrl &url, const QUrl& downloadUrl, WebEnginePart *part) :
    KMessageWidget({}, part->widget()),
    m_part{part},
    m_url{url},
    m_downloadUrl{downloadUrl},
    m_timer{new QTimer(this)}
{
    setMessageType(KMessageWidget::Positive);

    QMimeDatabase db;
    m_mimeType = db.mimeTypeForFile(downloadUrl.path()).name();

    setCloseButtonVisible(true);

    setupOpenAction();
    connect(m_openAction, &QAction::triggered, this, [this](){actOnChoice(UrlAction::Open, true, {});});
    setupEmbedAction(true);
    setupEmbedAction(false);
    if (m_embedActionNewTab) {
        connect(m_embedActionNewTab, &QAction::triggered, this, [this](){actOnChoice(UrlAction::Embed, true, {});});
    }
    if (m_embedActionHere) {
        connect(m_embedActionHere, &QAction::triggered, this, [this](){actOnChoice(UrlAction::Embed, false, {});});
    }

    connect(m_timer, &QTimer::timeout, this, [this](){animatedHide();});
    m_timer->setSingleShot(true);
    m_timer->start(5000);
}

ActOnDownloadedFileBar::~ActOnDownloadedFileBar() noexcept
{
}

void WebEngine::ActOnDownloadedFileBar::resizeEvent(QResizeEvent* event)
{
    KMessageWidget::resizeEvent(event);
    if (text().isEmpty()) {
        setElidedText();
    }
}

void WebEngine::ActOnDownloadedFileBar::setElidedText()
{
    //Heuristic algorithm to ensure the text doesn't exceed the window's length
    //Start from the width of the layout's contents rectangle
    int maxWidth = layout()->contentsRect().width();
    //Subtract the spacing between widgets multiplied by the number of buttons
    maxWidth -= layout()->spacing() * buttons().count();
    QList<QAbstractButton*>btns = buttons();
    //Subtract the total width of the buttons
    maxWidth -= std::accumulate(btns.constBegin(), btns.constEnd(), 0, [](int res, auto *b){return res + b->width();});
    //Reduce the resulting value by 20% to be on the safe side: better to truncate some characters
    //than to force the window to grow (a window growing past the edge of the screen could be disconcerting
    //for users)
    maxWidth *= 0.8;
    //Compute an approximate number of characters for each of the remote URL and the path
    //Dividing the maximum width by the average character width, removing the length of the (translated) string
    //"was saved as" and dividing the result in 2
    QFontMetrics fm(font());
    //This string won't be shown to the user: it is a part of the completed string created at the end of the function
    //which *will* be shown to the user. Here, we need it to determine how many characters it will require so that
    //we can subtract them from those available to the URL and path. Because of this, we need the translated version
    int maxPartChars = (maxWidth / fm.averageCharWidth() - i18nc("@label:part of the text: 'url' was saved as 'file'", "was saved as").length())/2;

    QUrl::FormattingOptions opts = QUrl::RemoveUserInfo;
    QString urlString = m_url.toDisplayString(opts);
    QString downloadString = m_downloadUrl.path();
    int maxUrlChars = maxPartChars;
    int maxDownloadChars = maxPartChars;

    //Attempt to allocate as much space as possible to URL and path: if one of the two is less than the
    //length available to it, correspondingly increase the length available to the other
    if (urlString.length() < maxPartChars) {
        maxDownloadChars += maxPartChars - urlString.length();
    } else if (downloadString.length() < maxPartChars) {
        maxUrlChars += maxPartChars - downloadString.length();
    }

    //Squeeze the URL while attempting to keep as much information as possible
    if (urlString.length() > maxUrlChars) {
        //If removing the query would give a short enough string, remove the right-most part of the query,
        //otherwise, remove all the query (as it often is not very informative for humans) and remove the middle
        //part of the remaining string, adding three dots at the end to show that the URL is also truncated to the right
        urlString = m_url.toDisplayString(opts | QUrl::RemoveQuery);
        if (urlString.length() < maxUrlChars) {
            urlString = KStringHandler::rsqueeze(m_url.toDisplayString(opts), maxUrlChars);
        } else {
            urlString = KStringHandler::csqueeze(m_url.toDisplayString(opts | QUrl::RemoveQuery), maxUrlChars) + QStringLiteral("...");
        }
    }

    //Remove the middle part of the path
    downloadString = KStringHandler::csqueeze(downloadString, maxDownloadChars);

    QString text = i18nc("@label location where a remote URL was downloaded", "<tt>%1</tt> was saved as <tt>%2</tt>",
                         urlString, downloadString);
    setText(text);
}

QList<QAbstractButton *> WebEngine::ActOnDownloadedFileBar::buttons() const
{
    return findChildren<QAbstractButton*>();
}

void WebEngine::ActOnDownloadedFileBar::setupOpenAction()
{
    m_openAction = new QAction(this);
    addAction(m_openAction);
    KService::List apps = KFileItemActions::associatedApplications(QStringList{m_mimeType});
    QMenu *menu = createOpenWithMenu(apps);
    connect(menu, &QMenu::triggered, this, [this](QAction *action){
        actOnChoice(Konq::UrlAction::Open, true, action ? action->data() : QVariant());
    });
    m_openAction->setMenu(menu);
    if (apps.isEmpty()) {
        m_openAction->setText(i18nc("@action:inmenu Open downloaded file choosing application", "Open With..."));
        m_openAction->setIcon(QIcon::fromTheme(QStringLiteral("document-open")));
    } else {
        const KService::Ptr service = apps.at(0);
        QString name = service->name().replace(QLatin1Char('&'), QLatin1String("&&"));
        m_openAction->setText(i18nc("@action:inmenu Open downloaded file", "Open"));
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
    QAction *openWith = new QAction(QIcon::fromTheme(QStringLiteral("document-open")), i18nc("@action:inmenu Open downloaded file choosing application", "Open With..."), this);
    actions.append(openWith);
    QMenu *menu = createMenu(actions);
    if (actions.count() > 1) {
        menu->insertSeparator(actions.last());
    }
    return menu;
}

void WebEngine::ActOnDownloadedFileBar::setupEmbedAction(bool newTab)
{
    QAction* &act = newTab ? m_embedActionNewTab : m_embedActionHere;
    QList<KPluginMetaData> parts = KParts::PartLoader::partsForMimeType(m_mimeType);
    if (parts.isEmpty()) {
        act = nullptr;
        return;
    }
    const KPluginMetaData md = parts.at(0);
    QString actionName = newTab ? i18nc("@action:button", "Show in new tab") : i18nc("@action:button", "Show here");
    act = new QAction(QIcon::fromTheme(md.iconName()), actionName, this);
    QMenu *menu = createEmbedWithMenu(parts);
    connect(menu, &QMenu::triggered, this, [this, newTab](QAction *action){
        actOnChoice(Konq::UrlAction::Embed, newTab, action ? action->data() : QVariant());
    });
    act->setMenu(menu);
    addAction(act);
}

QMenu* WebEngine::ActOnDownloadedFileBar::createEmbedWithMenu(const QList<KPluginMetaData> &parts)
{
    auto createAction = [this](const KPluginMetaData &md) {
        QString actionName(md.name().replace(QLatin1Char('&'), QLatin1String("&&")));
        actionName = i18nc("@action:inmenu", "Show &with %1", actionName);
        QAction *act = new QAction(this);
        act->setIcon(QIcon::fromTheme(md.iconName()));
        act->setText(actionName);
        act->setData(md.pluginId());
        return act;
    };
    QList<QAction*> actions;
    std::transform(parts.constBegin(), parts.constEnd(), std::back_inserter(actions), createAction);
    QMenu *menu = createMenu(actions);
    connect(menu, &QMenu::triggered, this, [this](QAction *action){
        actOnChoice(Konq::UrlAction::Embed, true, action ? action->data() : QVariant());
    });
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

void WebEngine::ActOnDownloadedFileBar::actOnChoice(Konq::UrlAction choice, bool newTab, const QVariant &data)
{
    if (!m_part) {
        return;
    }

    KParts::OpenUrlArguments args;
    args.setMimeType(m_mimeType);
    BrowserArguments bargs;
    if (data.isValid()) {
        if (choice == Konq::UrlAction::Embed) {
            bargs.setEmbedWith(data.toString());
        } else {
            bargs.setOpenWith(data.toString());
        }
    }
    bargs.setAllowedUrlActions(AllowedUrlActions{choice});
    bargs.setForcesNewWindow(newTab);
    bargs.setNewTab(newTab);
    m_part->browserExtension()->browserOpenUrlRequest(m_downloadUrl, args, bargs);
    animatedHide();
    deleteLater();
}
