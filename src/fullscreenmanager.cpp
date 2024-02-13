// This file is part of the KDE project
// SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "fullscreenmanager.h"
#include "konqmainwindow.h"
#include "konqguiclients.h"
#include "konqview.h"
#include "konqframestatusbar.h"
#include "konqviewmanager.h"
#include "konqframevisitor.h"
#include "konqtabs.h"

#include <KToggleAction>
#include <KToggleFullScreenAction>
#include <KToolBar>

#include <QMenuBar>

FullScreenManager::~FullScreenManager()
{
}

FullScreenManager::FullScreenManager(KonqMainWindow* parent) : QObject(parent)
{
}

KonqMainWindow * FullScreenManager::mainWindow() const
{
    return qobject_cast<KonqMainWindow*>(parent());
}

QAction* FullScreenManager::action(const QString &name)
{
#if QT_VERSION_MAJOR < 6
    return mainWindow()->actionCollection()->action(name);
#else
    return mainWindow() ->action(name);
#endif
}

void FullScreenManager::init()
{
    m_currentState = mainWindow()->fullScreenMode() ? FullScreenState::OrdinaryFullScreen : FullScreenState::NoFullScreen;
    m_previousState = m_currentState;
    m_wasStatusBarVisible = action(QStringLiteral("options_show_statusbar"))->isChecked();
    m_wasMenuBarVisible = action(QStringLiteral("options_show_menubar"))->isChecked();
}

void FullScreenManager::fullscreenToggleActionTriggered(bool on)
{
    if (on) {
        if (m_currentState == FullScreenState::NoFullScreen) {
            switchToState(FullScreenState::OrdinaryFullScreen);
        }
    } else {
        switch (m_currentState) {
            case FullScreenState::CompleteFullScreen:
                switchToState(m_previousState);
                return;
            case FullScreenState::OrdinaryFullScreen:
                switchToState(FullScreenState::NoFullScreen);
                return;
            case FullScreenState::NoFullScreen:
                return;
        }
    }
}

void FullScreenManager::toggleCompleteFullScreen(bool on)
{
    if (on && m_currentState != FullScreenState::CompleteFullScreen) {
        switchToState(FullScreenState::CompleteFullScreen);
    } else if (!on && m_currentState == FullScreenState::CompleteFullScreen) {
        switchToState(m_previousState);
    }
}

void FullScreenManager::switchToState(FullScreenState newState)
{
    if (newState == m_currentState) {
        return;
    }

    if (newState == FullScreenState::CompleteFullScreen) {
        mainWindow()->forceSaveMainWindowSettings();
        mainWindow()->resetAutoSaveSettings();
    } else {
        mainWindow()->setAutoSaveSettings();
    }
    toggleMenuBar(newState);
    if (newState == FullScreenState::OrdinaryFullScreen) {
        toggleFullScreenToolbar(true);
    } else if (newState == FullScreenState::NoFullScreen) {
        toggleFullScreenToolbar(false);
    }

    //WARNING: toggleStatusBar needs to be called before calling toggleViewGUIClients, otherwise
    //status bar gets messed up
    toggleStatusBar(newState);
    toggleToggableViews(newState);
    toggleToolbars(newState);
    mainWindow()->viewManager()->forceHideTabBar(newState == FullScreenState::CompleteFullScreen);

    m_previousState = m_currentState;
    m_currentState = newState;

    KToggleFullScreenAction::setFullScreen(mainWindow(), newState != FullScreenState::NoFullScreen);
}

void FullScreenManager::hideOtherViews(FullScreenState newState)
{
    if (newState == m_currentState) {
        return;
    }
    //Views should only be hidden or shown when switching from or switching to complete full screen, so do
    //nothing in all other cases
    if (newState != FullScreenState::CompleteFullScreen && m_currentState != FullScreenState::CompleteFullScreen) {
        return;
    }

    KonqMainWindow *mw = mainWindow();
    KonqView *current = mw->currentView();
    QList<KonqView*> views = KonqViewCollector::collect(mw->viewManager()->tabContainer()->currentTab());
    bool visible = m_currentState == FullScreenState::CompleteFullScreen;
    for (KonqView *v : views) {
        if (v != current) {
            v->frame()->setVisible(visible);
        }
    }
}

void FullScreenManager::toggleMenuBar(FullScreenState newState)
{
    if (newState == m_currentState) {
        return;
    }
    KonqMainWindow *mw = mainWindow();
    KToggleAction *a = qobject_cast<KToggleAction*>(action("options_show_menubar"));
    Q_ASSERT(a);
    //Shouldn't calling a->setChecked automatically hide or show the menu bar? It seems it doesn't
    if (newState == FullScreenState::NoFullScreen) {
        a->setChecked(m_wasMenuBarVisible);
        mw->menuBar()->setVisible(m_wasMenuBarVisible);
    } else {
        //Only store the menu bar visibility when the current state is NoFullScreen:
        //in all other cases, menu bar is always disabled
        if (m_currentState == FullScreenState::NoFullScreen) {
            m_wasMenuBarVisible = a->isChecked();
        }
        a->setChecked(false);
        mw->menuBar()->hide();
    }
}

void FullScreenManager::toggleFullScreenToolbar(bool on)
{
    // Create toolbar button for exiting from full-screen mode
    // ...but only if there isn't one already...
    KonqMainWindow *mw = mainWindow();
    if (on) {
        QList<KToolBar*> toolbars = mw->findChildren<KToolBar *>();
        QAction *exitFullScreenAction = action(QStringLiteral("fullscreen"));
        auto findExitFullScreenToolbar = [exitFullScreenAction](KToolBar *bar) {
            return bar->isVisible() && bar->actions().contains(exitFullScreenAction);
        };
        auto it = std::find_if(toolbars.constBegin(), toolbars.constEnd(), findExitFullScreenToolbar);
        if (it == toolbars.constEnd()) {
            QList<QAction *> lst{exitFullScreenAction};
            mw->plugActionList(QStringLiteral("fullscreen"), lst);
        }
    } else {
        mw->unplugActionList(QStringLiteral("fullscreen"));
    }
}

void FullScreenManager::toggleToggableViews(FullScreenState newState)
{
    if (newState == m_currentState) {
        return;
    }
    if (newState == FullScreenState::CompleteFullScreen) {
        m_visibleViewGUIClients.clear();
    }

    QList<QAction*> actions = mainWindow()->toggleViewActions();
    for (QAction *a : actions) {
        KToggleAction *ta = qobject_cast<KToggleAction*>(a);
        if (!ta) {
            continue;
        }
        if (newState == FullScreenState::CompleteFullScreen && ta->isChecked()) {
            m_visibleViewGUIClients.append(a->objectName());
            ta->setChecked(false);
        } else if (newState != FullScreenState::CompleteFullScreen) {
            ta->setChecked(m_visibleViewGUIClients.contains(ta->objectName()));
        }
    }
}

void FullScreenManager::toggleToolbars(FullScreenState newState)
{
    if (m_currentState == newState) {
        return;
    }
    if (newState == FullScreenState::CompleteFullScreen) {
        m_visibleToolBars.clear();
    }
    const QList<QAction*> actions = mainWindow()->toolBarMenuAction()->menu()->actions();
    for (QAction *a : actions) {
        if (newState == FullScreenState::CompleteFullScreen && a->isChecked()) {
            m_visibleToolBars.append(a->objectName());
            a->setChecked(false);
        } else if (m_currentState == FullScreenState::CompleteFullScreen) {
            a->setChecked(m_visibleToolBars.contains(a->objectName()));
        }
    }
}

void FullScreenManager::toggleStatusBar(FullScreenState newState)
{
    KonqMainWindow *mw = mainWindow();
    KonqView *view = mw->currentView();
    if (!view) {
        return;
    }

    KonqFrameStatusBar *statusBar = view->frame()->statusbar();
    if (!statusBar) {
        if (newState == FullScreenState::CompleteFullScreen) {
            m_wasStatusBarVisible = false;
        }
        return;
    }

    QAction *showStatusBarAction = action(QStringLiteral("options_show_statusbar"));
    Q_ASSERT(showStatusBarAction);
    if (newState == FullScreenState::CompleteFullScreen) {
        //Shouldn't calling a->setChecked automatically hide or show the statusbar? It seems it doesn't
        m_wasStatusBarVisible = showStatusBarAction->isChecked();
        showStatusBarAction->setChecked(false);
        statusBar->setVisible(false);
    } else {
        showStatusBarAction->setChecked(m_wasStatusBarVisible);
        statusBar->setVisible(m_wasStatusBarVisible);
    }
}

QDebug& operator<<(QDebug &dbg, FullScreenManager::FullScreenState state)
{
    switch (state) {
        case FullScreenManager::FullScreenState::NoFullScreen:
            return dbg << "NoFullScreen";
        case FullScreenManager::FullScreenState::OrdinaryFullScreen:
            return dbg << "OrdinaryFullScreen";
        case FullScreenManager::FullScreenState::CompleteFullScreen:
            return dbg << "CompleteFullScreen";
        default:
            return dbg << "Unknown state";
    }
}
