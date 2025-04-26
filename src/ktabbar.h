/* This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2003 Stephan Binner <binner@kde.org>
    SPDX-FileCopyrightText: 2003 Zack Rusin <zack@kde.org>
    SPDX-FileCopyrightText: 2009 Urs Wolfer <uwolfer @ kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KTABBAR_H
#define KTABBAR_H

#include <QTabBar>

/**
 * @brief A QTabBar with extended features.
 *
 * It provides support for drag and drop on a tab, improves support for switching
 * tabs using the mouse wheel and provides additional signals.
 *
 */
class KTabBar : public QTabBar
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     *
     * @param parent The parent widget.
     */
    explicit KTabBar(QWidget *parent = nullptr);

    ~KTabBar() override; //!< Destructor

Q_SIGNALS:
    /**
     * @brief Signal emitted when the right mouse button is clicked on a tab
     *
     * The signal is emitted on button press (not release).
     *
     * @param index the index of the tab
     * @param globalPos the position at which the mouse was clicked, in global coordinates
     */
    void contextMenu(int index, const QPoint &globalPos);

    /**
     * @brief Signal emitted when the right mouse button is clicked over the empty area of the tab bar
     *
     * The signal is emitted on button press (not release).
     *
     * @param globalPos the position at which the mouse was clicked, in global coordinates
     */
    void emptyAreaContextMenu(const QPoint &globalPos);

    /**
     * @brief Signal emitted when the left mouse button is double clicked on a tab
     *
     * The signal is emitted on the second button press (not release).
     *
     * @param index the index of the tab
     */
    void tabDoubleClicked(int index);

    /**
     * @brief Signal emitted when the left mouse button is double clicked over the empty area of the tab bar
     *
     * The signal is emitted on the second button press (not release).
     */
    void newTabRequest();

    /**
     * @brief Signal emitted when the middle mouse button is double clicked on a tab
     *
     * The signal is emitted on the second button press (not release).
     *
     * @param index the index of the tab
     */
    void mouseMiddleClick(int index);

    /**
     * @brief Signal emitted when a drag operation starts
     *
     * @param index the index of the tab where the drag operation started
     */
    void initiateDrag(int index);

    /**
     * @brief Signal emitted when deciding to accept a drag enter or move event
     *
     * @param event the event to accept or reject
     * @param [out] accept whether the event should be accepted or not. Receivers
     * of this signals should set this to `true` if they want the event to be accepted
     */
    void testCanDecode(const QDragMoveEvent *event, bool &accept);

    /**
     * @brief Signal emitted when a drop event happens on a tab
     *
     * @param index the index of the tab where the event happened
     * @param event the event
     */
    void receivedDropEvent(int index, QDropEvent *event);

    /**
     * @brief Signal emitted when moving a tab
     *
     * Unused
     */
    void moveTab(int, int);
#ifndef QT_NO_WHEELEVENT

    /**
     * @brief Signal emitted when the mouse wheel is turned
     *
     * @param delta the vertical rotation angle of the wheel, in eighths of a degree
     */
    void wheelDelta(int delta);
#endif

protected:
    /**
     * @brief Override of `QTabBar::mouseDoubleClickEvent()`
     *
     * It emits the newTabRequest() or tabDoubleClicked() if needed
     *
     * @param event the double click event
     */
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    /**
     * @brief Override of `QTabBar::mousePressEvent()`
     *
     * It emits the contextMenu() or emptyAreaContextMenu() if needed.
     *
     * @param event the mouse press event
     */
    void mousePressEvent(QMouseEvent *event) override;

    /**
     * @brief Override of `QTabBar::mouseMoveEvent()`
     *
     * It starts a drag operation if needed.
     *
     * @param event the mouse move event
     */
    void mouseMoveEvent(QMouseEvent *event) override;

#ifndef QT_NO_WHEELEVENT

    /**
     * @brief Override of `QTabBar::mouseWheelEvent()`
     *
     * When the user rotates the mouse wheel, `QTabBar` switches to the next
     * or previous tab, depending on whether the wheel is rotated forwards or backwards,
     * but does nothing if the wheel is rotated forward while at the last tab or
     * backwards while at the first tab. This override improves that behaviour by
     * "wrapping around": a forward rotation while on the last tab moves to the first
     * tab, while a backward rotation while on the first tab moves to the last tab.
     *
     * It also emits the wheelDelta() signal, but only if there are slots connected
     * to it.
     *
     * @param event the mouse wheel event
     */
    void wheelEvent(QWheelEvent *event) override;
#endif

    /**
     * @brief Override of `QTabBar::dragEnterEvent()`
     *
     * It emits the testCanDecode() signal and starts a timer if the signal is
     * accepted.
     *
     * @param event the drag enter event
     */
    void dragEnterEvent(QDragEnterEvent *event) override;

    /**
     * @brief Override of `QTabBar::dragMoveEvent()`
     *
     * It emits the testCanDecode() signal and starts a timer if the signal is
     * accepted.
     *
     * @param event the drag move event
     */
    void dragMoveEvent(QDragMoveEvent *event) override;

    /**
     * @brief Override of `QTabBar::dropEvent()`
     *
     * It emits the receivedDropEvent() signal.
     *
     * @param event the drop event
     */
    void dropEvent(QDropEvent *event) override;

    /**
     * @brief Override of `QTabBar::tabLayoutChange()`
     *
     * It resets drag and drop information.
     */
    void tabLayoutChange() override;

private Q_SLOTS:
    /**
     * @brief Slot called when the drag and drop timer times out
     *
     * It activates the tab where the cursor is if it's the target of the drop
     * operation.
     */
    void activateDragSwitchTab();

private:

    /**
     * @brief The tab containing a given point
     *
     * @param pos the point
     * @return the index of the tab containing @p pt or -1 if the point is outside all tabs
     */
    int selectTab(const QPoint &pos) const;

    /**
     * @overload KTabBar::selectTab(const QPointF &pos) const
     *
     * This is an overload of KTabBar::selectTab(const QPoint &)const.
     *
     * @param pos the point
     * @return the index of the tab containing @p pt or -1 if the point is outside all tabs
     */
    int selectTab(const QPointF &pos) const;

private:
    class Private;
    Private *const d; //!< The d-pointer
};

#endif
