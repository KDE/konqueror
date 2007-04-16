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

#include <QClipboard>
#include <QMimeData>
#include <QPainter>

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
#include <kparts/componentfactory.h>

#include <kbookmark.h>
#include <kbookmarkmanager.h>
#include <kbookmarkimporter.h>

#include <kbookmarkimporter_ie.h>
#include <kbookmarkimporter_opera.h>
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
    (void) KStandardAction::print(actn, SLOT( slotPrint() ), actionCollection());

    // actions
    QAction* actnDelete = actionCollection()->addAction("delete");
    actnDelete->setIcon(KIcon("edit-delete"));
    actnDelete->setText(i18n("&Delete"));
    actnDelete->setShortcut(Qt::Key_Delete);
    connect(actnDelete, SIGNAL( triggered() ), actn, SLOT( slotDelete() ));

    QAction* actnRename = actionCollection()->addAction("rename");
    actnRename->setIcon(KIcon("text"));
    actnRename->setText(i18n("Rename"));
    actnRename->setShortcut(Qt::Key_F2);
    connect(actnRename, SIGNAL( triggered() ), actn, SLOT( slotRename() ));

    QAction* actnChangeURL = actionCollection()->addAction("changeurl");
    actnChangeURL->setIcon(KIcon("text"));
    actnChangeURL->setText(i18n("C&hange URL"));
    actnChangeURL->setShortcut(Qt::Key_F3);
    connect(actnChangeURL, SIGNAL( triggered() ), actn, SLOT( slotChangeURL() ));

    QAction* actnChangeComment = actionCollection()->addAction("changecomment");
    actnChangeComment->setIcon(KIcon("text"));
    actnChangeComment->setText(i18n("C&hange Comment"));
    actnChangeComment->setShortcut(Qt::Key_F4);
    connect(actnChangeComment, SIGNAL( triggered() ), actn, SLOT( slotChangeComment() ));

    QAction* actnChangeIcon = actionCollection()->addAction("changeicon");
    actnChangeIcon->setIcon(KIcon("icons"));
    actnChangeIcon->setText(i18n("Chan&ge Icon..."));
    connect(actnChangeIcon, SIGNAL( triggered() ), actn, SLOT( slotChangeIcon() ));

    QAction* actnUpdateFavIcon = actionCollection()->addAction("updatefavicon");
    actnUpdateFavIcon->setText(i18n("Update Favicon"));
    connect(actnUpdateFavIcon, SIGNAL( triggered() ), actn, SLOT( slotUpdateFavIcon() ));

    QAction* actnRecursiveSort = actionCollection()->addAction("recursivesort");
    actnRecursiveSort->setText(i18n("Recursive Sort"));
    connect(actnRecursiveSort, SIGNAL( triggered() ), actn, SLOT( slotRecursiveSort() ));

    QAction* actnNewFolder = actionCollection()->addAction("newfolder");
    actnNewFolder->setIcon(KIcon("folder-new"));
    actnNewFolder->setText(i18n("&New Folder..."));
    actnNewFolder->setShortcut(Qt::CTRL+Qt::Key_N);
    connect(actnNewFolder, SIGNAL( triggered() ), actn, SLOT( slotNewFolder() ));

    QAction* actnNewBookmark = actionCollection()->addAction("newbookmark");
    actnNewBookmark->setIcon(KIcon("www"));
    actnNewBookmark->setText(i18n("&New Bookmark"));
    connect(actnNewBookmark, SIGNAL( triggered() ), actn, SLOT( slotNewBookmark() ));

    QAction* actnInsertSeparator = actionCollection()->addAction("insertseparator");
    actnInsertSeparator->setText(i18n("&Insert Separator"));
    actnInsertSeparator->setShortcut(Qt::CTRL+Qt::Key_I);
    connect(actnInsertSeparator, SIGNAL( triggered() ), actn, SLOT( slotInsertSeparator() ));

    QAction* actnSort = actionCollection()->addAction("sort");
    actnSort->setText(i18n("&Sort Alphabetically"));
    connect(actnSort, SIGNAL( triggered() ), actn, SLOT( slotSort() ));

    QAction* actnSetAsToolbar = actionCollection()->addAction("setastoolbar");
    actnSetAsToolbar->setIcon(KIcon("bookmark-toolbar"));
    actnSetAsToolbar->setText(i18n("Set as T&oolbar Folder"));
    connect(actnSetAsToolbar, SIGNAL( triggered() ), actn, SLOT( slotSetAsToolbar() ));

    QAction* actnExpandAll = actionCollection()->addAction("expandall");
    actnExpandAll->setText(i18n("&Expand All Folders"));
    connect(actnExpandAll, SIGNAL( triggered() ), actn, SLOT( slotExpandAll() ));

    QAction* actnCollapseAll = actionCollection()->addAction("collapseall");
    actnCollapseAll->setText(i18n("Collapse &All Folders"));
    connect(actnCollapseAll, SIGNAL( triggered() ), actn, SLOT( slotCollapseAll() ));

    QAction* actnOpenLink = actionCollection()->addAction("openlink");
    actnOpenLink->setIcon(KIcon("document-open"));
    actnOpenLink->setText(i18n("&Open in Konqueror"));
    connect(actnOpenLink, SIGNAL( triggered() ), actn, SLOT( slotOpenLink() ));

    QAction* actnTestSelection = actionCollection()->addAction("testlink");
    actnTestSelection->setIcon(KIcon("bookmark"));
    actnTestSelection->setText(i18n("Check &Status"));
    connect(actnTestSelection, SIGNAL( triggered() ), actn, SLOT( slotTestSelection() ));

    QAction* actnTestAll = actionCollection()->addAction("testall");
    actnTestAll->setText(i18n("Check Status: &All"));
    connect(actnTestAll, SIGNAL( triggered() ), actn, SLOT( slotTestAll() ));

    QAction* actnUpdateAllFavIcons = actionCollection()->addAction("updateallfavicons");
    actnUpdateAllFavIcons->setText(i18n("Update All &Favicons"));
    connect(actnUpdateAllFavIcons, SIGNAL( triggered() ), actn, SLOT( slotUpdateAllFavIcons() ));

    QAction* actnCancelAllTests = actionCollection()->addAction("canceltests");
    actnCancelAllTests->setText(i18n("Cancel &Checks"));
    connect(actnCancelAllTests, SIGNAL( triggered() ), actn, SLOT( slotCancelAllTests() ));

    QAction* actnCancelFavIconUpdates = actionCollection()->addAction("cancelfaviconupdates");
    actnCancelFavIconUpdates->setText(i18n("Cancel &Favicon Updates"));
    connect(actnCancelFavIconUpdates, SIGNAL( triggered() ), actn, SLOT( slotCancelFavIconUpdates() ));

    QAction* actnImportNS = actionCollection()->addAction("importNS");
    actnImportNS->setIcon(KIcon("netscape"));
    actnImportNS->setText(i18n("Import &Netscape Bookmarks..."));
    connect(actnImportNS, SIGNAL( triggered() ), actn, SLOT( slotImport() ));

    QAction* actnImportOpera = actionCollection()->addAction("importOpera");
    actnImportOpera->setIcon(KIcon("opera"));
    actnImportOpera->setText(i18n("Import &Opera Bookmarks..."));
    connect(actnImportOpera, SIGNAL( triggered() ), actn, SLOT( slotImport() ));

    QAction* actnImportCrashes = actionCollection()->addAction("importCrashes");
    actnImportCrashes->setText(i18n("Import All &Crash Sessions as Bookmarks..."));
    connect(actnImportCrashes, SIGNAL( triggered() ), actn, SLOT( slotImport() ));

    QAction* actnImportGaleon = actionCollection()->addAction("importGaleon");
    actnImportGaleon->setText(i18n("Import &Galeon Bookmarks..."));
    connect(actnImportGaleon, SIGNAL( triggered() ), actn, SLOT( slotImport() ));

    QAction* actnImportKDE2 = actionCollection()->addAction("importKDE2");
    actnImportKDE2->setText(i18n("Import &KDE2/KDE3 Bookmarks..."));
    connect(actnImportKDE2, SIGNAL( triggered() ), actn, SLOT( slotImport() ));

    QAction* actnImportIE = actionCollection()->addAction("importIE");
    actnImportIE->setText(i18n("Import &IE Bookmarks..."));
    connect(actnImportIE, SIGNAL( triggered() ), actn, SLOT( slotImport() ));

    QAction* actnImportMoz = actionCollection()->addAction("importMoz");
    actnImportMoz->setIcon(KIcon("mozilla"));
    actnImportMoz->setText(i18n("Import &Mozilla Bookmarks..."));
    connect(actnImportMoz, SIGNAL( triggered() ), actn, SLOT( slotImport() ));

    QAction* actnExportNS = actionCollection()->addAction("exportNS");
    actnExportNS->setIcon(KIcon("netscape"));
    actnExportNS->setText(i18n("Export to &Netscape Bookmarks"));
    connect(actnExportNS, SIGNAL( triggered() ), actn, SLOT( slotExportNS() ));

    QAction* actnExportOpera = actionCollection()->addAction("exportOpera");
    actnExportOpera->setIcon(KIcon("opera"));
    actnExportOpera->setText(i18n("Export to &Opera Bookmarks..."));
    connect(actnExportOpera, SIGNAL( triggered() ), actn, SLOT( slotExportOpera() ));

    QAction* actnExportHTML = actionCollection()->addAction("exportHTML");
    actnExportHTML->setIcon(KIcon("html"));
    actnExportHTML->setText(i18n("Export to &HTML Bookmarks..."));
    connect(actnExportHTML, SIGNAL( triggered() ), actn, SLOT( slotExportHTML() ));

    QAction* actnExportIE = actionCollection()->addAction("exportIE");
    actnExportIE->setText(i18n("Export to &IE Bookmarks..."));
    connect(actnExportIE, SIGNAL( triggered() ), actn, SLOT( slotExportIE() ));

    QAction* actnExportMoz = actionCollection()->addAction("exportMoz");
    actnExportMoz->setIcon(KIcon("mozilla"));
    actnExportMoz->setText(i18n("Export to &Mozilla Bookmarks..."));
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
                        i18n("*.html|HTML Bookmark Listing") );
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

        if ( sa.multiSelect || (sa.singleSelect && !sa.root) )
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

    for ( QStringList::Iterator it = toEnable.begin();
            it != toEnable.end(); ++it )
    {
        //kDebug() <<" enabling action "<<(*it) << endl;
        coll->action((*it).toAscii().data())->setEnabled(true);
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
            i18n( "New folder:" ), QString(), &ok );
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
    // kDebug() << "ActionsImpl::slotImport() where sender()->name() == "
    //           << sender()->name() << endl;
    ImportCommand* import
        = ImportCommand::performImport(sender()->objectName()+6, KEBApp::self());
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

#if 0
static DCOPCString s_appId, s_objId;
#endif
static KParts::ReadOnlyPart *s_part;

void ActionsImpl::slotPrint() {
    KEBApp::self()->bkInfo()->commitChanges();
    s_part = KParts::ComponentFactory
                        ::createPartInstanceFromQuery<KParts::ReadOnlyPart>(
                                "text/html", QString());
    s_part->setProperty("pluginsEnabled", QVariant(false));
    s_part->setProperty("javaScriptEnabled", QVariant(false));
    s_part->setProperty("javaEnabled", QVariant(false));

    // doc->openStream( "text/html", KUrl() );
    // doc->writeStream( QCString( "<HTML><BODY>FOO</BODY></HTML>" ) );
    // doc->closeStream();

    HTMLExporter exporter;
    KTemporaryFile tmpf;
    tmpf.setPrefix("print_bookmarks");
    tmpf.setSuffix(".html");
    tmpf.setAutoRemove(false);
    QTextStream tstream ( &tmpf );
    tstream.setCodec("UTF-16");
    tstream << exporter.toString(CurrentMgr::self()->root(), true);
    tstream.flush();

#if 0
    s_appId = kapp->dcopClient()->appId();
    s_objId = s_part->property("dcopObjectId").toString().toLatin1();
#endif
    connect(s_part, SIGNAL(completed()), this, SLOT(slotDelayedPrint()));

    s_part->openUrl(KUrl( tmpf.fileName() ));
}

void ActionsImpl::slotDelayedPrint() {
    Q_ASSERT(s_part);
#ifdef __GNUC__
#warning Re-implement khtml print call without dcop
#endif
#if 0
    // We could just link to khtml and call s_part->view()->print(false)...
    // Or print could be made a slot in either khtmlpart or khtmlview...
    DCOPRef(s_appId, s_objId).send("print", false);
#endif
    delete s_part;
    s_part = 0;
}

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
    virtual void visit(const KBookmark &) { ; }
    virtual void visitEnter(const KBookmarkGroup &);
    virtual void visitLeave(const KBookmarkGroup &) { ; }
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
    QString newIcon = KIconDialog::getIcon(K3Icon::Small, K3Icon::FileSystem);
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
