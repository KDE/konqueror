/* This file is part of the KDE projects
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "konq_iconview.h"
#include "konq_propsview.h"

#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <kaccel.h>
#include <kaction.h>
#include <kdebug.h>
#include <konq_dirlister.h>
#include <kfileivi.h>
#include <konq_fileitem.h>
#include <kio/job.h>
#include <klibloader.h>
#include <klineeditdlg.h>
#include <kmimetype.h>
#include <konq_iconviewwidget.h>
#include <konq_settings.h>
#include <kpropsdlg.h>
#include <krun.h>
#include <kstdaction.h>
#include <kurl.h>
#include <kparts/factory.h>
#include <kiconloader.h>
#include <kapp.h>

#include <qmessagebox.h>
#include <qfile.h>
#include <qkeycode.h>
#include <qpalette.h>
#include <klocale.h>
#include <qregexp.h>
#include <qvaluelist.h>
#include <qpainter.h>
#include <qpen.h>

#include <config.h>

template class QList<KFileIVI>;
//template class QValueList<int>;

class KonqIconViewFactory : public KParts::Factory
{
public:
   KonqIconViewFactory()
   {
      s_defaultViewProps = 0;
      s_instance = 0;
   }

   virtual ~KonqIconViewFactory()
   {
      if ( s_instance )
         delete s_instance;

      if ( s_defaultViewProps )
         delete s_defaultViewProps;

      s_instance = 0;
      s_defaultViewProps = 0;
   }

    virtual KParts::Part* createPart( QWidget *parentWidget, const char *,
                                      QObject *parent, const char *name, const char*, const QStringList &args )
   {
      if( args.count() < 1 )
         kdWarning() << "KonqKfmIconView: Missing Parameter" << endl;

      KonqKfmIconView *obj = new KonqKfmIconView( parentWidget, parent, name,args.first() );
      emit objectCreated( obj );
      return obj;
   }

   static KInstance *instance()
   {
      if ( !s_instance )
         s_instance = new KInstance( "konqiconview" );
      return s_instance;
   }

   static KonqPropsView *defaultViewProps()
   {
      if ( !s_defaultViewProps )
         s_defaultViewProps = new KonqPropsView( instance(), 0L );

      return s_defaultViewProps;
   }

   private:
      static KInstance *s_instance;
      static KonqPropsView *s_defaultViewProps;
};

KInstance *KonqIconViewFactory::s_instance = 0;
KonqPropsView *KonqIconViewFactory::s_defaultViewProps = 0;

extern "C"
{
    void *init_libkonqiconview()
    {
        return new KonqIconViewFactory;
    }
};

IconViewBrowserExtension::IconViewBrowserExtension( KonqKfmIconView *iconView )
 : KParts::BrowserExtension( iconView )
{
  m_iconView = iconView;
  m_bSaveViewPropertiesLocally = false;
}

int IconViewBrowserExtension::xOffset()
{
  return m_iconView->iconViewWidget()->contentsX();
}

int IconViewBrowserExtension::yOffset()
{
  return m_iconView->iconViewWidget()->contentsY();
}

void IconViewBrowserExtension::reparseConfiguration()
{
    KonqFMSettings::reparseConfiguration();
    // m_pProps is a problem here (what is local, what is global ?)
    // but settings is easy :
    m_iconView->iconViewWidget()->initConfig();
}

void IconViewBrowserExtension::properties()
{
    KFileItem * item = m_iconView->iconViewWidget()->selectedFileItems().first();
    (void) new KPropertiesDialog( item );
}

void IconViewBrowserExtension::editMimeType()
{
    KFileItem * item = m_iconView->iconViewWidget()->selectedFileItems().first();
    KonqOperations::editMimeType( item->mimetype() );
}

void IconViewBrowserExtension::setSaveViewPropertiesLocally( bool value )
{
  m_iconView->m_pProps->setSaveViewPropertiesLocally( value );
}

void IconViewBrowserExtension::setNameFilter( QString nameFilter )
{
  //kdDebug(1202) << "IconViewBrowserExtension::setNameFilter " << nameFilter << endl;
  m_iconView->m_nameFilter = nameFilter;
}


KonqKfmIconView::KonqKfmIconView( QWidget *parentWidget, QObject *parent, const char *name, const QString& mode  )
    : KonqDirPart( parent, name ), m_itemDict( 43 )
{
    kdDebug(1202) << "+KonqKfmIconView" << endl;

    setBrowserExtension( new IconViewBrowserExtension( this ) );

    // Create a properties instance for this view
    m_pProps = new KonqPropsView( KonqIconViewFactory::instance(), KonqIconViewFactory::defaultViewProps() );

    m_pIconView = new KonqIconViewWidget( parentWidget, "qiconview" );
    m_pIconView->initConfig();

    QTimer * timer = new QTimer( this );
    connect( timer, SIGNAL( timeout() ), this, SLOT( slotProcessMimeIcons() ) );
    m_mimeTypeResolver = new KonqMimeTypeResolver<KFileIVI,KonqKfmIconView>(this,timer);

    // When our viewport is adjusted (resized or scrolled) we need
    // to get the mime types for any newly visible icons. (Rikkus)
    connect( m_pIconView, SIGNAL( viewportAdjusted() ),
             SLOT( slotViewportAdjusted() ) );

    connect( m_pIconView,  SIGNAL(imagePreviewFinished()),
             this, SLOT(slotRenderingFinished()));

    // pass signals to the extension
    connect( m_pIconView, SIGNAL( enableAction( const char *, bool ) ),
             m_extension, SIGNAL( enableAction( const char *, bool ) ) );

    setWidget( m_pIconView );

    setInstance( KonqIconViewFactory::instance() );

    setXMLFile( "konq_iconview.rc" );

    // Don't repaint on configuration changes during construction
    m_bInit = true;

    m_paDotFiles = new KToggleAction( i18n( "Show &Hidden Files" ), 0, this, SLOT( slotShowDot() ), actionCollection(), "show_dot" );
    m_paImagePreview = new KToggleAction( i18n( "&Image Preview" ), 0, actionCollection(), "image_preview" );

    connect( m_paImagePreview, SIGNAL( toggled( bool ) ), this, SLOT( slotImagePreview( bool ) ) );

    //    m_pamSort = new KActionMenu( i18n( "Sort..." ), actionCollection(), "sort" );

    KToggleAction *aSortByNameCS = new KRadioAction( i18n( "by Name (Case Sensitive)" ), 0, actionCollection(), "sort_nc" );
    KToggleAction *aSortByNameCI = new KRadioAction( i18n( "by Name (Case Insensitive)" ), 0, actionCollection(), "sort_nci" );
    KToggleAction *aSortBySize = new KRadioAction( i18n( "by Size" ), 0, actionCollection(), "sort_size" );
    KToggleAction *aSortByType = new KRadioAction( i18n( "by Type" ), 0, actionCollection(), "sort_type" );

    aSortByNameCS->setExclusiveGroup( "sorting" );
    aSortByNameCI->setExclusiveGroup( "sorting" );
    aSortBySize->setExclusiveGroup( "sorting" );
    aSortByType->setExclusiveGroup( "sorting" );

    aSortByNameCS->setChecked( false );
    aSortByNameCI->setChecked( true );
    aSortBySize->setChecked( false );
    aSortByType->setChecked( false );

    connect( aSortByNameCS, SIGNAL( toggled( bool ) ), this, SLOT( slotSortByNameCaseSensitive( bool ) ) );
    connect( aSortByNameCI, SIGNAL( toggled( bool ) ), this, SLOT( slotSortByNameCaseInsensitive( bool ) ) );
    connect( aSortBySize, SIGNAL( toggled( bool ) ), this, SLOT( slotSortBySize( bool ) ) );
    connect( aSortByType, SIGNAL( toggled( bool ) ), this, SLOT( slotSortByType( bool ) ) );

    m_paSortDirsFirst = new KToggleAction( i18n( "Directories first" ), 0, actionCollection(), "sort_directoriesfirst" );
    KToggleAction *aSortDescending = new KToggleAction( i18n( "Descending" ), 0, actionCollection(), "sort_descend" );

    m_paSortDirsFirst->setChecked( true );

    connect( aSortDescending, SIGNAL( toggled( bool ) ), this, SLOT( slotSortDescending() ) );
    connect( m_paSortDirsFirst, SIGNAL( toggled( bool ) ), this, SLOT( slotSortDirsFirst() ) );
    /*
    m_pamSort->insert( aSortByNameCS );
    m_pamSort->insert( aSortByNameCI );
    m_pamSort->insert( aSortBySize );

    m_pamSort->popupMenu()->insertSeparator();

    m_pamSort->insert( aSortDescending );
    */
    m_paSelect = new KAction( i18n( "&Select..." ), CTRL+Key_Plus, this, SLOT( slotSelect() ), actionCollection(), "select" );
    m_paUnselect = new KAction( i18n( "&Unselect..." ), CTRL+Key_Minus, this, SLOT( slotUnselect() ), actionCollection(), "unselect" );
    m_paSelectAll = KStdAction::selectAll( this, SLOT( slotSelectAll() ), this, "selectall" );
    m_paUnselectAll = new KAction( i18n( "U&nselect All" ), CTRL+Key_U, this, SLOT( slotUnselectAll() ), actionCollection(), "unselectall" );
    m_paInvertSelection = new KAction( i18n( "&Invert Selection" ), CTRL+Key_Asterisk, this, SLOT( slotInvertSelection() ), actionCollection(), "invertselection" );

    m_paDefaultIcons = new KRadioAction( i18n( "&Default Size" ), 0, actionCollection(), "modedefault" );
    m_paLargeIcons = new KRadioAction( i18n( "&Large" ), 0, actionCollection(), "modelarge" );
    m_paMediumIcons = new KRadioAction( i18n( "&Medium" ), 0, actionCollection(), "modemedium" );
    m_paSmallIcons = new KRadioAction( i18n( "&Small" ), 0, actionCollection(), "modesmall" );

    m_paDefaultIcons->setExclusiveGroup( "ViewMode" );
    m_paLargeIcons->setExclusiveGroup( "ViewMode" );
    m_paMediumIcons->setExclusiveGroup( "ViewMode" );
    m_paSmallIcons->setExclusiveGroup( "ViewMode" );

    //m_paBottomText = new KToggleAction( i18n( "Text at the &bottom" ), 0, actionCollection(), "textbottom" );
    //m_paRightText = new KToggleAction( i18n( "Text at the &right" ), 0, actionCollection(), "textright" );

    //m_paBottomText->setExclusiveGroup( "TextPos" );
    //m_paRightText->setExclusiveGroup( "TextPos" );

    new KAction( i18n( "Background Color..." ), 0, this, SLOT( slotBackgroundColor() ), actionCollection(), "bgcolor" );
    new KAction( i18n( "Background Image..." ), 0, this, SLOT( slotBackgroundImage() ), actionCollection(), "bgimage" );

    m_paIncIconSize = new KAction( i18n( "Increase Icon Size" ), "viewmag+", 0, this, SLOT( slotIncIconSize() ), actionCollection(), "incIconSize" );
    m_paDecIconSize = new KAction( i18n( "Decrease Icon Size" ), "viewmag-", 0, this, SLOT( slotDecIconSize() ), actionCollection(), "decIconSize" );

    //

    connect( m_paDefaultIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotViewDefault( bool ) ) );
    connect( m_paLargeIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotViewLarge( bool ) ) );
    connect( m_paMediumIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotViewMedium( bool ) ) );
    connect( m_paSmallIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotViewSmall( bool ) ) );

    //connect( m_paBottomText, SIGNAL( toggled( bool ) ), this, SLOT( slotTextBottom( bool ) ) );
    //connect( m_paRightText, SIGNAL( toggled( bool ) ), this, SLOT( slotTextRight( bool ) ) );

    //

    connect( m_pIconView, SIGNAL( executed( QIconViewItem * ) ),
             this, SLOT( slotReturnPressed( QIconViewItem * ) ) );
    connect( m_pIconView, SIGNAL( returnPressed( QIconViewItem * ) ),
             this, SLOT( slotReturnPressed( QIconViewItem * ) ) );

    connect( m_pIconView, SIGNAL( onItem( QIconViewItem * ) ),
             this, SLOT( slotOnItem( QIconViewItem * ) ) );

    connect( m_pIconView, SIGNAL( onViewport() ),
             this, SLOT( slotOnViewport() ) );

    connect( m_pIconView, SIGNAL( mouseButtonPressed(int, QIconViewItem*, const QPoint&)),
             this, SLOT( slotMouseButtonPressed(int, QIconViewItem*, const QPoint&)) );
    connect( m_pIconView, SIGNAL( mouseButtonClicked(int, QIconViewItem*, const QPoint&)),
             this, SLOT( slotMouseButtonClicked(int, QIconViewItem*, const QPoint&)) );

    // Extract 3 icon sizes from the icon theme. Use 16,32,48 as default.
    int i;
    m_iIconSize[0] = 0; // Default value
    m_iIconSize[1] = KIcon::SizeSmall; // 16
    m_iIconSize[2] = KIcon::SizeMedium; // 32
    m_iIconSize[3] = KIcon::SizeLarge; // 48
    KIconTheme *root = KGlobal::instance()->iconLoader()->theme();
    if (root)
    {
      QValueList<int> avSizes = root->querySizes(KIcon::Desktop);
      QValueList<int>::Iterator it;
      for (i=1, it=avSizes.begin(); (it!=avSizes.end()) && (i<4); it++, i++)
      {
        m_iIconSize[i] = *it;
        //kdDebug(1202) << "m_iIconSize[" << i << "] = " << *it << endl;
      }
    }

    // Now we may react to configuration changes
    m_bInit = false;

    m_dirLister = 0L;
    m_bLoading = false;
    m_bNeedAlign = false;
    m_bNeedEmitCompleted = false;
    m_pIconView->setResizeMode( QIconView::Adjust );

    m_eSortCriterion = NameCaseInsensitive;

    connect( m_pIconView, SIGNAL( selectionChanged() ),
             this, SLOT( slotSelectionChanged() ) );

    // Respect kcmkonq's configuration for word-wrap icon text.
    // If we want something else, we have to adapt the configuration or remove it...
    m_pIconView->setWordWrapIconText(KonqFMSettings::settings()->wordWrapText());

    // Finally, determine initial grid size again, with those parameters
    //    m_pIconView->calculateGridX();

    setViewMode( mode );
}

KonqKfmIconView::~KonqKfmIconView()
{
    delete m_mimeTypeResolver;
    kdDebug(1202) << "-KonqKfmIconView" << endl;
    delete m_dirLister;
    delete m_pProps;
    //no need for that, KParts deletes our widget already ;-)
    //    delete m_pIconView;
}

void KonqKfmIconView::slotImagePreview( bool toggle )
{
    m_pProps->setShowingImagePreview( toggle );
    if ( !toggle )
    {
        m_pIconView->stopImagePreview();
        m_pIconView->setIcons( m_pIconView->iconSize(), true /* stopImagePreview */);
    }
    else
    {
        m_pIconView->startImagePreview( true );
    }
}

void KonqKfmIconView::slotShowDot()
{
    m_pProps->setShowingDotFiles( !m_pProps->isShowingDotFiles() );
    m_dirLister->setShowingDotFiles( m_pProps->isShowingDotFiles() );
    //we don't want the non-dot files to remain where they are
    m_bNeedAlign = true;
}

void KonqKfmIconView::slotSelect()
{
    KLineEditDlg l( i18n("Select files:"), "", m_pIconView );
    if ( l.exec() )
    {
        QString pattern = l.text();
        if ( pattern.isEmpty() )
            return;

        QRegExp re( pattern, true, true );

        m_pIconView->blockSignals( true );

        QIconViewItem *it = m_pIconView->firstItem();
        while ( it )
        {
            if ( re.match( it->text() ) != -1 )
                it->setSelected( true, true );
            it = it->nextItem();
        }

        m_pIconView->blockSignals( false );

        // do this once, not for each item
        m_pIconView->slotSelectionChanged();
        slotSelectionChanged();
    }
}

void KonqKfmIconView::slotUnselect()
{
    KLineEditDlg l( i18n("Unselect files:"), "", m_pIconView );
    if ( l.exec() )
    {
        QString pattern = l.text();
        if ( pattern.isEmpty() )
            return;

        QRegExp re( pattern, true, true );

        m_pIconView->blockSignals( true );

        QIconViewItem *it = m_pIconView->firstItem();
        while ( it )
        {
            if ( re.match( it->text() ) != -1 )
                it->setSelected( false, true );
            it = it->nextItem();
        }

        m_pIconView->blockSignals( false );

        // do this once, not for each item
        m_pIconView->slotSelectionChanged();
        slotSelectionChanged();
    }
}

void KonqKfmIconView::slotSelectAll()
{
    m_pIconView->selectAll( true );
}

void KonqKfmIconView::slotUnselectAll()
{
    m_pIconView->selectAll( false );
}

void KonqKfmIconView::slotInvertSelection()
{
    m_pIconView->invertSelection( );
}

void KonqKfmIconView::slotSortByNameCaseSensitive( bool toggle )
{
    if ( !toggle )
        return;

    setupSorting( NameCaseSensitive );
}

void KonqKfmIconView::slotSortByNameCaseInsensitive( bool toggle )
{
    if ( !toggle )
        return;

    setupSorting( NameCaseInsensitive );
}

void KonqKfmIconView::slotSortBySize( bool toggle )
{
    if ( !toggle )
        return;

    setupSorting( Size );
}

void KonqKfmIconView::slotSortByType( bool toggle )
{
  if ( !toggle )
    return;

  setupSorting( Type );
}

void KonqKfmIconView::setupSorting( SortCriterion criterion )
{
    m_eSortCriterion = criterion;

    setupSortKeys();

    m_pIconView->sort( m_pIconView->sortDirection() );
}

void KonqKfmIconView::slotSortDescending()
{
    if ( m_pIconView->sortDirection() )
        m_pIconView->setSorting( true, false );
    else
        m_pIconView->setSorting( true, true );

    setupSortKeys(); // keys have to change, for directories

    m_pIconView->sort( m_pIconView->sortDirection() );
}

void KonqKfmIconView::slotSortDirsFirst()
{
    m_pIconView->setSortDirectoriesFirst( m_paSortDirsFirst->isChecked() );

    setupSortKeys();

    m_pIconView->sort( m_pIconView->sortDirection() );
}

void KonqKfmIconView::guiActivateEvent( KParts::GUIActivateEvent *event )
{
    KParts::ReadOnlyPart::guiActivateEvent( event );
    if ( event->activated() )
        m_pIconView->slotSelectionChanged();
}

void KonqKfmIconView::slotViewLarge( bool b )
{
    if ( b )
        newIconSize(m_iIconSize[3]);
}

void KonqKfmIconView::slotViewMedium( bool b )
{
    if ( b )
        newIconSize(m_iIconSize[2]);
}

void KonqKfmIconView::slotViewSmall( bool b )
{
    if ( b )
        newIconSize(m_iIconSize[1]);
}

void KonqKfmIconView::slotViewDefault( bool b)
{
    if ( b )
        newIconSize(0);
}

void KonqKfmIconView::slotIncIconSize()
{
    int s = m_pProps->iconSize();
    s = s ? s : KGlobal::iconLoader()->currentSize( KIcon::Desktop );
    int sizeIndex = 0;
    for ( int idx=1; idx < 4 ; ++idx )
        if (s == m_iIconSize[idx])
            sizeIndex = idx;
    ASSERT( sizeIndex != 0 && sizeIndex < 3 );
    newIconSize( m_iIconSize[sizeIndex + 1] );
}

void KonqKfmIconView::slotDecIconSize()
{
    int s = m_pProps->iconSize();
    s = s ? s : KGlobal::iconLoader()->currentSize( KIcon::Desktop );
    int sizeIndex = 0;
    for ( int idx=1; idx < 4 ; ++idx )
        if (s == m_iIconSize[idx])
            sizeIndex = idx;
    ASSERT( sizeIndex > 1 );
    newIconSize( m_iIconSize[sizeIndex - 1] );
}

void KonqKfmIconView::newIconSize( int size /*0=default, or 16,32,48....*/ )
{
    m_pProps->setIconSize( size );
    m_pIconView->setIcons( size );
    if ( m_pProps->isShowingImagePreview() )
        m_pIconView->startImagePreview( true );
    // Update the actions
    m_paDecIconSize->setEnabled(size > m_iIconSize[1]);
    m_paIncIconSize->setEnabled(size < m_iIconSize[3]);
    m_paDefaultIcons->setChecked( size == 0 );
    m_paLargeIcons->setChecked( size == m_iIconSize[3] );
    m_paMediumIcons->setChecked( size == m_iIconSize[2] );
    m_paSmallIcons->setChecked( size == m_iIconSize[1] );
}

bool KonqKfmIconView::closeURL()
{
    if ( m_dirLister ) m_dirLister->stop();
    m_mimeTypeResolver->m_lstPendingMimeIconItems.clear();
    m_pIconView->stopImagePreview();
    return true;
}

void KonqKfmIconView::slotReturnPressed( QIconViewItem *item )
{
    if ( !item )
        return;

    item->setSelected( false, true );
    m_pIconView->visualActivate(item);

    KonqFileItem *fileItem = (static_cast<KFileIVI*>(item))->item();
    if ( !fileItem )
        return;
    if (KonqFMSettings::settings()->alwaysNewWin() && fileItem->mode() & S_IFDIR) {
        fileItem->run();
    } else {
        KParts::URLArgs args;
        args.serviceType = fileItem->mimetype();
        args.trustedSource = true;
        emit m_extension->openURLRequest( fileItem->url(), args );
    }
}

void KonqKfmIconView::slotMouseButtonPressed(int _button, QIconViewItem* _item, const QPoint& _global)
{
    if ( _button == RightButton )
        if(_item) {
            (static_cast<KFileIVI*>(_item))->setSelected( true, true /* don't touch other items */ );
            emit m_extension->popupMenu( _global, m_pIconView->selectedFileItems() );
        }
        else
        {
            if (!m_dirLister) return;
            // Right click on viewport
            KFileItem * item = m_dirLister->rootItem();
            bool delRootItem = false;
            if ( ! item )
            {
                if ( m_bLoading )
                {
                    kdDebug(1202) << "slotViewportRightClicked : still loading and no root item -> dismissed" << endl;
                    return; // too early, '.' not yet listed
                }
                else
                {
                    // We didn't get a root item (e.g. over FTP)
                    // We have to create a dummy item. I tried using KonqOperations::statURL,
                    // but this was leading to a huge delay between the RMB and the popup. Bad.
                    // But KonqPopupMenu now takes care of stating before opening properties.
                    item = new KFileItem( S_IFDIR, (mode_t)-1, url() );
                    delRootItem = true;
                }
            }

            KFileItemList items;
            items.append( item );
            emit m_extension->popupMenu( QCursor::pos(), items );

            if ( delRootItem )
                delete item; // we just created it
        }
}

void KonqKfmIconView::slotMouseButtonClicked(int _button, QIconViewItem* _item, const QPoint& )
{
    if( _item && _button == MidButton )
        mmbClicked( static_cast<KFileIVI*>(_item)->item() );
}

void KonqKfmIconView::slotStarted( const QString & /*url*/ )
{
    m_pIconView->selectAll( false );
    if ( m_bLoading )
        emit started( m_dirLister->job() );
    // An update may come in while we are still processing icons...
    // So don't clear the list.
    //m_mimeTypeResolver->m_lstPendingMimeIconItems.clear();
}

void KonqKfmIconView::slotCanceled()
{
    if ( m_bLoading )
    {
        emit canceled( QString::null );
        m_bLoading = false;
    }
}

void KonqKfmIconView::slotCompleted()
{
    // Root item ? Store root item in konqiconviewwidget (whether 0L or not)
    m_pIconView->setRootItem( static_cast<KonqFileItem *>(m_dirLister->rootItem()) );

    if ( m_bUpdateContentsPosAfterListing )
      m_pIconView->setContentsPos( m_extension->urlArgs().xOffset, m_extension->urlArgs().yOffset );

    m_bUpdateContentsPosAfterListing = false;

    slotOnViewport();

    // slotRenderingFinished will do it
    m_bNeedEmitCompleted = true;

    if (m_pProps->isShowingImagePreview())
        m_mimeTypeResolver->start( 0 ); // We need the mimetypes asap
    else
    {
        slotRenderingFinished(); // emit completed, we don't want the wheel...
        // to keep turning while we find mimetypes in the background
        m_mimeTypeResolver->start( 10 );
    }

    m_bLoading = false;

    // Disable cut icons if any
    slotClipboardDataChanged();

}

void KonqKfmIconView::slotNewItems( const KFileItemList& entries )
{
    for (KFileItemListIterator it(entries); it.current(); ++it)
    {
        KonqFileItem * _fileitem = static_cast<KonqFileItem *>(it.current());

        //kdDebug(1202) << "KonqKfmIconView::slotNewItem(...)" << _fileitem->url().url() << endl;
        KFileIVI* item = new KFileIVI( m_pIconView, _fileitem,
                                       m_pIconView->iconSize() );
        item->setRenameEnabled( false );

        QString key;

        switch ( m_eSortCriterion )
        {
            case NameCaseSensitive: key = item->text(); break;
            case NameCaseInsensitive: key = item->text().lower(); break;
            case Size: key = makeSizeKey( item ); break;
            case Type: key = item->item()->mimetype(); break; // ### slows down listing :-(
            default: ASSERT(0);
        }

        item->setKey( key );

        m_mimeTypeResolver->m_lstPendingMimeIconItems.append( item );
        m_itemDict.insert( _fileitem, item );
    }
    KonqDirPart::newItems( entries );
}

void KonqKfmIconView::slotDeleteItem( KFileItem * _fileitem )
{
    KonqDirPart::deleteItem( _fileitem );

    //kdDebug(1202) << "KonqKfmIconView::slotDeleteItem(...)" << endl;
    // we need to find out the iconcontainer item containing the fileitem
    KFileIVI * ivi = m_itemDict[ _fileitem ];
    ASSERT(ivi);
    if (ivi)
    {
        m_pIconView->takeItem( ivi );
        m_mimeTypeResolver->m_lstPendingMimeIconItems.remove( ivi );
        m_itemDict.remove( _fileitem );
        // This doesn't delete the item, but we don't really care.
        // slotClear() will do it anyway - and it seems this avoids crashes
    }
}

void KonqKfmIconView::slotRefreshItems( const KFileItemList& entries )
{
    KFileItemListIterator rit(entries);
    for (; rit.current(); ++rit)
    {
        KFileIVI * ivi = m_itemDict[ rit.current() ];
        ASSERT(ivi);
        if (ivi)
            ivi->refreshIcon( true );
    }
    // In case we replace a big icon with a small one, need to repaint.
    m_pIconView->updateContents();
}

void KonqKfmIconView::slotClear()
{
    m_pIconView->clear();
    m_mimeTypeResolver->m_lstPendingMimeIconItems.clear();
    m_itemDict.clear();
}

void KonqKfmIconView::slotRedirection( const KURL & url )
{
    emit m_extension->setLocationBarURL( url.prettyURL() );
    m_url = url;
}

void KonqKfmIconView::slotCloseView()
{
    kdDebug() << "KonqKfmIconView::slotCloseView" << endl;
    //delete this; // we need better support for this first
}

void KonqKfmIconView::slotSelectionChanged()
{
    // Display statusbar info, and emit selectionInfo
    KFileItemList lst = m_pIconView->selectedFileItems();
    emitCounts( lst, true );
}

void KonqKfmIconView::determineIcon( KFileIVI * item )
{
  int oldSerial = item->pixmap()->serialNumber();

  (void) item->item()->determineMimeType();

  QPixmap newIcon = item->item()->pixmap( iconSize(),
                                          item->state() );

  if ( oldSerial != newIcon.serialNumber() )
      item->setPixmap( newIcon );
}

void KonqKfmIconView::mimeTypeDeterminationFinished()
{
    if ( m_pProps->isShowingImagePreview() )
        // We can do this only when the mimetypes are fully determined,
        // since we only do image preview... on images :-)
        m_pIconView->startImagePreview( false );
    else
        slotRenderingFinished();
}

void KonqKfmIconView::slotRenderingFinished()
{
    kdDebug(1202) << "KonqKfmIconView::slotRenderingFinished()" << endl;
    if ( m_bNeedEmitCompleted )
    {
        kdDebug(1202) << "KonqKfmIconView completed() after rendering" << endl;
        emit completed();
        m_bNeedEmitCompleted = false;
        m_pIconView->setCurrentItem( m_pIconView->firstItem() ); // workaround for qiconview bug, says reggie ;-)
    }
    if ( m_bNeedAlign )
    {
        m_bNeedAlign = false;
        m_pIconView->arrangeItemsInGrid();
    }
}

bool KonqKfmIconView::openURL( const KURL & url )
{
    if ( !m_dirLister )
    {
        // Create the directory lister
        m_dirLister = new KonqDirLister( true );

        QObject::connect( m_dirLister, SIGNAL( started( const QString & ) ),
                          this, SLOT( slotStarted( const QString & ) ) );
        QObject::connect( m_dirLister, SIGNAL( completed() ), this, SLOT( slotCompleted() ) );
        QObject::connect( m_dirLister, SIGNAL( canceled() ), this, SLOT( slotCanceled() ) );
        QObject::connect( m_dirLister, SIGNAL( clear() ), this, SLOT( slotClear() ) );
        QObject::connect( m_dirLister, SIGNAL( newItems( const KFileItemList& ) ),
                          this, SLOT( slotNewItems( const KFileItemList& ) ) );
        QObject::connect( m_dirLister, SIGNAL( deleteItem( KFileItem * ) ),
                          this, SLOT( slotDeleteItem( KFileItem * ) ) );
        QObject::connect( m_dirLister, SIGNAL( refreshItems( const KFileItemList& ) ),
                          this, SLOT( slotRefreshItems( const KFileItemList& ) ) );
        QObject::connect( m_dirLister, SIGNAL( redirection( const KURL & ) ),
                          this, SLOT( slotRedirection( const KURL & ) ) );
        QObject::connect( m_dirLister, SIGNAL( closeView() ),
                          this, SLOT( slotCloseView() ) );
    }

    m_bLoading = true;

    resetCount();

    // Store url in the icon view
    m_pIconView->setURL( url );

    // and in the part :-)
    m_url = url;

    // Check for new properties in the new dir
    // newProps returns true the first time, and any time something might
    // have changed.
    bool newProps = m_pProps->enterDir( url );

    m_dirLister->setNameFilter( m_nameFilter );

    // Start the directory lister !
    m_dirLister->openURL( url, m_pProps->isShowingDotFiles() );

    m_bNeedAlign = false;
    m_bUpdateContentsPosAfterListing = true;

    // Apply properties and reflect them on the actions
    // do it after starting the dir lister to avoid changing the properties
    // of the old view
    if ( newProps )
    {
      newIconSize( m_pProps->iconSize() );

      m_paDotFiles->setChecked( m_pProps->isShowingDotFiles() );
      m_paImagePreview->setChecked( m_pProps->isShowingImagePreview() );
      // Done after listing now
      //m_pIconView->setImagePreviewAllowed ( m_pProps->isShowingImagePreview() );

      // It has to be "viewport()" - this is what KonqDirPart's slots act upon,
      // and otherwise we get a color/pixmap in the square between the scrollbars.
      m_pProps->applyColors( m_pIconView->viewport() );
    }

    emit setWindowCaption( url.prettyURL() );

    return true;
}

void KonqKfmIconView::slotOnItem( QIconViewItem *item )
{
    emit setStatusBarText( ((KFileIVI *)item)->item()->getStatusBarInfo() );
}

void KonqKfmIconView::slotOnViewport()
{
    KFileItemList lst = m_pIconView->selectedFileItems();
    emitCounts( lst, false );
}

void KonqKfmIconView::setViewMode( const QString &mode )
{
    if ( mode == m_mode )
        return;
    // note: this should be moved to KonqIconViewWidget. It would make the code
    // more readable :)
    m_pIconView->setUpdatesEnabled( false );

    m_mode = mode;
    if (mode=="MultiColumnView")
    {
        m_pIconView->setArrangement(QIconView::TopToBottom);
        m_pIconView->setItemTextPos(QIconView::Right);
    }
    else
    {
        m_pIconView->setArrangement(QIconView::LeftToRight);
        m_pIconView->setItemTextPos(QIconView::Bottom);
    }

    m_pIconView->setUpdatesEnabled( true );
}

void KonqKfmIconView::setupSortKeys()
{

    switch ( m_eSortCriterion )
    {
    case NameCaseSensitive:
        for ( QIconViewItem *it = m_pIconView->firstItem(); it; it = it->nextItem() )
            it->setKey( it->text() );
        break;
    case NameCaseInsensitive:
        for ( QIconViewItem *it = m_pIconView->firstItem(); it; it = it->nextItem() )
            it->setKey( it->text().lower() );
        break;
    case Size:
        for ( QIconViewItem *it = m_pIconView->firstItem(); it; it = it->nextItem() )
            it->setKey( makeSizeKey( (KFileIVI *)it ) );
        break;
     case Type:
        for ( QIconViewItem *it = m_pIconView->firstItem(); it; it = it->nextItem() )
            it->setKey( static_cast<KFileIVI *>( it )->item()->mimetype() );
        break;
    }
}

QString KonqKfmIconView::makeSizeKey( KFileIVI *item )
{
    return QString::number( item->item()->size() ).rightJustify( 20, '0' );
}

void KonqKfmIconView::disableIcons( const KURL::List & lst )
{
    m_pIconView->disableIcons( lst );
}

#include "konq_iconview.moc"
