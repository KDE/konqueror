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
 * \brief A widget containing multiple tabs
 *
 * It extends the Qt QTabWidget, providing extra features such as automatic resizing of tabs, and moving tabs.
 *
 * See also the QTabWidget documentation.
 */
class KONQ_TESTS_EXPORT KTabWidget : public QTabWidget
{
    Q_OBJECT
    Q_PROPERTY(bool automaticResizeTabs READ automaticResizeTabs WRITE setAutomaticResizeTabs)

public:

    /**
     * Creates a new tab widget.
     *
     * @param parent The parent widgets.
     * @param flags The Qt window flags @see QWidget.
     */
    explicit KTabWidget(QWidget *parent = nullptr, Qt::WindowFlags flags = {});

    /**
     * Destroys the tab widget.
     */
    ~KTabWidget() override;

    /**
     * Returns true if calling setTitle() will resize tabs
     * to the width of the tab bar.
     */
    bool automaticResizeTabs() const;

    /**
     * Reimplemented for internal reasons.
     */
    QString tabText(int) const;   // but it's not virtual...

    /**
     * Reimplemented for internal reasons.
     */
    void setTabText(int, const QString &);

public Q_SLOTS:
    /**
     * Removes the widget, reimplemented for
     * internal reasons (keeping labels in sync).
     */
    virtual void removeTab(int index); // but it's not virtual in QTabWidget...

    /**
     * If \a enable is true, tabs will be resized to the width of the tab bar.
     *
     * Does not work reliably with "QTabWidget* foo=new KTabWidget()" and if
     * you change tabs via the tabbar or by accessing tabs directly.
     */
    void setAutomaticResizeTabs(bool enable);

Q_SIGNALS:
    /**
     * Connect to this and set accept to true if you can and want to decode the event.
     */
    void testCanDecode(const QDragMoveEvent *e, bool &accept /* result */);

    /**
     * Received an event in the empty space beside tabbar. Usually creates a new tab.
     * This signal is only possible after testCanDecode and positive accept result.
     */
    void receivedDropEvent(QDropEvent *);

    /**
     * Received an drop event on given widget's tab.
     * This signal is only possible after testCanDecode and positive accept result.
     */
    void receivedDropEvent(QWidget *, QDropEvent *);

    /**
     * Request to start a drag operation on the given tab.
     */
    void initiateDrag(QWidget *);

    /**
     * The right mouse button was pressed over empty space besides tabbar.
     */
    void contextMenu(const QPoint &);

    /**
     * The right mouse button was pressed over a widget.
     */
    void contextMenu(QWidget *, const QPoint &);

    /**
     * A double left mouse button click was performed over empty space besides tabbar.
     * The signal is emitted on the second press of the mouse button, before the release.
     */
    void mouseDoubleClick();

    /**
     * A double left mouse button click was performed over the widget.
     * The signal is emitted on the second press of the mouse button, before the release.
     */
    void mouseDoubleClick(QWidget *);

    /**
     * A middle mouse button click was performed over empty space besides tabbar.
     * The signal is emitted on the release of the mouse button.
     */
    void mouseMiddleClick();

    /**
     * A middle mouse button click was performed over the widget.
     * The signal is emitted on the release of the mouse button.
     */
    void mouseMiddleClick(QWidget *);

protected:
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void dragEnterEvent(QDragEnterEvent *) override;
    void dragMoveEvent(QDragMoveEvent *) override;
    void dropEvent(QDropEvent *) override;
    int tabBarWidthForMaxChars(int);
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *) override;
#endif
    void resizeEvent(QResizeEvent *) override;
    void tabInserted(int) override;

protected Q_SLOTS:
    virtual void receivedDropEvent(int, QDropEvent *);
    virtual void initiateDrag(int);
    virtual void contextMenu(int, const QPoint &);
    virtual void mouseDoubleClick(int);
    virtual void mouseMiddleClick(int);
#ifndef QT_NO_WHEELEVENT
    virtual void wheelDelta(int);
#endif

private:
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT(d, void slotTabMoved(int, int))
};

#endif
