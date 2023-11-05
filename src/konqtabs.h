/*  This file is part of the KDE project

    SPDX-FileCopyrightText: 2002-2003 Konqueror Developers <konq-e@kde.org>
    SPDX-FileCopyrightText: 2002-2003 Douglas Hanley <douglash@caltech.edu>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQTABS_H
#define KONQTABS_H

#include "konqframe.h"
#include "konqframecontainer.h"

#include "ktabwidget.h"
#include <QKeyEvent>
#include <QList>

class QMenu;

class KonqView;
class KonqViewManager;
class KonqFrameContainerBase;
class KonqFrameContainer;
class KConfig;
class QToolButton;

class NewTabToolButton;

class KONQ_TESTS_EXPORT KonqFrameTabs : public KTabWidget, public KonqFrameContainerBase
{
    Q_OBJECT

public:
    KonqFrameTabs(QWidget *parent, KonqFrameContainerBase *parentContainer,
                  KonqViewManager *viewManager);
    ~KonqFrameTabs() override;

    bool accept(KonqFrameVisitor *visitor) override;

    void saveConfig(KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options,
                            KonqFrameBase *docContainer, int id = 0, int depth = 0) override;
    void copyHistory(KonqFrameBase *other) override;

    const QList<KonqFrameBase *> &childFrameList() const
    {
        return m_childFrameList;
    }

    void setTitle(const QString &title, QWidget *sender) override;
    void setTabIcon(const QUrl &url, QWidget *sender) override;

    QWidget *asQWidget() override
    {
        return this;
    }
    KonqFrameBase::FrameType frameType() const override
    {
        return KonqFrameBase::Tabs;
    }

    void activateChild() override;

    /**
     * Insert a new frame into the container.
     */
    void insertChildFrame(KonqFrameBase *frame, int index = -1) override;

    /**
     * Call this before deleting one of our children.
     */
    void childFrameRemoved(KonqFrameBase *frame) override;

    void replaceChildFrame(KonqFrameBase *oldFrame, KonqFrameBase *newFrame) override;

    /**
     * Returns the tab at a given index
     * (same as widget(index), but casted to a KonqFrameBase)
     */
    KonqFrameBase *tabAt(int index) const;

    /**
     * Returns the current tab
     * (same as currentWidget(), but casted to a KonqFrameBase)
     */
    KonqFrameBase *currentTab() const;

    void moveTabBackward(int index);
    void moveTabForward(int index);

    void setLoading(KonqFrameBase *frame, bool loading);

    /**
     * Returns the tab index that contains (directly or indirectly) the frame @p frame,
     * or -1 if the frame is not in the tab widget.
     */
    int tabIndexContaining(KonqFrameBase *frame) const;

    /**
     * Implemented to catch MMB click when KonqSettings::mouseMiddleClickClosesTab()
     * returns true so that the tab can be properly closed without being activated
     * first.
     */
    bool eventFilter(QObject *, QEvent *) override;

public Q_SLOTS:
    void slotCurrentChanged(int index);
    void setAlwaysTabbedMode(bool);
    void forceHideTabBar(bool force);
    void reparseConfiguration();

Q_SIGNALS:
    void removeTabPopup();
    void openUrl(KonqView *view, const QUrl &url);

private:
    void refreshSubPopupMenuTab();
    void updateTabBarVisibility();
    void initPopupMenu();
    /**
     * Returns the index position of the tab where the frame @p frame is, assuming that
     * it's the active frame in that tab,
     * or -1 if the frame is not in the tab widget or it's not active.
     */
    int tabWhereActive(KonqFrameBase *frame) const;

    /**
     * @brief Applies the user choice regarding the tab bar position
     *
     * If the option contains an invalid value, `North` will be used
     */
    void applyTabBarPositionOption();

private Q_SLOTS:
    void slotContextMenu(const QPoint &);
    void slotContextMenu(QWidget *, const QPoint &);
    void slotCloseRequest(int);
    void slotMovedTab(int, int);
    void slotMouseMiddleClick();
    void slotMouseMiddleClick(QWidget *);

    void slotTestCanDecode(const QDragMoveEvent *e, bool &accept /* result */);
    void slotReceivedDropEvent(QDropEvent *);
    void slotInitiateDrag(QWidget *);
    void slotReceivedDropEvent(QWidget *, QDropEvent *);
    void slotSubPopupMenuTabActivated(QAction *);

private:
    QList<KonqFrameBase *> m_childFrameList;

    KonqViewManager *m_pViewManager;
    QMenu *m_pPopupMenu;
    QMenu *m_pSubPopupMenuTab;
    QToolButton *m_rightWidget;
    NewTabToolButton *m_leftWidget;
    bool m_permanentCloseButtons;
    bool m_alwaysTabBar;
    bool m_forceHideTabBar;
    QMap<QString, QAction *> m_popupActions;
};

#include <QToolButton>

class NewTabToolButton : public QToolButton // subclass with drag'n'drop functionality for links
{
    Q_OBJECT
public:
    NewTabToolButton(QWidget *parent)
        : QToolButton(parent)
    {
        setAcceptDrops(true);
    }

Q_SIGNALS:
    void testCanDecode(const QDragMoveEvent *event, bool &accept);
    void receivedDropEvent(QDropEvent *event);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override
    {
        bool accept = false;
        emit testCanDecode(event, accept);
        if (accept) {
            event->acceptProposedAction();
        }
    }

    void dropEvent(QDropEvent *event) override
    {
        emit receivedDropEvent(event);
        event->acceptProposedAction();
    }
};

#endif // KONQTABS_H
