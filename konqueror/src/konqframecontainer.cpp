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

#include "konqframecontainer.h"
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kconfig.h>
#include <math.h> // pow()

#include "konqframevisitor.h"

void KonqFrameContainerBase::replaceChildFrame(KonqFrameBase* oldFrame, KonqFrameBase* newFrame)
{
    childFrameRemoved(oldFrame);
    insertChildFrame(newFrame);
}

KonqFrameContainer* KonqFrameContainerBase::splitChildFrame(KonqFrameBase* splitFrame, Qt::Orientation orientation)
{
    KonqFrameContainer *newContainer = new KonqFrameContainer(orientation, asQWidget(), this);
    replaceChildFrame(splitFrame, newContainer);
    newContainer->insertChildFrame(splitFrame);
    return newContainer;
}

////

KonqFrameContainer::KonqFrameContainer( Qt::Orientation o,
                                        QWidget* parent,
                                        KonqFrameContainerBase* parentContainer )
  : QSplitter( o, parent ), m_bAboutToBeDeleted(false)
{
  m_pParentContainer = parentContainer;
  m_pFirstChild = 0L;
  m_pSecondChild = 0L;
  m_pActiveChild = 0L;
  setOpaqueResize( KGlobalSettings::opaqueResize() );
  connect(this, SIGNAL(splitterMoved(int, int)), this, SIGNAL(setRubberbandCalled()));
//### CHECKME
}

KonqFrameContainer::~KonqFrameContainer()
{
    //kDebug(1202) << "KonqFrameContainer::~KonqFrameContainer() " << this << " - " << className() << endl;
    delete m_pFirstChild;
    delete m_pSecondChild;
}

void KonqFrameContainer::saveConfig( KConfigGroup& config, const QString &prefix, bool saveURLs, KonqFrameBase* docContainer, int id, int depth )
{
  int idSecond = id + (int)pow( 2.0, depth );

  //write children sizes
  config.writeEntry( QString::fromLatin1( "SplitterSizes" ).prepend( prefix ), sizes() );

  //write children
  QStringList strlst;
  if( firstChild() )
    strlst.append( QString::fromLatin1( firstChild()->frameType() ) + QString::number(idSecond - 1) );
  if( secondChild() )
    strlst.append( QString::fromLatin1( secondChild()->frameType() ) + QString::number( idSecond ) );

  config.writeEntry( QString::fromLatin1( "Children" ).prepend( prefix ), strlst );

  //write orientation
  QString o;
  if( orientation() == Qt::Horizontal )
    o = QString::fromLatin1("Horizontal");
  else if( orientation() == Qt::Vertical )
    o = QString::fromLatin1("Vertical");
  config.writeEntry( QString::fromLatin1( "Orientation" ).prepend( prefix ), o );

  //write docContainer
  if (this == docContainer) config.writeEntry( QString::fromLatin1( "docContainer" ).prepend( prefix ), true );

  if (m_pSecondChild == m_pActiveChild) config.writeEntry( QString::fromLatin1( "activeChildIndex" ).prepend( prefix ), 1 );
  else config.writeEntry( QString::fromLatin1( "activeChildIndex" ).prepend( prefix ), 0 );

  //write child configs
  if( firstChild() ) {
    QString newPrefix = QString::fromLatin1( firstChild()->frameType() ) + QString::number(idSecond - 1);
    newPrefix.append( QLatin1Char( '_' ) );
    firstChild()->saveConfig( config, newPrefix, saveURLs, docContainer, id, depth + 1 );
  }

  if( secondChild() ) {
    QString newPrefix = QString::fromLatin1( secondChild()->frameType() ) + QString::number( idSecond );
    newPrefix.append( QLatin1Char( '_' ) );
    secondChild()->saveConfig( config, newPrefix, saveURLs, docContainer, idSecond, depth + 1 );
  }
}

void KonqFrameContainer::copyHistory( KonqFrameBase *other )
{
    Q_ASSERT( other->frameType() == "Container" );
    if ( firstChild() )
        firstChild()->copyHistory( static_cast<KonqFrameContainer *>( other )->firstChild() );
    if ( secondChild() )
        secondChild()->copyHistory( static_cast<KonqFrameContainer *>( other )->secondChild() );
}

KonqFrameBase* KonqFrameContainer::otherChild( KonqFrameBase* child )
{
    if( m_pFirstChild == child )
        return m_pSecondChild;
    else if( m_pSecondChild == child )
        return m_pFirstChild;
    return 0;
}

void KonqFrameContainer::swapChildren()
{
    qSwap( m_pFirstChild, m_pSecondChild );
}

void KonqFrameContainer::setTitle( const QString &title , QWidget* sender)
{
  //kDebug(1202) << "KonqFrameContainer::setTitle( " << title << " , " << sender << " )" << endl;
  if (m_pParentContainer && activeChild() && (sender == activeChild()->asQWidget()))
      m_pParentContainer->setTitle( title , this);
}

void KonqFrameContainer::setTabIcon( const KUrl &url, QWidget* sender )
{
  //kDebug(1202) << "KonqFrameContainer::setTabIcon( " << url << " , " << sender << " )" << endl;
  if (m_pParentContainer && activeChild() && (sender == activeChild()->asQWidget()))
      m_pParentContainer->setTabIcon( url, this );
}

void KonqFrameContainer::insertChildFrame(KonqFrameBase* frame, int index)
{
    //kDebug(1202) << "KonqFrameContainer " << this << ": insertChildFrame " << frame << endl;
    if (frame) {
        QSplitter::insertWidget(index, frame->asQWidget());
        // Insert before existing child? Move first to second.
        if (index == 0 && m_pFirstChild && !m_pSecondChild) {
            qSwap( m_pFirstChild, m_pSecondChild );
        }
        if( !m_pFirstChild ) {
            m_pFirstChild = frame;
            frame->setParentContainer(this);
            //kDebug(1202) << "Setting as first child" << endl;
        } else if( !m_pSecondChild ) {
            m_pSecondChild = frame;
            frame->setParentContainer(this);
            //kDebug(1202) << "Setting as second child" << endl;
        } else {
            kWarning(1202) << this << " already has two children..."
                           << m_pFirstChild << " and " << m_pSecondChild << endl;
        }
    } else {
        kWarning(1202) << "KonqFrameContainer " << this << ": insertChildFrame(NULL) !" << endl;
    }
}

void KonqFrameContainer::childFrameRemoved(KonqFrameBase * frame)
{
    //kDebug(1202) << "KonqFrameContainer::RemoveChildFrame " << this << ". Child " << frame << " removed" << endl;

    if( m_pFirstChild == frame ) {
        m_pFirstChild = m_pSecondChild;
        m_pSecondChild = 0;
    } else if( m_pSecondChild == frame ) {
        m_pSecondChild = 0;
    } else {
        kWarning(1202) << this << " Can't find this child:" << frame << endl;
    }
}

void KonqFrameContainer::childEvent( QChildEvent *c )
{
    // Child events cause layout changes. These are unnecessary if we are going
    // to be deleted anyway.
    if (!m_bAboutToBeDeleted)
        QSplitter::childEvent(c);
}

bool KonqFrameContainer::accept( KonqFrameVisitor* visitor )
{
    if ( !visitor->visit( this ) )
        return false;
    Q_ASSERT( m_pFirstChild );
    if ( m_pFirstChild && !m_pFirstChild->accept( visitor ) )
        return false;
    Q_ASSERT( m_pSecondChild );
    if ( m_pSecondChild && !m_pSecondChild->accept( visitor ) )
        return false;
    if ( !visitor->endVisit( this ) )
        return false;
    return true;
}

void KonqFrameContainer::replaceChildFrame(KonqFrameBase* oldFrame, KonqFrameBase* newFrame)
{
    const int idx = QSplitter::indexOf(oldFrame->asQWidget());
    const QList<int> splitterSizes = sizes();
    childFrameRemoved(oldFrame);
    insertChildFrame(newFrame, idx);
    setSizes(splitterSizes);
}

// TODO remove hasWidgetAfter

#include "konqframecontainer.moc"
