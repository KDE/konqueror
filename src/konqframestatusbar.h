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
 * The KonqFrameStatusBar is the statusbar under each konqueror view.
 * It indicates in particular whether a view is active or not.
 */
class KonqFrameStatusBar : public QStatusBar
{
    Q_OBJECT

public:
    explicit KonqFrameStatusBar(KonqFrame *_parent = nullptr);
    ~KonqFrameStatusBar() override;

    void setMessage(const QString &msg, KonqStatusBarMessageLabel::Type type);

    /**
     * Checks/unchecks the linked-view checkbox
     */
    void setLinkedView(bool b);
    /**
     * Shows/hides the active-view indicator
     */
    void showActiveViewIndicator(bool b);
    /**
     * Shows/hides the linked-view indicator
     */
    void showLinkedViewIndicator(bool b);
    /**
     * Updates the active-view indicator and the statusbar color.
     */
    void updateActiveStatus();

public Q_SLOTS:
    void slotConnectToNewView(KonqView *, KParts::ReadOnlyPart *oldOne, KParts::ReadOnlyPart *newOne);
    void slotLoadingProgress(int percent);
    void slotSpeedProgress(int bytesPerSecond);
    void slotDisplayStatusText(const QString &text);

    void slotClear();
    void message(const QString &message);

Q_SIGNALS:
    /**
     * This signal is emitted when the user clicked the bar.
     */
    void clicked();

    /**
     * The "linked view" checkbox was clicked
     */
    void linkedViewClicked(bool mode);

protected:
    bool eventFilter(QObject *, QEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    /**
     * Brings up the context menu for this frame
     */
    virtual void splitFrameMenu();

private:
    KonqFrame *m_pParentKonqFrame;
    QCheckBox *m_pLinkedViewCheckBox;
    QProgressBar *m_progressBar;
    KonqStatusBarMessageLabel *m_pStatusLabel;
    QLabel *m_led;
    QString m_savedMessage;
};

#endif /* KONQ_FRAMESTATUSBAR_H */

