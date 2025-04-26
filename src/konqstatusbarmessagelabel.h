/*
    SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz@gmx.at>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQ_STATUSBARMESSAGELABEL_H
#define KONQ_STATUSBARMESSAGELABEL_H

#include <QWidget>

class QPaintEvent;
class QResizeEvent;

/**
 * @brief Represents a message text label as part of the status bar.
 *
 * Dependent from the given type automatically a corresponding icon
 * is shown in front of the text. For message texts having the type
 * KonqStatusBarMessageLabel::Error a dynamic color blending is done to get the
 * attention from the user.
 *
 * In case of errore message, a close button is displayed next to the message.
 * Multiple error messages are queued: this means that only the most recent error message
 * is shown. Closing it will show the previous one and so on.
 */
class KonqStatusBarMessageLabel : public QWidget
{
    Q_OBJECT

public:

    /**
     * @brief Constructor
     *
     * @param parent the parent widget
     */
    explicit KonqStatusBarMessageLabel(QWidget *parent);
    ~KonqStatusBarMessageLabel() override; //!< Destructor

    /**
     * @brief Enum describing the type of the message text
     *
     * Dependent from the type, a corresponding icon and color is used for the message text.
     */
    enum Type {
        Default, //!< The default message type
        OperationCompleted, //!< A message telling that an operation has been completed
        Information, //!< A message for showing a piece of information
        Error //!< An error message
    };

    /**
     * @brief Displays a message with the given text and type
     *
     * @param text the text of the message
     * @param type the message type
     */
    void setMessage(const QString &text, Type type);

    /**
     * @brief The time of message which is currently shown
     */
    Type type() const;

    /**
     * @brief The currently displayed text
     * @return The text currently shown in the label
     */
    QString text() const;

    /**
     * @brief Sets the default text of the label
     *
     * This is the text that should be displayed when no messages are visible
     * @param text the default text
     */
    void setDefaultText(const QString &text);

    /**
     * @brief The default text
     * @return The text which is displayed when there there are no visible messages
     */
    QString defaultText() const;

    // TODO: maybe a better approach is possible with the size hint
    /**
     * @brief Sets the minimum height of the text
     *
     * @param min the new minimum text height
     */
    void setMinimumTextHeight(int min);

    /**
     * @brief The minimum text height
     * @return the minimum text height
     */
    int minimumTextHeight() const;

    /**
     * @brief Override of `QWidget::sizeHint()`
     *
     * @return the same size as minimumSizeHint()
     */
    QSize sizeHint() const override;

    /**
     * @brief Override of `QWidget::minimumSizeHint()`
     *
     * @return a size whose height is enough to show both the text and the close button (if visible)
     */
    QSize minimumSizeHint() const override;

protected:
    /**
     * @brief Override of `QWidget::paintEvent()`
     *
     * It draws the text, using an appropriate background in case of the transparency
     * animation for error messages is running.
     * @internal
     * In particular, if the @link KonqStatusBarMessageLabel::Private::m_illumination d->m_illumination@endlink
     * is greater than 0, it paints the background with the `NegativeBackround` color and a transparency level
     * which is twice @link KonqStatusBarMessageLabel::Private::m_illumination d->m_illumination@endlink. At the
     * end of the animation, this value is 125 so that the background is completely opaque and covers the widget
     * below it. If @link KonqStatusBarMessageLabel::Private::m_illumination d->m_illumination@endlink is 0, the
     * background is completely transparent so that it won't affect the widget appearance.
     *
     * @param event the paint event
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief Override of QWidget::resizeEvent()`
     *
     * It updates the position of the close button, if visible
     * @param event the resize event
     */
    void resizeEvent(QResizeEvent *event) override;

private Q_SLOTS:

    /**
     * @brief Handles dynamically changing the color of the label in case of an error message
     *
     * Depending on the state of the bar (Private::m_state), it changes the transparency of the background,
     * initially gradually increasing at small time intervals up to a maximum amount, then
     * leaving it constant for 5 seconds and then gradually reducing it to 0.
     */
    void timerDone();

    /**
     * @brief Changes the height of the label so that it's enough to show the text
     *
     * This function takes into account the width available for the text, according to
     * availableTextWidth() and the fact that the text can be split on multiple lines.
     *
     * This function never increases the label height if the message type is `Default`,
     * as that is the type of message used, for example, when hovering on a link and
     * such messages should never modify the widget layout.
     */
    void assureVisibleText();

    /**
     * @brief The available width in pixels for the text
     *
     * @return The widget's width reduced by the pixmap's width, the close button's width
     * (if visible) and the space among them
     */
    int availableTextWidth() const;

    /**
     * @brief Moves the close button to the upper right corner of the message label
     */
    void updateCloseButtonPosition();

    /**
     * @brief Closes the currently shown error message and replaces it by the next pending message
     */
    void closeErrorMessage();

private:
    /**
     * @brief Shows the next pending error message
     *
     * If no pending message was in the queue, nothing is done
     *
     * @return `true` if a message was displayed and `false` if the queue didn't contain
     * any messages
     */
    bool showPendingMessage();

    /**
     * @brief Resets the message label properties
     *
     * This clears any text and sets the message type to Default.
     *
     * This is useful when the * result of invoking KonqStatusBarMessageLabel::setMessage() should
     * not rely on previous states.
     */
    void reset();

private:
    /**
     * @brief Enum describing the state of the widget during the color changing animation when displaying an error message
     */
    enum State {
        DefaultState, //!< Background is not transparent and should remain so
        Illuminate, //!< Background transparency should be increased
        Illuminated, //!< Background transparency has reached its maximum value
        Desaturate //!< Background transparency should be decreased
    };

    class Private;
    Private *const d; //!< The d-pointer
};

#endif
