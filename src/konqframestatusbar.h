/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Michael Reiher <michael.reiher@gmx.de>
    SPDX-FileCopyrightText: 2007, 2010 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQ_FRAMESTATUSBAR_H
#define KONQ_FRAMESTATUSBAR_H

#include <QStatusBar>
#include "konqstatusbarmessagelabel.h"
class QLabel;
class QProgressBar;
class QCheckBox;
class KonqView;
class KonqFrame;
namespace KParts
{
class ReadOnlyPart;
}

/**
 * @brief The status bar under each view
 *
 * It provides:
 * - a led to display whether the view is active or not
 * - a checkbox to toggle the linked status of the view
 * - an area to display the actual messages.
 *
 * @warning You shouldn't use `QStatusBar` API when using this class
 *
 * @note Konqueror doesn't have an main window-wide statusbar, but a status bar for
 * each view
 */
class KonqFrameStatusBar : public QStatusBar
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param _parent the parent frame. This is the frame where the view associated to
     * the statusbar is
     */
    explicit KonqFrameStatusBar(KonqFrame *_parent = nullptr);
    ~KonqFrameStatusBar() override; //!< Destructor

    /**
     * @brief Changes the message shown in the statusbar
     *
     * @param msg the new message
     * @param type the type of message
     */
    void setMessage(const QString &msg, KonqStatusBarMessageLabel::Type type);

    /**
     * @brief Toggles the linked-view checkbox
     *
     * @param b whether the check box should be checked or unchecked
     */
    void setLinkedView(bool b);

    /**
     * @brief Changes the visibility of the _active view_ indicator
     *
     * @param b `true` if the indicator should be visibile and false if it should be hidden
     */
    void showActiveViewIndicator(bool b);

    /**
     * @brief Change the visibility of the _linked_ indicator
     *
     * @param b `true` if the _linked_ indicator should be visible and `false` if it should be hidden
     */
    void showLinkedViewIndicator(bool b);

    /**
     * @brief Updates the _active view_ indicator and the statusbar color
     *
     * The status of the indicator and the color of the statusbar depend on whether
     * the associated frame contains the active part or not
     */
    void updateActiveStatus();

public Q_SLOTS:

    /**
     * @brief Associates the statusbar with a new part
     *
     * This makes the necessary signal-slot connections and clears the statusbar text
     *
     * @param oldOne the original part associated with the statusbar
     * @param newOne the part to associate with the statusbar
     */
    void slotConnectToNewView(KonqView *, KParts::ReadOnlyPart *oldOne, KParts::ReadOnlyPart *newOne);

    /**
     * @brief Updates the loading progress bar
     *
     * When the percentage is 100 or it's -1, the bar is hidden.
     *
     * @param percent the loading percentage. A value of 100 means that loading is
     * completed. A value of -1 means that an error occurred
     */
    void slotLoadingProgress(int percent);

    /**
     * @brief Shows a message with the loading speed
     *
     * @param bytesPerSecond the download speed in bytes per second
     */
    void slotSpeedProgress(int bytesPerSecond);

    /**
     * @brief Displays a permanent message
     *
     * Besides showing @p text, it also sets it as saved message in #m_savedMessage
     *
     * @param text the message
     */
    void slotDisplayStatusText(const QString &text);

    /**
     * @brief Removes the currently visible message and replaces it with the previous one
     */
    void slotClear();

    /**
     * @brief Shows a permanent message in the statusbar
     *
     * It saves the previously shown message in #m_savedMessage
     *
     * @p message the message to show
     */
    void message(const QString &message);

Q_SIGNALS:
    /**
     * @brief Signal emitted when the user clicked the bar
     */
    void clicked();

    /**
     * @brief Signal emitted when the user toggles the _linked view_ check box
     *
     * @param mode whether the check box was turned on or off
     */
    void linkedViewClicked(bool mode);

protected:
    /**
     * @brief Override of `QObject::eventFilter()`
     *
     * This is meant to be installed on the label (#m_pStatusLabel).
     *
     * It emits the clicked() signal when on a mouse click and
     * also shows a context menu if it was a right-button click. It also updates the
     * statusbar when the palette changes.
     *
     * @param obj the object which received the event
     * @param ev the event
     */
    bool eventFilter(QObject *obj, QEvent *ev) override;

    /**
     * @brief Override of `QWidget::mousePressEvent()`
     *
     * It emits the clicked() signal
     *
     * @param ev the event
     */
    void mousePressEvent(QMouseEvent *ev) override;

    /**
     * @brief Brings up the context menu for this frame
     */
    virtual void splitFrameMenu();

private:
    KonqFrame *m_pParentKonqFrame; //!< The parent frame
    QCheckBox *m_pLinkedViewCheckBox; //!< The check box to toggle the linked status
    QProgressBar *m_progressBar; //!< The progress bar
    KonqStatusBarMessageLabel *m_pStatusLabel; //!< The label where the messages are actually shown
    QLabel *m_led; //!< The led to display the active view
    QString m_savedMessage; //!< The message to restore when slotClear() is called
};

#endif /* KONQ_FRAMESTATUSBAR_H */

