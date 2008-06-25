/*  This file is part of the KDE project
    Copyright (C) 1998, 1999 Michael Reiher <michael.reiher@gmx.de>
    Copyright 2007 David Faure <faure@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef KONQ_FRAMESTATUSBAR_H
#define KONQ_FRAMESTATUSBAR_H

#include <KStatusBar>
class QLabel;
class QProgressBar;
class QCheckBox;
class KonqView;
class KSqueezedTextLabel;
class KonqFrame;
namespace KParts { class ReadOnlyPart; }


/**
 * The KonqFrameStatusBar is the statusbar under each konqueror view.
 * It indicates in particular whether a view is active or not.
 */
class KonqFrameStatusBar : public KStatusBar
{
    Q_OBJECT

public:
    explicit KonqFrameStatusBar( KonqFrame *_parent = 0 );
    virtual ~KonqFrameStatusBar();

    /**
     * Checks/unchecks the linked-view checkbox
     */
    void setLinkedView( bool b );
    /**
     * Shows/hides the active-view indicator
     */
    void showActiveViewIndicator( bool b );
    /**
     * Shows/hides the linked-view indicator
     */
    void showLinkedViewIndicator( bool b );
    /**
     * Updates the active-view indicator and the statusbar color.
     */
    void updateActiveStatus();

public Q_SLOTS:
    void slotConnectToNewView(KonqView *, KParts::ReadOnlyPart *oldOne,KParts::ReadOnlyPart *newOne);
    void slotLoadingProgress( int percent );
    void slotSpeedProgress( int bytesPerSecond );
    void slotDisplayStatusText(const QString& text);

    void slotClear();
    void message ( const QString & message );

Q_SIGNALS:
    /**
     * This signal is emitted when the user clicked the bar.
     */
    void clicked();

    /**
     * The "linked view" checkbox was clicked
     */
    void linkedViewClicked( bool mode );

protected:
    virtual bool eventFilter(QObject*,QEvent *);
    virtual void mousePressEvent( QMouseEvent* );
    /**
     * Brings up the context menu for this frame
     */
    virtual void splitFrameMenu();

    /**
     * Takes care of the statusbars size
     **/
    virtual void fontChange(const QFont &oldFont);

private:
    KonqFrame* m_pParentKonqFrame;
    QCheckBox *m_pLinkedViewCheckBox;
    QProgressBar *m_progressBar;
    KSqueezedTextLabel *m_pStatusLabel;
    QLabel* m_led;
    QString m_savedMessage;
};

#endif /* KONQ_FRAMESTATUSBAR_H */

