/* This file is part of the KDE libraries
    Copyright (C) 2003 Stephan Binner <binner@kde.org>
    Copyright (C) 2003 Zack Rusin <zack@kde.org>
    Copyright (C) 2009 Urs Wolfer <uwolfer @ kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KTABBAR_H
#define KTABBAR_H

#include <QTabBar>

/**
 * A QTabBar with extended features.
 *
 */
class KTabBar : public QTabBar
{
    Q_OBJECT

public:
    /**
     * Creates a new tab bar.
     *
     * @param parent The parent widget.
     */
    explicit KTabBar(QWidget *parent = nullptr);

    /**
     * Destroys the tab bar.
     */
    ~KTabBar() override;

Q_SIGNALS:
    /**
     * A right mouse button click was performed over the tab with the @param index.
     * The signal is emitted on the press of the mouse button.
     */
    void contextMenu(int index, const QPoint &globalPos);
    /**
     * A right mouse button click was performed over the empty area on the tab bar.
     * The signal is emitted on the press of the mouse button.
     */
    void emptyAreaContextMenu(const QPoint &globalPos);
    /**
     * A double left mouse button click was performed over the tab with the @param index.
     * The signal is emitted on the second press of the mouse button, before the release.
     */
    void tabDoubleClicked(int index);
    /**
     * A double left mouse button click was performed over the empty area on the tab bar.
     * The signal is emitted on the second press of the mouse button, before the release.
     */
    void newTabRequest();
    /**
     * A double middle mouse button click was performed over the tab with the @param index.
     * The signal is emitted on the release of the mouse button.
     */
    void mouseMiddleClick(int index);
    void initiateDrag(int);
    void testCanDecode(const QDragMoveEvent *, bool &);
    void receivedDropEvent(int, QDropEvent *);
    /**
     * Used internally by KTabBar's/KTabWidget's middle-click tab moving mechanism.
     * Tells the KTabWidget which owns the KTabBar to move a tab.
     */
    void moveTab(int, int);
#ifndef QT_NO_WHEELEVENT
    void wheelDelta(int);
#endif

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *event) override;
#endif

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    void tabLayoutChange() override;

private Q_SLOTS:
    void activateDragSwitchTab();

private:
    int selectTab(const QPoint &pos) const;

private:
    class Private;
    Private *const d;
};

#endif
