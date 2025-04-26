//  This file is part of the KDE project
//  SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
// 
//  SPDX-License-Identifier: LGPL-2.0-or-later

#ifndef KONQIMPLEMENTATIONS_KONQWINDOW_H
#define KONQIMPLEMENTATIONS_KONQWINDOW_H

#include <interfaces/window.h>

#include <QPointer>

class KonqMainWindow;
class KonqViewManager;
class KonqFrameTabs;
class KonqView;

namespace KonqImplementations {

/**
 * @brief Implementation of KonqInterfaces::Window
 */
class KonqWindow : public KonqInterfaces::Window
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     *
     * @param window the window associated to the interface
     * @param viewManager the view manager associated with @p window
     */
    KonqWindow(KonqMainWindow *window, KonqViewManager *viewManager);
    ~KonqWindow(); //!< Destructor

    /**
     * @brief Implementation of KonqInterfaces::Window::widget()
     *
     * @return the main window object
     */
    QWidget* widget() override;

    int tabsCount() const override; //!< Implmentation of KonqInterfaces::Window::tabsCount()

    /**
     * @brief Override of KonqInterfaces::Window::tabFavicon()
     *
     * @return the favicon for the tab at position @p idx or an invalid icon if
     * there's not such tab
     */
    QIcon tabFavicon(int idx) override;

    /**
     * @brief Override of KonqInterfaces::Window::tabTitle()
     *
     * @return the title for the tab at position @p idx or an empty string if
     * there's not such tab
     */
    QString tabTitle(int idx) const override;

    /**
     * @brief Override of KonqInterfaces::Window::tabUrl()
     *
     * @return the URL for the tab at position @p idx or an empty URL if
     * there's not such tab
     */
    QUrl tabUrl(int idx) const override;

    /**
     * @brief Implementation of KonqInterfaces::Window::activeTab()
     */
    int activeTab() const override;

    KonqInterfaces::TabBarContextMenu* tabBarContextMenu(QWidget *parent = nullptr) const override;

public Q_SLOTS:

    /**
     * @brief Implementation of KonqInterfaces::Window::activateTab()
     */
    void activateTab(int idx) override;

    /**
     * @brief Implmentation of KonqInterfaces::Window::moveTab()
     *
     * It moves the given tab unless the tab container doesn't exist
     */
    void moveTab(int fromIdx, int toIdx) override;

private Q_SLOTS:

    /**
     * @brief Slot called when the tab container changes
     *
     * It makes the necessary signal-slot connections and disconnections with
     * the old and new tab container.
     *
     * @param container the new tab container
     */
    void tabsContainerChanged(KonqFrameTabs *container);

    /**
     * @brief Slot called when a new tab is added
     *
     * It emits the tabAdded() signal.
     *
     * @param idx the index of the new tab
     */
    void slotTabAdded(int idx);

    /**
     * @brief Slot called when a tab is removed
     *
     * It emits the tabRemoved() signal.
     *
     * @param idx the index of the removed tab
     */
    void slotTabRemoved(int idx);

    /**
     * @brief Performs the necessary signal-slot connections with the given view
     *
     * @param view the view to connect with
     */
    void connectToView(KonqView *view);

    /**
     * @brief Slot called when the URL of a view changes
     *
     * If the view is the active view in the tab, it emits the tabUrlChanged() signal.
     * @param view the view whose URL has changed
     * @param url the new URL of the view
     */
    void viewUrlChanged(KonqView *view, const QUrl &url);

    /**
     * @brief Slot called when the caption of a view changes
     *
     * If the view is the active view in the tab, it emits the tabTitleChanged() signal.
     * @param view the view whose URL has changed
     * @param caption the new caption of the view
     */
    void viewCaptionChanged(KonqView *view, const QString &caption);

private:
    /**
     * @brief Whether the given view is the active one in the given tab
     *
     * @param view the view to test
     * @param idx the index of the tab
     *
     * @return `true` if the view is the active view in the tab and `false` otherwise
     */
    bool isViewActive(KonqView *view, int idx) const;

private:
    QPointer<KonqMainWindow> m_mainWindow; //!< The main window
    QPointer<KonqViewManager> m_viewManager; //!< The view manager
    QPointer<KonqFrameTabs> m_tabsContainer; //!< The tab container
};

}

#endif // KONQIMPLEMENTATIONS_KONQWINDOW_H
