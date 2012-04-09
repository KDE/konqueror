/*  This file is part of the KDE project
    Copyright (C) 1998, 1999 Michael Reiher <michael.reiher@gmx.de>

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

#ifndef __konq_frame_h__
#define __konq_frame_h__

#include "konqfactory.h"
#include <kparts/part.h> // for the inline QPointer usage

#include <QtCore/QPointer>
#include <QColor>
#include <QWidget>
#include <QCheckBox>
#include <QLabel>
#include <QPixmap>
#include <QKeyEvent>
#include <QtCore/QEvent>
#include <QtCore/QList>

#include <KConfig>

class KonqFrameStatusBar;
class KonqFrameVisitor;
class QPixmap;
class QVBoxLayout;
class QProgressBar;

class KonqView;
class KonqViewManager;
class KonqFrameBase;
class KonqFrame;
class KonqFrameContainerBase;
class KonqFrameContainer;
class KConfig;
class KSeparator;
class KSqueezedTextLabel;

namespace KParts
{
  class ReadOnlyPart;
}

typedef QList<KonqView*> ChildViewList;

class KONQ_TESTS_EXPORT KonqFrameBase
{
 public:
    enum Option {
        None = 0x0,
        saveURLs = 0x01, // TODO rename to SaveUrls
        saveHistoryItems = 0x02 // TODO rename to SaveHistoryItems
    };
    Q_DECLARE_FLAGS(Options, Option)

  enum FrameType { View, Tabs, ContainerBase, Container, MainWindow };

  virtual ~KonqFrameBase() {}

    virtual bool isContainer() const = 0;

    virtual bool accept( KonqFrameVisitor* visitor ) = 0;

  virtual void saveConfig( KConfigGroup& config, const QString &prefix, const KonqFrameBase::Options &options, KonqFrameBase* docContainer, int id = 0, int depth = 0) = 0;

  virtual void copyHistory( KonqFrameBase *other ) = 0;

  KonqFrameContainerBase* parentContainer() const { return m_pParentContainer; }
  void setParentContainer(KonqFrameContainerBase* parent) { m_pParentContainer = parent; }

  virtual void setTitle( const QString &title , QWidget* sender) = 0;
  virtual void setTabIcon( const KUrl &url, QWidget* sender ) = 0;

  virtual QWidget* asQWidget() = 0;

  virtual FrameType frameType() const = 0;

  virtual void activateChild() = 0;

  virtual KonqView* activeChildView() const = 0;

  static QString frameTypeToString( const FrameType frameType );
  static FrameType frameTypeFromString( const QString& str );

protected:
  KonqFrameBase();

  KonqFrameContainerBase* m_pParentContainer;
};

  Q_DECLARE_OPERATORS_FOR_FLAGS ( KonqFrameBase::Options )

/**
 * The KonqFrame is the actual container for the views. It takes care of the
 * widget handling i.e. it attaches/detaches the view widget and activates
 * them on click at the statusbar.
 *
 * We create a vertical layout in the frame, with the view and the KonqFrameStatusBar.
 */

class KONQ_TESTS_EXPORT KonqFrame : public QWidget, public KonqFrameBase
{
  Q_OBJECT

public:
  explicit KonqFrame( QWidget* parent, KonqFrameContainerBase *parentContainer = 0 );
  virtual ~KonqFrame();

    virtual bool isContainer() const { return false; }

    virtual bool accept( KonqFrameVisitor* visitor );

  /**
   * Attach a view to the KonqFrame.
   * @param viewFactory the view to attach (instead of the current one, if any)
   */
  KParts::ReadOnlyPart *attach( const KonqViewFactory &viewFactory );

  /**
   * Inserts the widget and the statusbar into the layout
   */
  void attachWidget(QWidget* widget);

  /**
   * Inserts a widget at the top of the part's widget, in the layout
   * (used for the find functionality)
   */
  void insertTopWidget( QWidget * widget );

  /**
   * Returns the part that is currently connected to the Frame.
   */
  KParts::ReadOnlyPart *part() { return m_pPart; }
  /**
   * Returns the view that is currently connected to the Frame.
   */
  KonqView* childView() const;

  bool isActivePart();

  void setView( KonqView* child );

  virtual void saveConfig( KConfigGroup& config, const QString &prefix, const KonqFrameBase::Options &options, KonqFrameBase* docContainer, int id = 0, int depth = 0 );
  virtual void copyHistory( KonqFrameBase *other );

  virtual void setTitle( const QString &title, QWidget* sender );
  virtual void setTabIcon( const KUrl &url, QWidget* sender );

  virtual QWidget* asQWidget() { return this; }
  virtual KonqFrameBase::FrameType frameType() const { return KonqFrameBase::View; }

  QVBoxLayout *layout()const { return m_pLayout; }

  KonqFrameStatusBar *statusbar() const { return m_pStatusBar; }

  virtual void activateChild();

  virtual KonqView* activeChildView() const;

  QString title() const { return m_title; }

public Q_SLOTS:

  /**
   * Is called when the frame statusbar has been clicked
   */
  void slotStatusBarClicked();

  void slotLinkedViewClicked( bool mode );

  /**
   * Is called when 'Remove View' is called from the popup menu
   */
  void slotRemoveView();

private:
  QVBoxLayout *m_pLayout;
  QPointer<KonqView> m_pView;

  QPointer<KParts::ReadOnlyPart> m_pPart;

  KSeparator *m_separator;
  KonqFrameStatusBar* m_pStatusBar;

  QString m_title;
};

#endif
