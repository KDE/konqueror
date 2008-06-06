// -*- indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "actionsimpl.h"

#include "toplevel.h"
#include "commands.h"
#include "importers.h"
#include "favicons.h"
#include "testlink.h"
#include "exporters.h"
#include "bookmarkinfo.h"

#include <stdlib.h>

#include <QtGui/QClipboard>
#include <QtCore/QMimeData>
#include <QtGui/QPainter>

#include <kdebug.h>
#include <kapplication.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <kedittoolbar.h>
#include <kicon.h>
#include <kicondialog.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kstandardaction.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <krun.h>
#include <kstandarddirs.h>
#include <ktemporaryfile.h>
#include <ktoggleaction.h>

#include <kparts/part.h>
#include <kservicetypetrader.h>

#include <kbookmark.h>
#include <kbookmarkmanager.h>
#include <kbookmarkimporter.h>

#include <kbookmarkimporter_ie.h>
#include <kbookmarkimporter_opera.h>
#include <kbookmarkimporter_ns.h>
#include <kbookmarkexporter.h>

ActionsImpl* ActionsImpl::s_self = 0;

// decoupled from resetActions in toplevel.cpp
// as resetActions simply uses the action groups
// specified in the ui.rc file
void KEBApp::createActions() {

    ActionsImpl *actn = ActionsImpl::self();

    // save and quit should probably not be in the toplevel???
    (void) KStandardAction::quit(
        this, SLOT( close() ), actionCollection());
    KStandardAction::keyBindings(guiFactory(), SLOT(configureShortcuts()), actionCollection());
    (void) KStandardAction::configureToolbars(
        this, SLOT( slotConfigureToolbars() ), actionCollection());

    if (m_browser) {
        (void) KStandardAction::open(
            actn, SLOT( slotLoad() ), actionCollection());
        (void) KStandardAction::saveAs(
            actn, SLOT( slotSaveAs() ), actionCollection());
    }

    (void) KStandardAction::cut(actn, SLOT( slotCut() ), actionCollection());
    (void) KStandardAction::copy(actn, SLOT( slotCopy() ), actionCollection());
    (void) KStandardAction::paste(actn, SLOT( slotPaste() ), actionCollection());

    // actions
    KAction* actnDelete = actionCollection()->addAction("delete");
    actnDelete->setIcon(KIcon("edit-delete"));
    actnDelete->setText(i18n("&Delete"));
    actnDelete->setShortcut(Qt::Key_Delete);
    connect(actnDelete, SIGNAL( triggered() ), actn, SLOT( slotDelete() ));

    KAction* actnRename = actionCollection()->addAction("rename");
    actnRename->setIcon(KIcon("edit-rename"));
    actnRename->setText(i18n("Rename"));
    actnRename->setShortcut(Qt::Key_F2);
    connect(actnRename, SIGNAL( triggered() ), actn, SLOT( slotRename() ));

    KAction* actnChangeURL = actionCollection()->addAction("changeurl");
    actnChangeURL->setIcon(KIcon("edit-rename"));
    actnChangeURL->setText(i18n("C&hange URL"));
    actnChangeURL->setShortcut(Qt::Key_F3);
    connect(actnChangeURL, SIGNAL( triggered() ), actn, SLOT( slotChangeURL() ));

    KAction* actnChangeComment = actionCollection()->addAction("changecomment");
    actnChangeComment->setIcon(KIcon("edit-rename"));
    actnChangeComment->setText(i18n("C&hange Comment"));
    actnChangeComment->setShortcut(Qt::Key_F4);
    connect(actnChangeComment, SIGNAL( triggered() ), actn, SLOT( slotChangeComment() ));

    KAction* actnChangeIcon = actionCollection()->addAction("changeicon");
    actnChangeIcon->setIcon(KIcon("preferences-desktop-icons"));
    actnChangeIcon->setText(i18n("Chan&ge Icon..."));
    connect(actnChangeIcon, SIGNAL( triggered() ), actn, SLOT( slotChangeIcon() ));

    KAction* actnUpdateFavIcon = actionCollection()->addAction("updatefavicon");
    actnUpdateFavIcon->setText(i18n("Update Favicon"));
    connect(actnUpdateFavIcon, SIGNAL( triggered() ), actn, SLOT( slotUpdateFavIcon() ));

    KAction* actnRecursiveSort = actionCollection()->addAction("recursivesort");
    actnRecursiveSort->setText(i18n("Recursive Sort"));
    connect(actnRecursiveSort, SIGNAL( triggered() ), actn, SLOT( slotRecursiveSort() ));

    KAction* actnNewFolder = actionCollection()->addAction("newfolder");
    actnNewFolder->setIcon(KIcon("folder-new"));
    actnNewFolder->setText(i18n("&New Folder..."));
    actnNewFolder->setShortcut(Qt::CTRL+Qt::Key_N);
    connect(actnNewFolder, SIGNAL( triggered() ), actn, SLOT( slotNewFolder() ));

    KAction* actnNewBookmark = actionCollection()->addAction("newbookmark");
    actnNewBookmark->setIcon(KIcon("bookmark-new"));
    actnNewBookmark->setText(i18n("&New Bookmark"));
    connect(actnNewBookmark, SIGNAL( triggered() ), actn, SLOT( slotNewBookmark() ));

    KAction* actnInsertSeparator = actionCollection()->addAction("insertseparator");
    actnInsertSeparator->setText(i18n("&Insert Separator"));
    actnInsertSeparator->setShortcut(Qt::CTRL+Qt::Key_I);
    connect(actnInsertSeparator, SIGNAL( triggered() ), actn, SLOT( slotInsertSeparator() ));

    KAction* actnSort = actionCollection()->addAction("sort");
    actnSort->setText(i18n("&Sort Alphabetically"));
    connect(actnSort, SIGNAL( triggered() ), actn, SLOT( slotSort() ));

    KAction* actnSetAsToolbar = actionCollection()->addAction("setastoolbar");
    actnSetAsToolbar->setIcon(KIcon("bookmark-toolbar"));
    actnSetAsToolbar->setText(i18n("Set as T&oolbar Folder"));
    connect(actnSetAsToolbar, SIGNAL( triggered() ), actn, SLOT( slotSetAsToolbar() ));

    KAction* actnExpandAll = actionCollection()->addAction("expandall");
    actnExpandAll->setText(i18n("&Expand All Folders"));
    connect(actnExpandAll, SIGNAL( triggered() ), actn, SLOT( slotExpandAll() ));

    KAction* actnCollapseAll = actionCollection()->addAction("collapseall");
    actnCollapseAll->setText(i18n("Collapse &All Folders"));
    connect(actnCollapseAll, SIGNAL( triggered() ), actn, SLOT( slotCollapseAll() ));

    KAction* actnOpenLink = actionCollection()->addAction("openlink");
    actnOpenLink->setIcon(KIcon("document-open"));
    actnOpenLink->setText(i18n("&Open in Konqueror"));
    connect(actnOpenLink, SIGNAL( triggered() ), actn, SLOT( slotOpenLink() ));

    KAction* actnTestSelection = actionCollection()->addAction("testlink");
    actnTestSelection->setIcon(KIcon("bookmarks"));
    actnTestSelection->setText(i18n("Check &Status"));
    connect(actnTestSelection, SIGNAL( triggered() ), actn, SLOT( slotTestSelection() ));

    KAction* actnTestAll = actionCollection()->addAction("testall");
    actnTestAll->setText(i18n("Check Status: &All"));
    connect(actnTestAll, SIGNAL( triggered() ), actn, SLOT( slotTestAll() ));

    KAction* actnUpdateAllFavIcons = actionCollection()->addAction("updateallfavicons");
    actnUpdateAllFavIcons->setText(i18n("Update All &Favicons"));
    connect(actnUpdateAllFavIcons, SIGNAL( triggered() ), actn, SLOT( slotUpdateAllFavIcons() ));

    KAction* actnCancelAllTests = actionCollection()->addAction("canceltests");
    actnCancelAllTests->setText(i18n("Cancel &Checks"));
    connect(actnCancelAllTests, SIGNAL( triggered() ), actn, SLOT( slotCancelAllTests() ));

    KAction* actnCancelFavIconUpdates = actionCollection()->addAction("cancelfaviconupdates");
    actnCancelFavIconUpdates->setText(i18n("Cancel &Favicon Updates"));
    connect(actnCancelFavIconUpdates, SIGNAL( triggered() ), actn, SLOT( slotCancelFavIconUpdates() ));

    KAction* actnImportNS = actionCollection()->addAction("importNS");
    actnImportNS->setObjectName("NS");
    actnImportNS->setIcon(KIcon("netscape"));
    actnImportNS->setText(i18n("Import &Netscape Bookmarks..."));
    connect(actnImportNS, SIGNAL( triggered() ), actn, SLOT( slotImport() ));

    KAction* actnImportOpera = actionCollection()->addAction("importOpera");
    actnImportOpera->setObjectName("Opera");
    actnImportOpera->setIcon(KIcon("opera"));
    actnImportOpera->setText(i18n("Import &Opera Bookmarks..."));
    connect(actnImportOpera, SIGNAL( triggered() ), actn, SLOT( slotImport() ));

    KAction* actnImportCrashes = actionCollection()->addAction("importCrashes");
    actnImportCrashes->setObjectName("Crashes");
    actnImportCrashes->setText(i18n("Import All &Crash Sessions as Bookmarks..."));
    connect(actnImportCrashes, SIGNAL( triggered() ), actn, SLOT( slotImport() ));

    KAction* actnImportGaleon = actionCollection()->addAction("importGaleon");
    actnImportGaleon->setObjectName("Galeon");
    actnImportGaleon->setText(i18n("Import &Galeon Bookmarks..."));
    connect(actnImportGaleon, SIGNAL( triggered() ), actn, SLOT( slotImport() ));

    KAction* actnImportKDE2 = actionCollection()->addAction("importKDE2");
    actnImportKDE2->setObjectName("KDE2");
    actnImportKDE2->setIcon(KIcon("kde"));
    actnImportKDE2->setText(i18n("Import &KDE 2 or KDE 3 Bookmarks..."));

    connect(actnImportKDE2, SIGNAL( triggered() ), actn, SLOT( slotImport() ));

    KAction* actnImportIE = actionCollection()->addAction("importIE");
    actnImportIE->setObjectName("IE");
    actnImportIE->setText(i18n("Import &Internet Explorer Bookmarks..."));
    connect(actnImportIE, SIGNAL( triggered() ), actn, SLOT( slotImport() ));

    KAction* actnImportMoz = actionCollection()->addAction("importMoz");
    actnImportMoz->setObjectName("Moz");
    actnImportMoz->setIcon(KIcon("mozilla"));
    actnImportMoz->setText(i18n("Import &Mozilla Bookmarks..."));
    connect(actnImportMoz, SIGNAL( triggered() ), actn, SLOT( slotImport() ));

    KAction* actnExportNS = actionCollection()->addAction("exportNS");
    actnExportNS->setIcon(KIcon("netscape"));
    actnExportNS->setText(i18n("Export &Netscape Bookmarks"));
    connect(actnExportNS, SIGNAL( triggered() ), actn, SLOT( slotExportNS() ));

    KAction* actnExportOpera = actionCollection()->addAction("exportOpera");
    actnExportOpera->setIcon(KIcon("opera"));
    actnExportOpera->setText(i18n("Export &Opera Bookmarks..."));
    connect(actnExportOpera, SIGNAL( triggered() ), actn, SLOT( slotExportOpera() ));

    KAction* actnExportHTML = actionCollection()->addAction("exportHTML");
    actnExportHTML->setIcon(KIcon("text-html"));
    actnExportHTML->setText(i18n("Export &HTML Bookmarks..."));
    connect(actnExportHTML, SIGNAL( triggered() ), actn, SLOT( slotExportHTML() ));

    KAction* actnExportIE = actionCollection()->addAction("exportIE");
    actnExportIE->setText(i18n("Export &Internet Explorer Bookmarks..."));
    connect(actnExportIE, SIGNAL( triggered() ), actn, SLOT( slotExportIE() ));

    KAction* actnExportMoz = actionCollection()->addAction("exportMoz");
    actnExportMoz->setIcon(KIcon("mozilla"));
    actnExportMoz->setText(i18n("Export &Mozilla Bookmarks..."));
    connect(actnExportMoz, SIGNAL( triggered() ), actn, SLOT( slotExportMoz() ));
}

void ActionsImpl::slotLoad()
{
    QString bookmarksFile
        = KFileDialog::getOpenFileName(QString(), "*.xml", KEBApp::self());
    if (bookmarksFile.isNull())
        return;
    KEBApp::self()->reset(QString(), bookmarksFile);
}

void ActionsImpl::slotSaveAs() {
    KEBApp::self()->bkInfo()->commitChanges();
    QString saveFilename
        = KFileDialog::getSaveFileName(QString(), "*.xml", KEBApp::self());
    if (!saveFilename.isEmpty())
        CurrentMgr::self()->saveAs(saveFilename);
}

void CurrentMgr::doExport(ExportType type, const QString & _path) {
    //it can be null when we use command line to export => there is not interface
    if ( KEBApp::self() && KEBApp::self()->bkInfo() )
        KEBApp::self()->bkInfo()->commitChanges();
    QString path(_path);
    // TODO - add a factory and make all this use the base class
    if (type == OperaExport) {
        if (path.isNull())
            path = KOperaBookmarkImporterImpl().findDefaultLocation(true);
        KOperaBookmarkExporterImpl exporter(mgr(), path);
        exporter.write(mgr()->root());
        return;

    } else if (type == HTMLExport) {
        if (path.isNull())
            path = KFileDialog::getSaveFileName(
                        QDir::homePath(),
                        i18n("*.html|HTML Bookmark Listing"),
                        KEBApp::self() );
        HTMLExporter exporter;
        exporter.write(mgr()->root(), path);
        return;

    } else if (type == IEExport) {
        if (path.isNull())
            path = KIEBookmarkImporterImpl().findDefaultLocation(true);
        KIEBookmarkExporterImpl exporter(mgr(), path);
        exporter.write(mgr()->root());
        return;
    }

    bool moz = (type == MozillaExport);

    if (path.isNull()) {
        if (moz) {
            KMozillaBookmarkImporterImpl importer;
            path = importer.findDefaultLocation(true);
        }
        else {
            KNSBookmarkImporterImpl importer;
            path = importer.findDefaultLocation(true);
        }
    }

    if (!path.isEmpty()) {
        KNSBookmarkExporterImpl exporter(mgr(), path);
        exporter.write(mgr()->root());
    }
}

void KEBApp::setCancelFavIconUpdatesEnabled(bool enabled) {
    actionCollection()->action("cancelfaviconupdates")->setEnabled(enabled);
}

void KEBApp::setCancelTestsEnabled(bool enabled) {
    actionCollection()->action("canceltests")->setEnabled(enabled);
}

void ActionsImpl::slotCut() {
    KEBApp::self()->bkInfo()->commitChanges();
    slotCopy();
    DeleteManyCommand *mcmd = new DeleteManyCommand( i18n("Cut Items"), KEBApp::self()->selectedBookmarks() );
    CmdHistory::self()->addCommand(mcmd);

}

void ActionsImpl::slotCopy()
{
    KEBApp::self()->bkInfo()->commitChanges();
    // this is not a command, because it can't be undone
    KBookmark::List bookmarks = KEBApp::self()->selectedBookmarksExpanded();
    QMimeData *mimeData = new QMimeData;
    bookmarks.populateMimeData(mimeData);
    QApplication::clipboard()->setMimeData( mimeData );
}

void ActionsImpl::slotPaste() {
    KEBApp::self()->bkInfo()->commitChanges();

    QString addr;
    KBookmark bk = KEBApp::self()->firstSelected();
    if(bk.isGroup())
        addr = bk.address() + "/0"; //FIXME internal
    else
        addr = bk.address();

    KEBMacroCommand *mcmd = CmdGen::insertMimeSource( i18n("Paste"), QApplication::clipboard()->mimeData(), addr);
    CmdHistory::self()->didCommand(mcmd);
}

/* -------------------------------------- */

void ActionsImpl::slotNewFolder()
{
    KEBApp::self()->bkInfo()->commitChanges();
    bool ok;
    QString str = KInputDialog::getText( i18n( "Create New Bookmark Folder" ),
            i18n( "New folder:" ), QString(), &ok, KEBApp::self() );
    if (!ok)
        return;

    CreateCommand *cmd = new CreateCommand(
                                KEBApp::self()->insertAddress(),
                                str, "bookmark_folder", /*open*/ true);
    CmdHistory::self()->addCommand(cmd);
}

void ActionsImpl::slotNewBookmark()
{
    KEBApp::self()->bkInfo()->commitChanges();
    // TODO - make a setCurrentItem(Command *) which uses finaladdress interface
    CreateCommand * cmd = new CreateCommand(
                                KEBApp::self()->insertAddress(),
                                QString(), "www", KUrl("http://"));
    CmdHistory::self()->addCommand(cmd);
}

void ActionsImpl::slotInsertSeparator()
{
    KEBApp::self()->bkInfo()->commitChanges();
    CreateCommand * cmd = new CreateCommand(KEBApp::self()->insertAddress());
    CmdHistory::self()->addCommand(cmd);
}

void ActionsImpl::slotImport() {
    KEBApp::self()->bkInfo()->commitChanges();
    qDebug() << "ActionsImpl::slotImport() where sender()->name() == "
               << sender()->objectName() << endl;
    ImportCommand* import
        = ImportCommand::performImport(sender()->objectName(), KEBApp::self());
    if (!import)
        return;
    CmdHistory::self()->addCommand(import);
    //FIXME select import->groupAddress
}

// TODO - this is getting ugly and repetitive. cleanup!

void ActionsImpl::slotExportOpera() {
    KEBApp::self()->bkInfo()->commitChanges();
    CurrentMgr::self()->doExport(CurrentMgr::OperaExport); }
void ActionsImpl::slotExportHTML() {
    KEBApp::self()->bkInfo()->commitChanges();
    CurrentMgr::self()->doExport(CurrentMgr::HTMLExport); }
void ActionsImpl::slotExportIE() {
    KEBApp::self()->bkInfo()->commitChanges();
    CurrentMgr::self()->doExport(CurrentMgr::IEExport); }
void ActionsImpl::slotExportNS() {
    KEBApp::self()->bkInfo()->commitChanges();
    CurrentMgr::self()->doExport(CurrentMgr::NetscapeExport); }
void ActionsImpl::slotExportMoz() {
    KEBApp::self()->bkInfo()->commitChanges();
    CurrentMgr::self()->doExport(CurrentMgr::MozillaExport); }

/* -------------------------------------- */

void ActionsImpl::slotCancelFavIconUpdates() {
    FavIconsItrHolder::self()->cancelAllItrs();
}

void ActionsImpl::slotCancelAllTests() {
    TestLinkItrHolder::self()->cancelAllItrs();
}

void ActionsImpl::slotTestAll() {
    TestLinkItrHolder::self()->insertItr(
            new TestLinkItr(KEBApp::self()->allBookmarks()));
}

void ActionsImpl::slotUpdateAllFavIcons() {
    FavIconsItrHolder::self()->insertItr(
            new FavIconsItr(KEBApp::self()->allBookmarks()));
}

ActionsImpl::~ActionsImpl() {
    delete FavIconsItrHolder::self();
    delete TestLinkItrHolder::self();
}

/* -------------------------------------- */

void ActionsImpl::slotTestSelection() {
    KEBApp::self()->bkInfo()->commitChanges();
    TestLinkItrHolder::self()->insertItr(new TestLinkItr(KEBApp::self()->selectedBookmarksExpanded()));
}

void ActionsImpl::slotUpdateFavIcon() {
    KEBApp::self()->bkInfo()->commitChanges();
    FavIconsItrHolder::self()->insertItr(new FavIconsItr(KEBApp::self()->selectedBookmarksExpanded()));
}

/* -------------------------------------- */

class KBookmarkGroupList : private KBookmarkGroupTraverser {
public:
    KBookmarkGroupList(KBookmarkManager *);
    QList<KBookmark> getList(const KBookmarkGroup &);
private:
    virtual void visit(const KBookmark &) {}
    virtual void visitEnter(const KBookmarkGroup &);
    virtual void visitLeave(const KBookmarkGroup &) {}
private:
    KBookmarkManager *m_manager;
    QList<KBookmark> m_list;
};

KBookmarkGroupList::KBookmarkGroupList( KBookmarkManager *manager ) {
    m_manager = manager;
}

QList<KBookmark> KBookmarkGroupList::getList( const KBookmarkGroup &grp ) {
    traverse(grp);
    return m_list;
}

void KBookmarkGroupList::visitEnter(const KBookmarkGroup &grp) {
    m_list << grp;
}

void ActionsImpl::slotRecursiveSort() {
    KEBApp::self()->bkInfo()->commitChanges();
    KBookmark bk = KEBApp::self()->firstSelected();
    Q_ASSERT(bk.isGroup());
    KEBMacroCommand *mcmd = new KEBMacroCommand(i18n("Recursive Sort"));
    KBookmarkGroupList lister(CurrentMgr::self()->mgr());
    QList<KBookmark> bookmarks = lister.getList(bk.toGroup());
    bookmarks << bk.toGroup();
    for (QList<KBookmark>::ConstIterator it = bookmarks.begin(); it != bookmarks.end(); ++it) {
        SortCommand *cmd = new SortCommand("", (*it).address());
        cmd->execute();
        mcmd->addCommand(cmd);
    }
    CmdHistory::self()->didCommand(mcmd);
}

void ActionsImpl::slotSort() {
    KEBApp::self()->bkInfo()->commitChanges();
    KBookmark bk = KEBApp::self()->firstSelected();
    Q_ASSERT(bk.isGroup());
    SortCommand *cmd = new SortCommand(i18n("Sort Alphabetically"), bk.address());
    CmdHistory::self()->addCommand(cmd);
}

/* -------------------------------------- */

void ActionsImpl::slotDelete() {
    KEBApp::self()->bkInfo()->commitChanges();
    DeleteManyCommand *mcmd = new DeleteManyCommand(i18n("Delete Items"), KEBApp::self()->selectedBookmarks());
    CmdHistory::self()->addCommand(mcmd);
}

void ActionsImpl::slotOpenLink()
{
    KEBApp::self()->bkInfo()->commitChanges();
    QList<KBookmark> bookmarks = KEBApp::self()->selectedBookmarksExpanded();
    QList<KBookmark>::const_iterator it, end;
    end = bookmarks.constEnd();
    for (it = bookmarks.constBegin(); it != end; ++it) {
        if ((*it).isGroup() || (*it).isSeparator())
            continue;
        (void)new KRun((*it).url(), KEBApp::self());
    }
}

/* -------------------------------------- */

void ActionsImpl::slotRename() {
    KEBApp::self()->bkInfo()->commitChanges();
    KEBApp::self()->startEdit( KEBApp::NameColumn );
}

void ActionsImpl::slotChangeURL() {
    KEBApp::self()->bkInfo()->commitChanges();
    KEBApp::self()->startEdit( KEBApp::UrlColumn );
}

void ActionsImpl::slotChangeComment() {
    KEBApp::self()->bkInfo()->commitChanges();
    KEBApp::self()->startEdit( KEBApp::CommentColumn );
}

void ActionsImpl::slotSetAsToolbar() {
    KEBApp::self()->bkInfo()->commitChanges();
    KBookmark bk = KEBApp::self()->firstSelected();
    Q_ASSERT(bk.isGroup());
    KEBMacroCommand *mcmd = CmdGen::setAsToolbar(bk);
    CmdHistory::self()->addCommand(mcmd);
}

void ActionsImpl::slotChangeIcon() {
    KEBApp::self()->bkInfo()->commitChanges();
    KBookmark bk = KEBApp::self()->firstSelected();
    QString newIcon = KIconDialog::getIcon(KIconLoader::Small, KIconLoader::FileSystem, false, 0, false, KEBApp::self());
    if (newIcon.isEmpty())
        return;
    EditCommand *cmd = new EditCommand(bk.address(), -1, newIcon);

    CmdHistory::self()->addCommand(cmd);
}

void ActionsImpl::slotExpandAll()
{
    KEBApp::self()->expandAll();
}

void ActionsImpl::slotCollapseAll()
{
    KEBApp::self()->collapseAll();
}

#include "actionsimpl.moc"
