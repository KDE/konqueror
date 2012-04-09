/*  This file is part of the KDE project

    Copyright (C) 2002-2003 Konqueror Developers <konq-e@kde.org>
    Copyright (C) 2002-2003 Douglas Hanley <douglash@caltech.edu>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA  02110-1301, USA.
*/

#ifndef KONQTABS_H
#define KONQTABS_H

#include "konqframe.h"
#include "konqframecontainer.h"

#include <ktabwidget.h>
#include <QKeyEvent>
#include <QtCore/QList>

class QMenu;
class QToolButton;

class KonqView;
class KonqViewManager;
class KonqFrameContainerBase;
class KonqFrameContainer;
class KConfig;

class NewTabToolButton;

class KONQ_TESTS_EXPORT KonqFrameTabs : public KTabWidget, public KonqFrameContainerBase
{
  Q_OBJECT

public:
  KonqFrameTabs(QWidget* parent, KonqFrameContainerBase* parentContainer,
		KonqViewManager* viewManager);
  virtual ~KonqFrameTabs();

    virtual bool accept( KonqFrameVisitor* visitor );

  virtual void saveConfig( KConfigGroup& config, const QString &prefix, const KonqFrameBase::Options &options,
			   KonqFrameBase* docContainer, int id = 0, int depth = 0 );
  virtual void copyHistory( KonqFrameBase *other );

  const QList<KonqFrameBase*>& childFrameList() const { return m_childFrameList; }

  virtual void setTitle( const QString &title, QWidget* sender );
  virtual void setTabIcon( const KUrl &url, QWidget* sender );

  virtual QWidget* asQWidget() { return this; }
  virtual KonqFrameBase::FrameType frameType() const { return KonqFrameBase::Tabs; }

  void activateChild();

    /**
     * Insert a new frame into the container.
     */
    void insertChildFrame(KonqFrameBase * frame, int index = -1);

    /**
     * Call this before deleting one of our children.
     */
    void childFrameRemoved( KonqFrameBase * frame );

    virtual void replaceChildFrame(KonqFrameBase* oldFrame, KonqFrameBase* newFrame);

    /**
     * Returns the tab at a given index
     * (same as widget(index), but casted to a KonqFrameBase)
     */
    KonqFrameBase* tabAt(int index) const;

    /**
     * Returns the current tab
     * (same as currentWidget(), but casted to a KonqFrameBase)
     */
    KonqFrameBase* currentTab() const;

  void moveTabBackward(int index);
  void moveTabForward(int index);

  void setLoading(KonqFrameBase* frame, bool loading);

  /**
   * Returns the tab index that contains (directly or indirectly) the frame @p frame,
   * or -1 if the frame is not in the tab widget.
   */
  int tabIndexContaining(KonqFrameBase* frame) const;

public Q_SLOTS:
  void slotCurrentChanged( int index );
  void setAlwaysTabbedMode( bool );

Q_SIGNALS:
  void removeTabPopup();
    void openUrl(KonqView* view, const KUrl& url);

private:
  void refreshSubPopupMenuTab();
    void updateTabBarVisibility();
    void initPopupMenu();
   /**
    * Returns the index position of the tab where the frame @p frame is, assuming that
    * it's the active frame in that tab,
    * or -1 if the frame is not in the tab widget or it's not active.
    */
    int tabWhereActive(KonqFrameBase* frame) const;

private Q_SLOTS:
  void slotContextMenu( const QPoint& );
  void slotContextMenu( QWidget*, const QPoint& );
  void slotCloseRequest( QWidget* );
  void slotMovedTab( int, int );
  void slotMouseMiddleClick();
  void slotMouseMiddleClick( QWidget* );

  void slotTestCanDecode(const QDragMoveEvent *e, bool &accept /* result */);
  void slotReceivedDropEvent( QDropEvent* );
  void slotInitiateDrag( QWidget * );
  void slotReceivedDropEvent( QWidget *, QDropEvent * );
  void slotSubPopupMenuTabActivated( QAction * );

private:
  QList<KonqFrameBase*> m_childFrameList;

  KonqViewManager* m_pViewManager;
  QMenu* m_pPopupMenu;
  QMenu* m_pSubPopupMenuTab;
  QToolButton* m_rightWidget;
  NewTabToolButton* m_leftWidget;
  bool m_permanentCloseButtons;
  bool m_alwaysTabBar;
  QMap<QString,QAction*> m_popupActions;
};

#include <QToolButton>

class NewTabToolButton : public QToolButton // subclass with drag'n'drop functionality for links
{
    Q_OBJECT
public:
    NewTabToolButton( QWidget *parent )
            : QToolButton( parent ) {
        setAcceptDrops( true );
    }

Q_SIGNALS:
    void testCanDecode( const QDragMoveEvent *event, bool &accept );
    void receivedDropEvent( QDropEvent *event );

protected:
    void dragEnterEvent( QDragEnterEvent *event )
    {
        bool accept = false;
        emit testCanDecode( event, accept );
        if ( accept ) {
            event->acceptProposedAction();
        }
    }

    void dropEvent( QDropEvent *event )
    {
        emit receivedDropEvent( event );
        event->acceptProposedAction();
    }
};

#endif // KONQTABS_H
