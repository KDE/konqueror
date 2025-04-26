/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2002 John Firebaugh <jfirebaugh@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef KONQDRAGGABLELABEL_H
#define KONQDRAGGABLELABEL_H

#include <QUrl>
#include <QLabel>
class KonqMainWindow;

/**
 * @brief A `QLabel` which can be used to drag and drop URLs
 *
 * When starting a drag operation from it, the URL in the current view is dragged;
 * when ending a drop operation on it, the dropped URL is opened in the current view
 */
class KonqDraggableLabel : public QLabel
{
    Q_OBJECT
public:

    /**
     * @brief Constructor
     *
     * @param mw the main window where the label is
     * @param text the text of the label
     */
    KonqDraggableLabel(KonqMainWindow *mw, const QString &text);

protected:

    /**
     * @brief Override of `QLabel::mousePressEvent()`
     *
     * It records the position of the event as possible start of a drag operation and turns
     * the #validDrag flag on
     *
     * @param ev the event
     */
    void mousePressEvent(QMouseEvent *ev) override;

    /**
     * @brief Override of `QLabel::mouseMoveEvent()`
     *
     * It starts a drag operation if needed, using the URL of the current view as the dragged data.
     * If there's no current view, the drag operation is not started
     *
     * @param ev the event
     */
    void mouseMoveEvent(QMouseEvent *ev) override;

    /**
     * @brief Override of `QLabel::mouseReleaseEvent()`
     *
     * It turns the #validDrag flag off
     * @param ev the event
     */
    void mouseReleaseEvent(QMouseEvent *ev) override;

    /**
     * @brief Override of `QLabel::dragEnterEvent()`
     *
     * It accepts the event if @p ev contains one or more URLs
     *
     * @param ev the event
     */
    void dragEnterEvent(QDragEnterEvent *ev) override;

    /**
     * @brief Override of `QLabel::dropEvent()`
     *
     * It opens the first URLs in @p ev by calling delayedOpenURL() asynchronously
     *
     * @param ev the event
     */
    void dropEvent(QDropEvent *ev) override;

private Q_SLOTS:
    /**
     * @brief Opens the first URL in #_savedLst in the main window
     */
    void delayedOpenURL();

private:
    QPoint startDragPos; //!< The position of the last mouse click

    /**
     * @brief Whether a mouse move should start a drag
     *
     * This is set to `true` when a mouse button is clicked on the label and reset to
     * `false` when a drag operation starts or a mouse button is released
     */
    bool validDrag;

    KonqMainWindow *m_mw; //!< The main window where the label is
    /**
     * @brief A list of the URLs contained in the last dropped data
     *
     * The first element in the list is opened in the current view when a drop event occurs
    */
    QList<QUrl> _savedLst;
};
#endif
