/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 or at your option version 3 as published by
   the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "toplevel.h"
#include <QVBoxLayout>
#include "globalbookmarkmanager.h"

#include "kbookmarkmodel/model.h"

#include "bookmarkinfowidget.h"
#include "actionsimpl.h"
#include "exporters.h"
#include "settings.h"
#include "kbookmarkmodel/commands.h"
#include "kbookmarkmodel/commandhistory.h"
#include "kebsearchline.h"
#include "bookmarklistview.h"

#include <stdlib.h>

#include <QClipboard>
#include <QSplitter>
#include <QApplication>

#include <kaction.h>
#include <kactioncollection.h>
#include <ktoggleaction.h>
#include <kbookmark.h>
#include <kbookmarkmanager.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kedittoolbar.h>
#include <kfiledialog.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandardaction.h>


#include <QtDBus/QDBusConnection>
KEBApp *KEBApp::s_topLevel = 0;

KEBApp::KEBApp(
    const QString &bookmarksFile, bool readonly,
    const QString &address, bool browser, const QString &caption,
    const QString &dbusObjectName
) : KXmlGuiWindow(), m_bookmarksFilename(bookmarksFile),
    m_caption(caption),
    m_dbusObjectName(dbusObjectName), m_readOnly(readonly),m_browser(browser)
 {
    QDBusConnection::sessionBus().registerObject("/keditbookmarks", this, QDBusConnection::ExportScriptableSlots);
    Q_UNUSED(address);//FIXME sets the current item

    m_cmdHistory = new CommandHistory(this);
    m_cmdHistory->createActions(actionCollection());
    connect(m_cmdHistory, SIGNAL(notifyCommandExecuted(KBookmarkGroup)), this, SLOT(notifyCommandExecuted()));

    GlobalBookmarkManager::self()->createManager(m_bookmarksFilename, m_dbusObjectName, m_cmdHistory);

    s_topLevel = this;

    createActions();
    if (m_browser)
        createGUI();
    else
        createGUI("keditbookmarks-genui.rc");

    connect(qApp->clipboard(), SIGNAL(dataChanged()),
                               SLOT(slotClipboardDataChanged()));

    KGlobal::locale()->insertCatalog("libkonq");

    m_canPaste = false;

    mBookmarkListView = new BookmarkListView();
    mBookmarkListView->setModel( GlobalBookmarkManager::self()->model() );
    mBookmarkListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mBookmarkListView->loadColumnSetting();
    mBookmarkListView->loadFoldedState();

    KViewSearchLineWidget *searchline = new KViewSearchLineWidget(mBookmarkListView);

    mBookmarkFolderView = new BookmarkFolderView(mBookmarkListView);
    mBookmarkFolderView->expandAll();

    QWidget * rightSide = new QWidget;
    QVBoxLayout *listLayout = new QVBoxLayout(rightSide);
    listLayout->setMargin(0);
    rightSide->setLayout(listLayout);
    listLayout->addWidget(searchline);
    listLayout->addWidget(mBookmarkListView);

    m_bkinfo = new BookmarkInfoWidget(mBookmarkListView, GlobalBookmarkManager::self()->model());
    m_bkinfo->layout()->setContentsMargins(0, 0, KDialog::spacingHint(), KDialog::spacingHint());

    listLayout->addWidget(m_bkinfo);

    QSplitter *hsplitter = new QSplitter(this);
    hsplitter->setOrientation(Qt::Horizontal);
    hsplitter->addWidget(mBookmarkFolderView);
    hsplitter->addWidget(rightSide);
    hsplitter->setStretchFactor(1,1);

    setCentralWidget(hsplitter);

    slotClipboardDataChanged();
    setAutoSaveSettings();

    connect(mBookmarkListView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectionChanged()));

    connect(mBookmarkFolderView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectionChanged()));

    setCancelFavIconUpdatesEnabled(false);
    setCancelTestsEnabled(false);
    updateActions();
}

void KEBApp::expandAll()
{
    mBookmarkListView->expandAll();
}

void KEBApp::collapseAll()
{
    mBookmarkListView->collapseAll();
}

QString KEBApp::bookmarkFilename()
{
   return m_bookmarksFilename;
}

void KEBApp::reset(const QString & caption, const QString & bookmarksFileName)
{
    //FIXME check this code, probably should be model()->setRoot instead of resetModel()
    m_caption = caption;
    m_bookmarksFilename = bookmarksFileName;
    GlobalBookmarkManager::self()->createManager(m_bookmarksFilename, m_dbusObjectName, m_cmdHistory); //FIXME this is still a memory leak (iff called by slotLoad)
    GlobalBookmarkManager::self()->model()->resetModel();
    updateActions();
}

void KEBApp::startEdit( Column c )
{
    const QModelIndexList & list = mBookmarkListView->selectionModel()->selectedIndexes();
    QModelIndexList::const_iterator it, end;
    end = list.constEnd();
    for(it = list.constBegin(); it != end; ++it)
        if( (*it).column() == int(c) && (mBookmarkListView->model()->flags(*it) & Qt::ItemIsEditable) )
            return mBookmarkListView->edit( *it );
}

//FIXME clean up and remove unneeded things
SelcAbilities KEBApp::getSelectionAbilities() const
{
    SelcAbilities selctionAbilities;
    selctionAbilities.itemSelected   = false;
    selctionAbilities.group          = false;
    selctionAbilities.separator      = false;
    selctionAbilities.urlIsEmpty     = false;
    selctionAbilities.root           = false;
    selctionAbilities.multiSelect    = false;
    selctionAbilities.singleSelect   = false;
    selctionAbilities.notEmpty       = false;
    selctionAbilities.deleteEnabled  = false;

    KBookmark nbk;
    QModelIndexList sel = mBookmarkListView->selectionModel()->selectedIndexes();
    int columnCount;
    if(sel.count())
    {
        nbk = mBookmarkListView->bookmarkForIndex(sel.first());
        columnCount = mBookmarkListView->model()->columnCount();
    }
    else
    {
        sel = mBookmarkFolderView->selectionModel()->selectedIndexes();
        if(sel.count())
            nbk = mBookmarkFolderView->bookmarkForIndex(sel.first());
        columnCount = mBookmarkFolderView->model()->columnCount();
    }

    if ( sel.count() > 0)
    {
        selctionAbilities.deleteEnabled = true;
        selctionAbilities.itemSelected   = true;
        selctionAbilities.group          = nbk.isGroup();
        selctionAbilities.separator      = nbk.isSeparator();
        selctionAbilities.urlIsEmpty     = nbk.url().isEmpty();
        selctionAbilities.root           = nbk.address() == GlobalBookmarkManager::self()->root().address();
        selctionAbilities.multiSelect    = (sel.count() > columnCount);
        selctionAbilities.singleSelect   = (!selctionAbilities.multiSelect && selctionAbilities.itemSelected);
    }
    //FIXME check next line, if it actually works
    selctionAbilities.notEmpty = GlobalBookmarkManager::self()->root().first().hasParent();

/*    kDebug()
        <<"\nsa.itemSelected "<<selctionAbilities.itemSelected
        <<"\nsa.group        "<<selctionAbilities.group
        <<"\nsa.separator    "<<selctionAbilities.separator
        <<"\nsa.urlIsEmpty   "<<selctionAbilities.urlIsEmpty
        <<"\nsa.root         "<<selctionAbilities.root
        <<"\nsa.multiSelect  "<<selctionAbilities.multiSelect
        <<"\nsa.singleSelect "<<selctionAbilities.singleSelect
        <<"\nsa.deleteEnabled"<<selctionAbilities.deleteEnabled
        <<endl;
*/
    return selctionAbilities;
}

void KEBApp::setActionsEnabled(SelcAbilities sa) {
    KActionCollection * coll = actionCollection();

    QStringList toEnable;

    if (sa.multiSelect || (sa.singleSelect && !sa.root))
        toEnable << "edit_copy";

    if (sa.multiSelect || (sa.singleSelect && !sa.root && !sa.urlIsEmpty && !sa.group && !sa.separator))
            toEnable << "openlink";

    if (!m_readOnly) {
        if (sa.notEmpty)
            toEnable << "testall" << "updateallfavicons";

        if ( sa.deleteEnabled && (sa.multiSelect || (sa.singleSelect && !sa.root)) )
                toEnable << "delete" << "edit_cut";

        if( sa.singleSelect)
            if (m_canPaste)
                toEnable << "edit_paste";

        if( sa.multiSelect || (sa.singleSelect && !sa.root && !sa.urlIsEmpty && !sa.group && !sa.separator))
            toEnable << "testlink" << "updatefavicon";

        if (sa.singleSelect && !sa.root && !sa.separator) {
            toEnable << "rename" << "changeicon" << "changecomment";
            if (!sa.group)
                toEnable << "changeurl";
        }

        if (sa.singleSelect) {
            toEnable << "newfolder" << "newbookmark" << "insertseparator";
            if (sa.group)
                toEnable << "sort" << "recursivesort" << "setastoolbar";
        }
    }

    for ( QStringList::const_iterator it = toEnable.constBegin();
            it != toEnable.constEnd(); ++it )
    {
        //kDebug() <<" enabling action "<<(*it);
        coll->action(*it)->setEnabled(true);
    }
}

KBookmark KEBApp::firstSelected() const
{
    QModelIndex index;
    const QModelIndexList & list = mBookmarkListView->selectionModel()->selectedIndexes();
    if(list.count()) // selection in main listview, return bookmark for firstSelected
        return mBookmarkListView->bookmarkForIndex(*list.constBegin());

    // no selection in main listview, fall back to selection in left tree
    const QModelIndexList & list2 = mBookmarkFolderView->selectionModel()->selectedIndexes();
    return mBookmarkFolderView->bookmarkForIndex(*list2.constBegin());
}

QString KEBApp::insertAddress() const
{
    KBookmark current = firstSelected();
    return (current.isGroup())
        ? (current.address() + "/0") //FIXME internal represantation used
        : KBookmark::nextAddress(current.address());
}

bool lessAddress(const QString& first, const QString& second)
{
    QString a = first;
    QString b = second;

    if(a == b)
         return false;

    QString error("ERROR");
    if(a == error)
        return false;
    if(b == error)
        return true;

    a += '/';
    b += '/';

    uint aLast = 0;
    uint bLast = 0;
    uint aEnd = a.length();
    uint bEnd = b.length();
    // Each iteration checks one "/"-delimeted part of the address
    // "" is treated correctly
    while(true)
    {
        // Invariant: a[0 ... aLast] == b[0 ... bLast]
        if(aLast + 1 == aEnd) //The last position was the last slash
            return true; // That means a is shorter than b
        if(bLast +1 == bEnd)
            return false;

        uint aNext = a.indexOf("/", aLast + 1);
        uint bNext = b.indexOf("/", bLast + 1);

        bool okay;
        uint aNum = a.mid(aLast + 1, aNext - aLast - 1).toUInt(&okay);
        if(!okay)
            return false;
        uint bNum = b.mid(bLast + 1, bNext - bLast - 1).toUInt(&okay);
        if(!okay)
            return true;

        if(aNum != bNum)
            return aNum < bNum;

        aLast = aNext;
        bLast = bNext;
    }
}

bool lessBookmark(const KBookmark & first, const KBookmark & second) //FIXME Using internal represantation
{
    return lessAddress(first.address(), second.address());
}

KBookmark::List KEBApp::allBookmarks() const
{
    KBookmark::List bookmarks;
    selectedBookmarksExpandedHelper(GlobalBookmarkManager::self()->root(), bookmarks);
    return bookmarks;
}

KBookmark::List KEBApp::selectedBookmarks() const
{
    KBookmark::List bookmarks;
    const QModelIndexList & list = mBookmarkListView->selectionModel()->selectedIndexes();
    if (!list.isEmpty()) {
      QModelIndexList::const_iterator it, end;
      end = list.constEnd();
      for( it = list.constBegin(); it != end; ++it) {
	  if((*it).column() != 0)
	      continue;
	  KBookmark bk = mBookmarkListView->bookmarkModel()->bookmarkForIndex(*it);;
	  if(bk.address() != GlobalBookmarkManager::self()->root().address())
              bookmarks.push_back( bk );
      }
      qSort(bookmarks.begin(), bookmarks.end(), lessBookmark);
    } else {
        bookmarks.push_back(firstSelected());
    }

    return bookmarks;
}

KBookmark::List KEBApp::selectedBookmarksExpanded() const
{
    KBookmark::List bookmarks = selectedBookmarks();
    KBookmark::List result;
    KBookmark::List::const_iterator it, end;
    end = bookmarks.constEnd();
    for(it = bookmarks.constBegin(); it != end; ++it)
    {
        selectedBookmarksExpandedHelper( *it, result );
    }
    return result;
}

void KEBApp::selectedBookmarksExpandedHelper(const KBookmark& bk, KBookmark::List & bookmarks) const
{
    //FIXME in which order parents should ideally be: parent then child
    // or child and then parents
    if(bk.isGroup())
    {
        KBookmarkGroup parent = bk.toGroup();
        KBookmark child = parent.first();
        while(!child.isNull())
        {
            selectedBookmarksExpandedHelper(child, bookmarks);
            child = parent.next(child);
        }
    }
    else
    {
        bookmarks.push_back( bk );
    }
}

void KEBApp::updateStatus(const QString &url)
{
    if(m_bkinfo->bookmark().url() == url)
        m_bkinfo->updateStatus();
}

KEBApp::~KEBApp() {

    // Save again, just in case the user expanded/collapsed folders (#131127)
    GlobalBookmarkManager::self()->notifyManagers();

    s_topLevel = 0;
    delete m_cmdHistory;
    delete m_actionsImpl;
    delete mBookmarkListView;
    delete GlobalBookmarkManager::self();
}

KToggleAction* KEBApp::getToggleAction(const char *action) const {
    return static_cast<KToggleAction*>(actionCollection()->action(action));
}

void KEBApp::resetActions() {
    stateChanged("disablestuff");
    stateChanged("normal");

    if (!m_readOnly)
        stateChanged("notreadonly");
}

void  KEBApp::selectionChanged()
{
    updateActions();
}

void KEBApp::updateActions() {
    resetActions();
    setActionsEnabled(getSelectionAbilities());
}

void KEBApp::slotClipboardDataChanged() {
    // kDebug() << "KEBApp::slotClipboardDataChanged";
    if (!m_readOnly) {
        m_canPaste = KBookmark::List::canDecode(
                        QApplication::clipboard()->mimeData());
        updateActions();
    }
}

/* -------------------------- */

void KEBApp::notifyCommandExecuted() {
    // kDebug() << "KEBApp::notifyCommandExecuted()";
    updateActions();
}

/* -------------------------- */

void KEBApp::slotConfigureToolbars() {
    saveMainWindowSettings(KConfigGroup( KGlobal::config(), "MainWindow") );
    KEditToolBar dlg(actionCollection(), this);
    connect(&dlg, SIGNAL(newToolBarConfig()),
                  SLOT(slotNewToolbarConfig()));
    dlg.exec();
}

void KEBApp::slotNewToolbarConfig() {
    // called when OK or Apply is clicked
    createGUI();
    applyMainWindowSettings(KConfigGroup(KGlobal::config(), "MainWindow") );
}

/* -------------------------- */

#include "toplevel.moc"

