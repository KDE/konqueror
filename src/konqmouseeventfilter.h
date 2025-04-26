/*
    SPDX-FileCopyrightText: 2009 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2016 Anthony Fieroni <bvbfan@abv.bg>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef KONQMOUSEEVENTFILTER_H
#define KONQMOUSEEVENTFILTER_H

#include <QObject>

/**
 * @brief Singleton class providing a global event filter for handling mouse events
 *
 * This event filter provides functionality for the *back* and *forward* mouse buttons
 * and for the option to use the right mouse button as a *back* button.
 *
 * Since it must act globally, this event filter is automatically installed on the
 * application object.
 */
class KonqMouseEventFilter : public QObject
{
    Q_OBJECT

public:

    /**
     * @brief The only instance of this class
     *
     * @return the only instance of this class
     */
    static KonqMouseEventFilter *self();

    /**
     * @brief Applies the user settings from the configuration file
     *
     * It checks whether the user chose to use the right mouse button as a *back*
     * button.
     */
    void reparseConfiguration();

protected:
    /**
     * @brief The event filter function
     *
     * It only filters mouse events and context menu events:
     * - it calls KonqMainWindow::slotBack() or KonqMainWindow::slotForward()
     * when the back or forward buttons are pressed
     * - if the user chose to use the right mouse button as a *back* button:
     *  - it ignores right click press events
     *  - it calls KonqMainWindow::slotBack() on right button release
     *  - on move events with the right mouse button pressed, it simulates a
     *    right mouse button press followed by the move event,
     *    bypassing the filter itself
     *  - it ignores context menu events caused by right clicks
     * @return `true` in case of backward and forward button presses and, in case
     * the user chose to use the right mouse button as a *back* button, also for
     * right mouse presses and releases and for context menu events caused by
     * right mouse button presses. It returns `false` in all other cases.
     */
    bool eventFilter(QObject *obj, QEvent *e) override;

private:
    /**
     * @brief Constructor
     *
     * After creating the object, it automatically install the object itself on
     * the application.
     */
    explicit KonqMouseEventFilter();
    friend class KonqMouseEventFilterSingleton;

    bool m_bBackRightClick; //!< Whether the user chose to use the right mouse button as a *back* button
};

#endif /* KONQMOUSEEVENTFILTER_H */

