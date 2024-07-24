/* This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Simon Hausmann <hausmann@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

// Own
#include "konqguiclients.h"

// KDE
#include <ktoggleaction.h>
#include "konqdebug.h"
#include <QIcon>
#include <kiconloader.h>
#include <KLocalizedString>

// Local
#include "konqview.h"
#include "konqsettings.h"
#include "konqframe.h"
#include "konqframevisitor.h"
#include "konqframestatusbar.h"
#include "konqviewmanager.h"
#include "pluginmetadatautils.h"
#include "konqutils.h"

PopupMenuGUIClient::PopupMenuGUIClient(const QVector<KPluginMetaData> &embeddingServices,
                                       KonqPopupMenu::ActionGroupMap &actionGroups,
                                       QAction *showMenuBar, QAction *stopFullScreen)
    : m_actionCollection(this),
      m_embeddingServices(embeddingServices)
{
    QList<QAction *> topActions;
    if (showMenuBar) {
        topActions.append(showMenuBar);
        QAction *separator = new QAction(&m_actionCollection);
        separator->setSeparator(true);
        topActions.append(separator);
    }

    if (stopFullScreen) {
        topActions.append(stopFullScreen);
        QAction *separator = new QAction(&m_actionCollection);
        separator->setSeparator(true);
        topActions.append(separator);
    }

    if (!embeddingServices.isEmpty()) {
        QList<QAction *> previewActions;
        if (embeddingServices.count() == 1) {
            KPluginMetaData service = embeddingServices.first();
            QAction *act = addEmbeddingPlugin(0, i18n("Preview &in %1", service.name()), service);
            previewActions.append(act);
        } else if (embeddingServices.count() > 1) {
            QVector<KPluginMetaData>::ConstIterator it = embeddingServices.begin();
            const QVector<KPluginMetaData>::ConstIterator end = embeddingServices.end();
            int idx = 0;
            for (; it != end; ++it, ++idx) {
                QAction *act = addEmbeddingPlugin(idx, (*it).name(), *it);
                previewActions.append(act);
            }
        }
        actionGroups.insert(KonqPopupMenu::PreviewActions, previewActions);
    }
    actionGroups.insert(KonqPopupMenu::TopActions, topActions);
}

PopupMenuGUIClient::~PopupMenuGUIClient()
{
}

QAction *PopupMenuGUIClient::addEmbeddingPlugin(int idx, const QString &name, const KPluginMetaData &plugin)
{
    QAction *act = m_actionCollection.addAction(QByteArray::number(idx));
    act->setText(name);
    act->setIcon(QIcon::fromTheme(plugin.iconName()));
    QObject::connect(act, &QAction::triggered, this, &PopupMenuGUIClient::slotOpenEmbedded);
    return act;
}

void PopupMenuGUIClient::slotOpenEmbedded()
{
    int idx = sender()->objectName().toInt();
    // This calls KonqMainWindow::slotOpenEmbedded(service) (delayed so that the menu is closed first)
    emit openEmbedded(m_embeddingServices.at(idx));
}

////

ToggleViewGUIClient::ToggleViewGUIClient(KonqMainWindow *mainWindow)
    : QObject(mainWindow)
{
    m_mainWindow = mainWindow;

    auto filter = [](const KPluginMetaData &md) {
        if (!Konq::serviceTypes(md).contains(QStringLiteral("Browser/View"))) {
            return false;
        }
        bool toggable = md.value(QStringLiteral("X-KDE-BrowserView-Toggable"), false);
        if (!toggable) {
            return false;
        }
        QString orientation = (QStringLiteral("X-KDE-BrowserView-ToggableView-Orientation"), QString());
        if (!orientation.isEmpty()) {
            return false;
        }
        return true;
    };
    QVector<KPluginMetaData> mds = findParts(filter, true);
    //TODO KF6: check whether this is still true
    //Sometimes there may be duplicates, so remove them
    std::sort(mds.begin(), mds.end(), [](const KPluginMetaData &md1, const KPluginMetaData &md2){return md1.pluginId() <= md2.pluginId();});
    auto last = std::unique(mds.begin(), mds.end(), [](const KPluginMetaData &md1, const KPluginMetaData &md2){return md1.pluginId() == md2.pluginId();});
    mds.erase(last, mds.end());

    m_empty = mds.isEmpty();
    if (m_empty) {
        return;
    }

    for (const KPluginMetaData &md : mds) {
        QString description = i18n("Show %1", md.name());
        QString name = md.pluginId();
        //qCDebug(KONQUEROR_LOG) << "ToggleViewGUIClient: name=" << name;
        KToggleAction *action = new KToggleAction(description, this);
        mainWindow->actionCollection()->addAction(name.toLatin1(), action);

        // HACK
        if (md.iconName() != QLatin1String("unknown")) {
            action->setIcon(QIcon::fromTheme(md.iconName()));
        }

        connect(action, &QAction::toggled, this, &ToggleViewGUIClient::slotToggleView);

        m_actions.insert(name, action);

        QVariant orientation = md.value(QStringLiteral("X-KDE-BrowserView-ToggableView-Orientation"));
        bool horizontal = orientation.toString().toLower() == QLatin1String("horizontal");
        m_mapOrientation.insert(name, horizontal);
    }

    connect(m_mainWindow, &KonqMainWindow::viewAdded, this, &ToggleViewGUIClient::slotViewAdded);
    connect(m_mainWindow, &KonqMainWindow::viewRemoved, this, &ToggleViewGUIClient::slotViewRemoved);
}

ToggleViewGUIClient::~ToggleViewGUIClient()
{
}

QList<QAction *> ToggleViewGUIClient::actions() const
{
    return m_actions.values();
}

void ToggleViewGUIClient::slotToggleView(bool toggle)
{
    QString serviceName = sender()->objectName();

    bool horizontal = m_mapOrientation[ serviceName ];

    KonqViewManager *viewManager = m_mainWindow->viewManager();

    if (toggle) {
        // Don't crash when doing things too quickly.
        if (!m_mainWindow->currentView()) {
            return;
        }
        KonqView *childView = viewManager->splitMainContainer(m_mainWindow->currentView(),
                              horizontal ? Qt::Vertical : Qt::Horizontal,
                              QStringLiteral("Browser/View"),
                              serviceName,
                              !horizontal /* vertical = make it first */);

        QList<int> newSplitterSizes;

        if (horizontal) {
            newSplitterSizes << 100 << 30;
        } else {
            newSplitterSizes << 30 << 100;
        }

        if (!childView || !childView->frame()) {
            return;
        }

        // Toggleviews don't need their statusbar
        childView->frame()->statusbar()->hide();

        KonqFrameContainerBase *newContainer = childView->frame()->parentContainer();

        if (newContainer->frameType() == KonqFrameBase::Container) {
            static_cast<KonqFrameContainer *>(newContainer)->setSizes(newSplitterSizes);
        }

#if 0 // already done by splitWindow
        if (m_mainWindow->currentView()) {
            QString locBarURL = m_mainWindow->currentView()->url().prettyUrl(); // default one in case it doesn't set it
            childView->openUrl(m_mainWindow->currentView()->url(), locBarURL);
        }
#endif

        // If not passive, set as active :)
        if (!childView->isPassiveMode()) {
            viewManager->setActivePart(childView->part());
        }

        qCDebug(KONQUEROR_LOG) << "ToggleViewGUIClient::slotToggleView setToggleView(true) on " << childView;
        childView->setToggleView(true);

        m_mainWindow->viewCountChanged();

    } else {
        const QList<KonqView *> viewList = KonqViewCollector::collect(m_mainWindow);
        for (KonqView *view: viewList) {
            if (view->service().pluginId() == serviceName)
                // takes care of choosing the new active view, and also calls slotViewRemoved
            {
                viewManager->removeView(view);
            }
        }
    }

}

void ToggleViewGUIClient::saveConfig(bool add, const QString &serviceName)
{
    // This is used on konqueror's startup....... so it's never used, since
    // the K menu only contains calls to kfmclient......
    // Well, it could be useful again in the future.
    // Currently, the profiles save this info.
    QStringList toggableViewsShown = Konq::Settings::toggableViewsShown();
    if (add) {
        if (!toggableViewsShown.contains(serviceName)) {
            toggableViewsShown.append(serviceName);
        }
    } else {
        toggableViewsShown.removeAll(serviceName);
    }
    Konq::Settings::setToggableViewsShown(toggableViewsShown);
}

void ToggleViewGUIClient::slotViewAdded(KonqView *view)
{
    QString name = view->service().pluginId();

    QAction *action = m_actions.value(name);

    if (action) {
        disconnect(action, &QAction::toggled, this, &ToggleViewGUIClient::slotToggleView);
        static_cast<KToggleAction *>(action)->setChecked(true);
        connect(action, &QAction::toggled, this, &ToggleViewGUIClient::slotToggleView);

        saveConfig(true, name);

        // KonqView::isToggleView() is not set yet.. so just check for the orientation

#if 0
        QVariant vert = view->service()->property("X-KDE-BrowserView-ToggableView-Orientation");
        bool vertical = vert.toString().toLower() == "vertical";
        QVariant nohead = view->service()->property("X-KDE-BrowserView-ToggableView-NoHeader");
        bool noheader = nohead.isValid() ? nohead.toBool() : false;
        // if it is a vertical toggle part, turn on the header.
        // this works even when konq loads the view from a profile.
        if (vertical && (!noheader)) {
            view->frame()->header()->setText(view->service()->name());
            view->frame()->header()->setAction(action);
        }
#endif
    }
}

void ToggleViewGUIClient::slotViewRemoved(KonqView *view)
{
    QString name = view->service().pluginId();

    QAction *action = m_actions.value(name);

    if (action) {
        disconnect(action, &QAction::toggled, this, &ToggleViewGUIClient::slotToggleView);
        static_cast<KToggleAction *>(action)->setChecked(false);
        connect(action, &QAction::toggled, this, &ToggleViewGUIClient::slotToggleView);
        saveConfig(false, name);
    }
}

