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
 * @todo write docs
 */
class KonqWindow : public KonqInterfaces::Window
{
    Q_OBJECT

public:
    /**
     * @todo write docs
     */
    KonqWindow(KonqMainWindow *window, KonqViewManager *viewManager);
    ~KonqWindow();

    /**
     * @todo write docs
     *
     * @return TODO
     */
    QWidget* widget() override;

    int tabsCount() const override;

    /**
     * @todo write docs
     *
     * @param idx TODO
     * @return TODO
     */
    QIcon tabFavicon(int idx) override;

    QString tabTitle(int idx) const override;

    QUrl tabUrl(int idx) const override;

    int activeTab() const override;

    KonqInterfaces::TabBarContextMenu* tabBarContextMenu(QWidget *parent = nullptr) const override;

    // KonqInterfaces::Ta displayTabbarContextMenu(const QPoint &pt, int tab) override {}

public Q_SLOTS:
    void activateTab(int idx) override;
    void moveTab(int fromIdx, int toIdx) override;

private Q_SLOTS:
    void tabsContainerChanged(KonqFrameTabs *container);
    void slotTabAdded(int idx);
    void slotTabRemoved(int idx);
    void connectToView(KonqView *view);
    void viewUrlChanged(KonqView *view, const QUrl &url);
    void viewCaptionChanged(KonqView *view, const QString &caption);

private:
    bool isViewActive(KonqView *view, int idx) const;

private:
    QPointer<KonqMainWindow> m_mainWindow;
    QPointer<KonqViewManager> m_viewManager;
    QPointer<KonqFrameTabs> m_tabsContainer;
};

}

#endif // KONQIMPLEMENTATIONS_KONQWINDOW_H
