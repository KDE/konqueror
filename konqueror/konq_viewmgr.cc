/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
 
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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
*/ 

#include "konq_viewmgr.h"

#include "konq_mainview.h"
#include "konq_childview.h"
#include "konq_frame.h"
#include "konq_treeview.h"

#include <qsplitter.h>
#include <qstringlist.h>

#include <kconfig.h>
#include <browser.h>

KonqViewManager::KonqViewManager( KonqMainView *mainView )
{
  m_pMainView = mainView;

  m_lstRows.setAutoDelete( true );
  
  m_pMainSplitter = new QSplitter( Qt::Vertical, m_pMainView );
  m_pMainSplitter->setOpaqueResize();
  m_pMainSplitter->show();
}

KonqViewManager::~KonqViewManager()
{
  clear();
  delete m_pMainSplitter;
}

void KonqViewManager::saveViewProfile( KConfig &cfg )
{
  QList<KonqChildView> viewList;
  
  cfg.writeEntry( "MainSplitterSizes", QVariant( m_pMainSplitter->sizes() ) );

  cfg.writeEntry( "NumberOfRows", m_lstRows.count() );
 
  QListIterator<Konqueror::RowInfo> rowIt( m_lstRows );
  
  for ( int i = 0; rowIt.current(); ++rowIt, ++i )
  {
  
    QString rowStr = QString::fromLatin1( "Row%1" ).arg( i );    

    cfg.writeEntry( QString::fromLatin1( "SplitterSizes" ).prepend( rowStr ), QVariant( rowIt.current()->splitter->sizes() ) );

    QStringList strlst;
    QListIterator<KonqChildView> viewIt( rowIt.current()->children );
    
    for (; viewIt.current(); ++viewIt )
    {
      strlst.append( QString::number( viewList.count() ) );
      viewList.append( viewIt.current() );
    }
    
    cfg.writeEntry( QString::fromLatin1( "ChildViews" ).prepend( rowStr ), strlst );
  }
  
  QListIterator<KonqChildView> viewIt( viewList );
  for (int i = 0; viewIt.current(); ++viewIt, ++i )
  {

    QString viewStr = QString::fromLatin1( "View%1" ).arg( i );
    
    cfg.writeEntry( QString::fromLatin1( "URL" ).prepend( viewStr ), viewIt.current()->url() );
    cfg.writeEntry( QString::fromLatin1( "ServiceType" ).prepend( viewStr ), viewIt.current()->serviceTypes().first() );
    
    //HACK
    if ( viewIt.current()->viewName() == "KonquerorKfmTreeView" )
      cfg.writeEntry( QString::fromLatin1( "IsBuiltinTreeView" ).prepend( viewStr ), true );
  }
  
  cfg.sync();
}

void KonqViewManager::loadViewProfile( KConfig &cfg )
{
  bool bFirstViewActivated = false;

  clear();

  QValueList<int> mainSplitterSizes = 
    QVariant( cfg.readPropertyEntry( "MainSplitterSizes", QVariant::IntList ) )
    .intListValue();
  
  int rowCount = cfg.readNumEntry( "NumberOfRows" );
  
  for ( int i = 0; i < rowCount; ++i )
  {
    QString rowStr = QString::fromLatin1( "Row%1" ).arg( i );    

    QValueList<int> rowSplitterSizes = 
      QVariant( cfg.readPropertyEntry( 
       QString::fromLatin1( "SplitterSizes" ).prepend( rowStr ), QVariant::IntList ) )
      .intListValue();

    Konqueror::RowInfo *rowInfo = new Konqueror::RowInfo;
    
    QSplitter *rowSplitter = new QSplitter( Qt::Horizontal, m_pMainSplitter );
    rowSplitter->setOpaqueResize();
    rowSplitter->show();
    
    rowInfo->splitter = rowSplitter;

    QStringList childList = cfg.readListEntry( 
      QString::fromLatin1( "ChildViews" ).prepend( rowStr ) );

    QStringList::ConstIterator cIt = childList.begin();
    QStringList::ConstIterator cEnd = childList.end();
    for (; cIt != cEnd; ++cIt  )
    {
      QString viewStr = QString::fromLatin1( "View" ).append( *cIt );

      QString url = cfg.readEntry( QString::fromLatin1( "URL" ).prepend( viewStr ) );
      QString serviceType = cfg.readEntry( QString::fromLatin1( "ServiceType" ).prepend( viewStr ) );

      //HACK
      QString treeViewKey = QString::fromLatin1( "IsBuiltinTreeView" ).prepend( viewStr );
      bool treeView = ( cfg.hasKey( treeViewKey ) &&
                        cfg.readBoolEntry( treeViewKey, false ) &&
			serviceType == "inode/directory" );
      
      Browser::View_var vView;
      QStringList serviceTypes;
      
      if ( treeView )
      {
        //HACK
        vView = Browser::View::_duplicate( new KonqKfmTreeView( m_pMainView ) );
	serviceTypes.append( "inode/directory" );
      }
      else
      {
        //Simon TODO: error handling
        KonqChildView::createView( serviceType, vView, serviceTypes, m_pMainView );
      }
      
      setupView( rowInfo, vView, serviceTypes );
      
      m_pMainView->childView( vView->id() )->openURL( url );

      if ( !bFirstViewActivated )
      {
        OpenParts::MainWindow_var vMainWindow = m_pMainView->mainWindow();
	
	if ( vMainWindow->activePartId() == m_pMainView->id() )
	  vMainWindow->setActivePart( vView->id() );
	
        bFirstViewActivated = true;
      }
      
    }

    rowInfo->splitter->setSizes( rowSplitterSizes );
    
    m_lstRows.append( rowInfo );
    
  }  

  m_pMainSplitter->setSizes( mainSplitterSizes );  
}

void KonqViewManager::insertView ( Qt::Orientation orientation, 
			          Browser::View_ptr newView,
			          const QStringList &newViewServiceTypes )
{
/*
  The splitter layout looks like this:
  
  the mainsplitter is a vertical splitter, holding only horizontal splitters
  
  the horizontal splitters, childs of the mainsplitter, actually hold the
  views (viewframes) as children
 */

  QSplitter *parentSplitter;
  Konqueror::RowInfo *rowInfo;

  if ( orientation == Qt::Horizontal )
    rowInfo = m_pMainView->currentChildView()->rowInfo();
  else
  {
    parentSplitter = new QSplitter( Qt::Horizontal, m_pMainSplitter );
    parentSplitter->setOpaqueResize();
    parentSplitter->show();
    
    rowInfo = new Konqueror::RowInfo;
    rowInfo->splitter = parentSplitter;
    m_lstRows.append( rowInfo );
  }

  setupView( rowInfo, newView, newViewServiceTypes );

  if ( orientation == Qt::Vertical )
  {
    QValueList<int> sizes = m_pMainSplitter->sizes();
    QValueList<int>::Iterator it = sizes.fromLast();
    sizes.remove( it );
    sizes.append( 100 );
    
    m_pMainSplitter->setSizes( sizes );
  }
}

void KonqViewManager::removeView( KonqChildView *view )
{
  bool deleteParentSplitter = false;
  QSplitter *parentSplitter = view->frame()->parentSplitter();

  Konqueror::RowInfo *row = view->rowInfo();
  row->children.removeRef( view );

  if ( row->children.count() == 0 )
  {  
    m_lstRows.removeRef( row );
    deleteParentSplitter = true;
  }	  

  m_pMainView->removeChildView( view->id() );

  delete view;
  
  if ( deleteParentSplitter )
    delete parentSplitter;
}

void KonqViewManager::clear()
{
  QListIterator<Konqueror::RowInfo> it( m_lstRows );
  
  while ( it.current() )
    clearRow( it.current() );
}

void KonqViewManager::doGeometry( int width, int height )
{
  m_pMainSplitter->setGeometry( 0, 0, width, height );
}

KonqChildView *KonqViewManager::chooseNextView( KonqChildView *view )
{
  Konqueror::RowInfo *row = view->rowInfo();
  KonqChildView *next = 0L;
  
  if ( !view )
    return m_lstRows.getFirst()->children.getFirst();
  
  if ( m_lstRows.count() == 1 && row->children.count() == 1 )
    return view;
    
  if ( row->children.count() > 1 )
  {
    KonqChildView *savedCurrentChild = row->children.current();
    
    row->children.findRef( view );
    
    next = row->children.next();
    if ( !next )
    {
      row->children.findRef( view );
      next = row->children.prev();
    }
    
    row->children.findRef( savedCurrentChild );
  }
  else
  {
    Konqueror::RowInfo *savedCurrentRow = m_lstRows.current();
  
    m_lstRows.findRef( row );
    
    Konqueror::RowInfo *nextRow = m_lstRows.next();
    if ( !nextRow )
    {
      m_lstRows.findRef( row );
      nextRow = m_lstRows.prev();
    }
    
    if ( nextRow && nextRow->children.count() > 0 )
      next = nextRow->children.getFirst();
    
    m_lstRows.findRef( savedCurrentRow );
  }
  
  return next;
}

void KonqViewManager::clearRow( Konqueror::RowInfo *row )
{
  QListIterator<KonqChildView> it( row->children );
  
  while ( it.current() )
    removeView( it.current() );
}

void KonqViewManager::setupView( Konqueror::RowInfo *row, Browser::View_ptr view, const QStringList &serviceTypes )
{
  KonqFrame* newViewFrame = new KonqFrame( row->splitter );

  KonqChildView *v = new KonqChildView( view, newViewFrame, 
					m_pMainView, serviceTypes );

  v->setRowInfo( row );
  row->children.append( v );

  QObject::connect( v, SIGNAL( sigIdChanged( KonqChildView *, OpenParts::Id, OpenParts::Id ) ), 
                    m_pMainView, SLOT( slotIdChanged( KonqChildView * , OpenParts::Id, OpenParts::Id ) ) );

  m_pMainView->insertChildView( v );

  v->lockHistory();
}
