/*
    SPDX-FileCopyrightText: 2006 Peter Penz
    peter.penz@gmx.at

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
 */
class KonqStatusBarMessageLabel : public QWidget
{
    Q_OBJECT

public:
    explicit KonqStatusBarMessageLabel(QWidget *parent);
    ~KonqStatusBarMessageLabel() override;

    /**
     * Describes the type of the message text. Dependent
     * from the type a corresponding icon and color is
     * used for the message text.
     */
    enum Type {
        Default,
        OperationCompleted,
        Information,
        Error
    };

    void setMessage(const QString &text, Type type);

    Type type() const;

    QString text() const;

    void setDefaultText(const QString &text);
    QString defaultText() const;

    // TODO: maybe a better approach is possible with the size hint
    void setMinimumTextHeight(int min);
    int minimumTextHeight() const;

    /** @see QWidget::sizeHint */
    QSize sizeHint() const override;
    /** @see QWidget::minimumSizeHint */
    QSize minimumSizeHint() const override;

protected:
    /** @see QWidget::paintEvent() */
    void paintEvent(QPaintEvent *event) override;

    /** @see QWidget::resizeEvent() */
    void resizeEvent(QResizeEvent *event) override;

private Q_SLOTS:
    void timerDone();

    /**
     * Increases the height of the message label so that
     * the given text fits into given area.
     */
    void assureVisibleText();

    /**
     * Returns the available width in pixels for the text.
     */
    int availableTextWidth() const;

    /**
     * Moves the close button to the upper right corner
     * of the message label.
     */
    void updateCloseButtonPosition();

    /**
     * Closes the currently shown error message and replaces it
     * by the next pending message.
     */
    void closeErrorMessage();

private:
    /**
     * Shows the next pending error message. If no pending message
     * was in the queue, false is returned.
     */
    bool showPendingMessage();

    /**
     * Resets the message label properties. This is useful when the
     * result of invoking KonqStatusBarMessageLabel::setMessage() should
     * not rely on previous states.
     */
    void reset();

private:
    enum State {
        DefaultState,
        Illuminate,
        Illuminated,
        Desaturate
    };

    class Private;
    Private *const d;
};

#endif
