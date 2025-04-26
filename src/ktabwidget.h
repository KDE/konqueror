/* This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2003 Stephan Binner <binner@kde.org>
    SPDX-FileCopyrightText: 2003 Zack Rusin <zack@kde.org>
    SPDX-FileCopyrightText: 2009 Urs Wolfer <uwolfer @ kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KTABWIDGET_H
#define KTABWIDGET_H

#include <QTabWidget>
#include <konqprivate_export.h>

class QTab;

/**
 * @brief A widget containing multiple tabs
 *
 * It extends the Qt `QTabWidget`, providing extra features such as automatic
 * resizing of tabs, and moving tabs.
 *
 * When resizing tabs, tab texts may be squeezed to fit the available space. To
 * allow this, the unsqueezed text of the tabs is stored separately.
 */
class KONQ_TESTS_EXPORT KTabWidget : public QTabWidget
{
    Q_OBJECT
    Q_PROPERTY(bool automaticResizeTabs READ automaticResizeTabs WRITE setAutomaticResizeTabs)

public:

    /**
     * @brief Constructor
     *
     * @param parent the parent widget.
     * @param flags the Qt window flags @see QWidget.
     */
    explicit KTabWidget(QWidget *parent = nullptr, Qt::WindowFlags flags = {});

    ~KTabWidget() override; //!< Destructor

    /**
     * @brief Whether changing the title of a tab will resize tabs
     *
     * @return `true` if calling setTitle() will resize tabs to the width of the tab bar
     * and `false` if calling setTitle() won't change the size of the tabs.
     */
    bool automaticResizeTabs() const;

    /**
     * @brief The text for the given tab
     *
     * This is reimplemented from `QTabWidget` to return the correct value in case
     * autoresizing is enabled.
     *
     * @warning This is not a virtual function, so you need to explicitly call the
     * reimplemented function.
     *
     * @param index the index of the tab to retrieve the text for
     * @return the text for the tab at position @p index or an empty string if @p index
     * doesn't correspond to a tab
     */
    QString tabText(int index) const;   // but it's not virtual...

    /**
     * @brief Sets the text for the given tab
     *
     * This is reimplemented from `QTabWidget` to correctly update the tab size
     * if autoresizing is enabled.
     *
     * @warning This is not a virtual function, so you need to explicitly call the
     * reimplemented function.
     *
     * @param index the index of the tab to whose text should be changed
     * @param text the new text
     */
    void setTabText(int index, const QString &text);

public Q_SLOTS:
    /**
     * @brief Removes the tab at the given position
     *
     * This is reimplemented from `QTabWidget` to ensure that tab names are kept
     * in sync with the tabs themselves.
     *
     * @warning This is not a virtual function, so you need to explicitly call the
     * reimplemented function.
     *
     * @param index the index of the tab to remove. If no tab corresponds to this
     * index, nothing is done
     */
    virtual void removeTab(int index); // but it's not virtual in QTabWidget...

    /**
     * @brief Toggles automatic tab resizing to the width of the tab bar
     *
     * @param enable if `true`, tabs will be resized to the width of the tab bar;
     * if `false`, they won't be resized.
     *
     * @warning It does not work reliably with `QTabWidget* foo=new KTabWidget()` and if
     * you change tabs via the tab bar or by accessing tabs directly.
     */
    void setAutomaticResizeTabs(bool enable);

Q_SIGNALS:
    /**
     * @brief Signal which objects can connect to if they want and can decode a drag move event
     *
     * @param e the event
     * @param [out] accept a variable which the receiver object must set to `true`
     * if they are able to decode the event
     */
    void testCanDecode(const QDragMoveEvent *e, bool &accept /* result */);

    /**
     * @brief Signal emitted when a drop event happens in the empty space beside the
     * tab bar
     *
     * Usually this causes a new tab to be created.
     *
     * This signal is only emitted after testCanDecode() is emitted and if its
     * `accept` parameter is set to `true`.
     *
     * @param e the event
     */
    void receivedDropEvent(QDropEvent *e);

    /**
     * @brief Signal emitted when a drop event happens on the tab associated with a
     * given widget
     *
     * This signal is only emitted after testCanDecode() is emitted and if its
     * `accept` parameter is set to `true`.
     *
     * @param widget the widget on whose tab the drop event happened
     * @param e the event
     */
    void receivedDropEvent(QWidget *widget, QDropEvent *e);

    /**
     * @brief Signal to request starting a drag operation on the given tab
     *
     * @param widget the widget corresponding to the tab
     */
    void initiateDrag(QWidget *widget);

    /**
     * @brief signal emitted when the right mouse button is pressed on an empty space
     * beside the tab bar
     *
     * @param pt the point where the button was pressed
     */
    void contextMenu(const QPoint &pt);

    /**
     * @brief Signal emitted when the right mouse button is pressed over a tab
     *
     * @param widget the widget associated with the tab
     * @param pt the point where the button was pressed
     */
    void contextMenu(QWidget *widget, const QPoint &pt);

    /**
     * @brief Signal emitted when a double left mouse button click happens on an empty
     * space besides the tab bar
     *
     * The signal is emitted on the second press of the mouse button, before the release.
     */
    void mouseDoubleClick();

    /**
     * @brief Signal emitted when a double left mouse button click is performed over a widget
     *
     * The signal is emitted on the second press of the mouse button, before the release.
     *
     * @param widget the widget the double click was performed on
     */
    void mouseDoubleClick(QWidget *widget);

    /**
     * @brief Signal emitted when a middle mouse button click is performed over empty space besides tabbar
     *
     * The signal is emitted on the release of the mouse button.
     */
    void mouseMiddleClick();

    /**
     * @brief Signal emitted when a middle mouse button click is performed over a widget
     *
     * The signal is emitted on the release of the mouse button.
     *
     * @param widget the widget the mouse click was performed on
     */
    void mouseMiddleClick(QWidget *widget);

protected:
    /**
     * @brief Override of `QTabBar::mouseDoubleClickEvent()`
     *
     * It emits the mouseDoubleClick() signal if the double click happened on the
     * space besides the tab bar and the clicked button is the left one.
     *
     * @param e the event
     */
    void mouseDoubleClickEvent(QMouseEvent *e) override;

    /**
     * @brief Override of `QTabBar::mousePressEvent()`
     *
     * It emits the contextMenu() signal if the right mouse button was pressed
     * outside the tabs.
     *
     * @param e the event
     */
    void mousePressEvent(QMouseEvent *e) override;

    /**
     * @brief Override of `QTabBar::mouseReleaseEvent()`
     *
     * It emits the mouseMiddleClick() signal if the middle mouse button was pressed
     * outside the tabs.
     *
     * @param e the event
     */
    void mouseReleaseEvent(QMouseEvent *e) override;

    /**
     * @brief Override of `QTabBar::dragEnterEvent()`
     *
     * It emits the testCanDecode() signal if the event started outside the tabs
     * and accepts the event if the `accepted` parameter of the signal is set to
     * `true`.
     *
     * @param e the event
     */
    void dragEnterEvent(QDragEnterEvent *e) override;

    /**
     * @brief Override of `QTabBar::dragMoveEvent()`
     *
     * It emits the testCanDecode() signal if the event started outside the tabs
     * and accepts the event if the `accepted` parameter of the signal is set to
     * `true`.
     *
     * @param e the event
     */
    void dragMoveEvent(QDragMoveEvent *e) override;

    /**
     * @brief Override of `QTabBar::dropEvent()`
     *
     * It emits the receivedDropEvent() if the event happened outside the tabs.
     *
     * @param e the event
     */
    void dropEvent(QDropEvent *e) override;

    /**
     * @brief The width of the tab bar so that it can show the given number of
     * characters
     *
     * @param chars
     * @return the width the tab bar must have to show @p chars characters
     */
    int tabBarWidthForMaxChars(int chars);
#ifndef QT_NO_WHEELEVENT

    /**
     * @brief Override of `QTabBar::wheelEvent()`
     *
     * If the event happened outside the tabs, it sends the event to the tab bar,
     * otherwise it behaves as the base class method.
     *
     * @param e the event
     */
    void wheelEvent(QWheelEvent *e) override;
#endif

    /**
     * @brief Override of `QTabBar::resizeEvent()`
     *
     * It automatically resize the tabs, if automaticResizeTabs() is `true`.
     *
     * @param e the event
     */
    void resizeEvent(QResizeEvent *e) override;

    /**
     * @brief It updates the list of unsqueezed tabs labels
     *
     * @param idx the index of the inserted tab
     */
    void tabInserted(int idx) override;

protected Q_SLOTS:

    /**
     * @brief Slot connected to the signal with the same name of the tab bar
     *
     * It emits the receivedDropEvent(QWidget*, QDropEvent*) signal
     * @param index the index of the tab
     * @param event the event
     */
    virtual void receivedDropEvent(int index, QDropEvent *event);

    /**
     * @brief Slot connected to the signal with the same name of the tab bar
     *
     * It emits the initiateDrag(QWidget*) signal
     * @param index the index of the tab
     */
    virtual void initiateDrag(int index);

    /**
     * @brief Slot connected to the signal with the same name of the tab bar
     *
     * It emits the contextMenu(QWidget*, QPoint&) signal
     * @param index the index of the tab
     * @param point the point where the mouse was clicked
     */
    virtual void contextMenu(int index, const QPoint &point);

    /**
     * @brief Slot connected to the tab bar tabDoubleClicked() signal
     *
     * It emits the mouseDoubleClick(QWidget*) signal
     * @param index the index of the tab where the mouse was double-clicked
     */
    virtual void mouseDoubleClick(int);

    /**
     * @brief Slot connected to the signal with the same name of the tab bar
     *
     * It emits the mouseMiddleClickClick(QWidget*) signal
     * @param index the index of the tab where the mouse was clicked
     */
    virtual void mouseMiddleClick(int index);
#ifndef QT_NO_WHEELEVENT

    /**
     * @brief Changes the current tab when the mouse wheel is turned on the tab bar
     *
     * If @p delta is negative, the current tab is moved forwards, while if it's
     * positive, it's moved backwards.
     *
     * @param delta the angle the mouse wheel was turned by
     */
    virtual void wheelDelta(int delta);
#endif

private:
    class Private;
    Private *const d; //!< The d-pointer

    Q_PRIVATE_SLOT(d, void slotTabMoved(int, int))
};

#endif
