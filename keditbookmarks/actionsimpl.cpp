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
#include "globalbookmarkmanager.h"

#include "toplevel.h" // for KEBApp
#include "kbookmarkmodel/model.h"
#include "kbookmarkmodel/commands.h"
#include "kbookmarkmodel/commandhistory.h"
#include "importers.h"
#include "favicons.h"
#include "testlink.h"
#include "exporters.h"
#include "bookmarkinfowidget.h"

#include <stdlib.h>

#include <QClipboard>
#include <QtCore/QMimeData>
#include <QApplication>

#include <kdebug.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <kicon.h>
#include <kicondialog.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kstandardaction.h>
#include <kfiledialog.h>
#include <krun.h>
#include <kstandarddirs.h>

#include <kbookmark.h>
#include <kbookmarkmanager.h>
#include <kbookmarkimporter.h>

#include <kbookmarkimporter_ie.h>
#include <kbookmarkimporter_opera.h>
#include <kbookmarkimporter_ns.h>
#include <kbookmarkexporter.h>

// decoupled from resetActions in toplevel.cpp
// as resetActions simply uses the action groups
// specified in the ui.rc file
void KEBApp::createActions() {

    m_actionsImpl = new ActionsImpl(this, GlobalBookmarkManager::self()->model());

    connect(m_actionsImpl->testLinkHolder(), SIGNAL(setCancelEnabled(bool)),
            this, SLOT(setCancelTestsEnabled(bool)));
    connect(m_actionsImpl->favIconHolder(), SIGNAL(setCancelEnabled(bool)),
            this, SLOT(setCancelFavIconUpdatesEnabled(bool)));

    // save and quit should probably not be in the toplevel???
    (void) KStandardAction::quit(
        this, SLOT(close()), actionCollection());
    KStandardAction::keyBindings(guiFactory(), SLOT(configureShortcuts()), actionCollection());
    (void) KStandardAction::configureToolbars(
        this, SLOT(slotConfigureToolbars()), actionCollection());

    if (m_browser) {
        (void) KStandardAction::open(
            m_actionsImpl, SLOT(slotLoad()), actionCollection());
        (void) KStandardAction::saveAs(
            m_actionsImpl, SLOT(slotSaveAs()), actionCollection());
    }

    (void) KStandardAction::cut(m_actionsImpl, SLOT(slotCut()), actionCollection());
    (void) KStandardAction::copy(m_actionsImpl, SLOT(slotCopy()), actionCollection());
    (void) KStandardAction::paste(m_actionsImpl, SLOT(slotPaste()), actionCollection());

    // actions
    KAction* m_actionsImplDelete = actionCollection()->addAction("delete");
    m_actionsImplDelete->setIcon(KIcon("edit-delete"));
    m_actionsImplDelete->setText(i18n("&Delete"));
    m_actionsImplDelete->setShortcut(Qt::Key_Delete);
    connect(m_actionsImplDelete, SIGNAL(triggered()), m_actionsImpl, SLOT(slotDelete()));

    KAction* m_actionsImplRename = actionCollection()->addAction("rename");
    m_actionsImplRename->setIcon(KIcon("edit-rename"));
    m_actionsImplRename->setText(i18n("Rename"));
    m_actionsImplRename->setShortcut(Qt::Key_F2);
    connect(m_actionsImplRename, SIGNAL(triggered()), m_actionsImpl, SLOT(slotRename()));

    KAction* m_actionsImplChangeURL = actionCollection()->addAction("changeurl");
    m_actionsImplChangeURL->setIcon(KIcon("edit-rename"));
    m_actionsImplChangeURL->setText(i18n("C&hange Location"));
    m_actionsImplChangeURL->setShortcut(Qt::Key_F3);
    connect(m_actionsImplChangeURL, SIGNAL(triggered()), m_actionsImpl, SLOT(slotChangeURL()));

    KAction* m_actionsImplChangeComment = actionCollection()->addAction("changecomment");
    m_actionsImplChangeComment->setIcon(KIcon("edit-rename"));
    m_actionsImplChangeComment->setText(i18n("C&hange Comment"));
    m_actionsImplChangeComment->setShortcut(Qt::Key_F4);
    connect(m_actionsImplChangeComment, SIGNAL(triggered()), m_actionsImpl, SLOT(slotChangeComment()));

    KAction* m_actionsImplChangeIcon = actionCollection()->addAction("changeicon");
    m_actionsImplChangeIcon->setIcon(KIcon("preferences-desktop-icons"));
    m_actionsImplChangeIcon->setText(i18n("Chan&ge Icon..."));
    connect(m_actionsImplChangeIcon, SIGNAL(triggered()), m_actionsImpl, SLOT(slotChangeIcon()));

    KAction* m_actionsImplUpdateFavIcon = actionCollection()->addAction("updatefavicon");
    m_actionsImplUpdateFavIcon->setText(i18n("Update Favicon"));
    connect(m_actionsImplUpdateFavIcon, SIGNAL(triggered()), m_actionsImpl, SLOT(slotUpdateFavIcon()));

    KAction* m_actionsImplRecursiveSort = actionCollection()->addAction("recursivesort");
    m_actionsImplRecursiveSort->setText(i18n("Recursive Sort"));
    connect(m_actionsImplRecursiveSort, SIGNAL(triggered()), m_actionsImpl, SLOT(slotRecursiveSort()));

    KAction* m_actionsImplNewFolder = actionCollection()->addAction("newfolder");
    m_actionsImplNewFolder->setIcon(KIcon("folder-new"));
    m_actionsImplNewFolder->setText(i18n("&New Folder..."));
    m_actionsImplNewFolder->setShortcut(Qt::CTRL+Qt::Key_N);
    connect(m_actionsImplNewFolder, SIGNAL(triggered()), m_actionsImpl, SLOT(slotNewFolder()));

    KAction* m_actionsImplNewBookmark = actionCollection()->addAction("newbookmark");
    m_actionsImplNewBookmark->setIcon(KIcon("bookmark-new"));
    m_actionsImplNewBookmark->setText(i18n("&New Bookmark"));
    connect(m_actionsImplNewBookmark, SIGNAL(triggered()), m_actionsImpl, SLOT(slotNewBookmark()));

    KAction* m_actionsImplInsertSeparator = actionCollection()->addAction("insertseparator");
    m_actionsImplInsertSeparator->setText(i18n("&Insert Separator"));
    m_actionsImplInsertSeparator->setShortcut(Qt::CTRL+Qt::Key_I);
    connect(m_actionsImplInsertSeparator, SIGNAL(triggered()), m_actionsImpl, SLOT(slotInsertSeparator()));

    KAction* m_actionsImplSort = actionCollection()->addAction("sort");
    m_actionsImplSort->setText(i18n("&Sort Alphabetically"));
    connect(m_actionsImplSort, SIGNAL(triggered()), m_actionsImpl, SLOT(slotSort()));

    KAction* m_actionsImplSetAsToolbar = actionCollection()->addAction("setastoolbar");
    m_actionsImplSetAsToolbar->setIcon(KIcon("bookmark-toolbar"));
    m_actionsImplSetAsToolbar->setText(i18n("Set as T&oolbar Folder"));
    connect(m_actionsImplSetAsToolbar, SIGNAL(triggered()), m_actionsImpl, SLOT(slotSetAsToolbar()));

    KAction* m_actionsImplExpandAll = actionCollection()->addAction("expandall");
    m_actionsImplExpandAll->setText(i18n("&Expand All Folders"));
    connect(m_actionsImplExpandAll, SIGNAL(triggered()), m_actionsImpl, SLOT(slotExpandAll()));

    KAction* m_actionsImplCollapseAll = actionCollection()->addAction("collapseall");
    m_actionsImplCollapseAll->setText(i18n("Collapse &All Folders"));
    connect(m_actionsImplCollapseAll, SIGNAL(triggered()), m_actionsImpl, SLOT(slotCollapseAll()));

    KAction* m_actionsImplOpenLink = actionCollection()->addAction("openlink");
    m_actionsImplOpenLink->setIcon(KIcon("document-open"));
    m_actionsImplOpenLink->setText(i18n("&Open in Konqueror"));
    connect(m_actionsImplOpenLink, SIGNAL(triggered()), m_actionsImpl, SLOT(slotOpenLink()));

    KAction* m_actionsImplTestSelection = actionCollection()->addAction("testlink");
    m_actionsImplTestSelection->setIcon(KIcon("bookmarks"));
    m_actionsImplTestSelection->setText(i18n("Check &Status"));
    connect(m_actionsImplTestSelection, SIGNAL(triggered()), m_actionsImpl, SLOT(slotTestSelection()));

    KAction* m_actionsImplTestAll = actionCollection()->addAction("testall");
    m_actionsImplTestAll->setText(i18n("Check Status: &All"));
    connect(m_actionsImplTestAll, SIGNAL(triggered()), m_actionsImpl, SLOT(slotTestAll()));

    KAction* m_actionsImplUpdateAllFavIcons = actionCollection()->addAction("updateallfavicons");
    m_actionsImplUpdateAllFavIcons->setText(i18n("Update All &Favicons"));
    connect(m_actionsImplUpdateAllFavIcons, SIGNAL(triggered()), m_actionsImpl, SLOT(slotUpdateAllFavIcons()));

    KAction* m_actionsImplCancelAllTests = actionCollection()->addAction("canceltests");
    m_actionsImplCancelAllTests->setText(i18n("Cancel &Checks"));
    connect(m_actionsImplCancelAllTests, SIGNAL(triggered()), m_actionsImpl, SLOT(slotCancelAllTests()));

    KAction* m_actionsImplCancelFavIconUpdates = actionCollection()->addAction("cancelfaviconupdates");
    m_actionsImplCancelFavIconUpdates->setText(i18n("Cancel &Favicon Updates"));
    connect(m_actionsImplCancelFavIconUpdates, SIGNAL(triggered()), m_actionsImpl, SLOT(slotCancelFavIconUpdates()));

    KAction* m_actionsImplImportNS = actionCollection()->addAction("importNS");
    m_actionsImplImportNS->setObjectName( QLatin1String("NS" ));
    m_actionsImplImportNS->setIcon(KIcon("netscape"));
    m_actionsImplImportNS->setText(i18n("Import &Netscape Bookmarks..."));
    connect(m_actionsImplImportNS, SIGNAL(triggered()), m_actionsImpl, SLOT(slotImport()));

    KAction* m_actionsImplImportOpera = actionCollection()->addAction("importOpera");
    m_actionsImplImportOpera->setObjectName( QLatin1String("Opera" ));
    m_actionsImplImportOpera->setIcon(KIcon("opera"));
    m_actionsImplImportOpera->setText(i18n("Import &Opera Bookmarks..."));
    connect(m_actionsImplImportOpera, SIGNAL(triggered()), m_actionsImpl, SLOT(slotImport()));
/*
    KAction* m_actionsImplImportCrashes = actionCollection()->addAction("importCrashes");
    m_actionsImplImportCrashes->setObjectName( QLatin1String("Crashes" ));
    m_actionsImplImportCrashes->setText(i18n("Import All &Crash Sessions as Bookmarks..."));
    connect(m_actionsImplImportCrashes, SIGNAL(triggered()), m_actionsImpl, SLOT(slotImport()));
*/
    KAction* m_actionsImplImportGaleon = actionCollection()->addAction("importGaleon");
    m_actionsImplImportGaleon->setObjectName( QLatin1String("Galeon" ));
    m_actionsImplImportGaleon->setText(i18n("Import &Galeon Bookmarks..."));
    connect(m_actionsImplImportGaleon, SIGNAL(triggered()), m_actionsImpl, SLOT(slotImport()));

    KAction* m_actionsImplImportKDE2 = actionCollection()->addAction("importKDE2");
    m_actionsImplImportKDE2->setObjectName( QLatin1String("KDE2" ));
    m_actionsImplImportKDE2->setIcon(KIcon("kde"));
    m_actionsImplImportKDE2->setText(i18n("Import &KDE 2 or KDE 3 Bookmarks..."));

    connect(m_actionsImplImportKDE2, SIGNAL(triggered()), m_actionsImpl, SLOT(slotImport()));

    KAction* m_actionsImplImportIE = actionCollection()->addAction("importIE");
    m_actionsImplImportIE->setObjectName( QLatin1String("IE" ));
    m_actionsImplImportIE->setText(i18n("Import &Internet Explorer Bookmarks..."));
    connect(m_actionsImplImportIE, SIGNAL(triggered()), m_actionsImpl, SLOT(slotImport()));

    KAction* m_actionsImplImportMoz = actionCollection()->addAction("importMoz");
    m_actionsImplImportMoz->setObjectName( QLatin1String("Moz" ));
    m_actionsImplImportMoz->setIcon(KIcon("mozilla"));
    m_actionsImplImportMoz->setText(i18n("Import &Mozilla Bookmarks..."));
    connect(m_actionsImplImportMoz, SIGNAL(triggered()), m_actionsImpl, SLOT(slotImport()));

    KAction* m_actionsImplExportNS = actionCollection()->addAction("exportNS");
    m_actionsImplExportNS->setIcon(KIcon("netscape"));
    m_actionsImplExportNS->setText(i18n("Export &Netscape Bookmarks"));
    connect(m_actionsImplExportNS, SIGNAL(triggered()), m_actionsImpl, SLOT(slotExportNS()));

    KAction* m_actionsImplExportOpera = actionCollection()->addAction("exportOpera");
    m_actionsImplExportOpera->setIcon(KIcon("opera"));
    m_actionsImplExportOpera->setText(i18n("Export &Opera Bookmarks..."));
    connect(m_actionsImplExportOpera, SIGNAL(triggered()), m_actionsImpl, SLOT(slotExportOpera()));

    KAction* m_actionsImplExportHTML = actionCollection()->addAction("exportHTML");
    m_actionsImplExportHTML->setIcon(KIcon("text-html"));
    m_actionsImplExportHTML->setText(i18n("Export &HTML Bookmarks..."));
    connect(m_actionsImplExportHTML, SIGNAL(triggered()), m_actionsImpl, SLOT(slotExportHTML()));

    KAction* m_actionsImplExportIE = actionCollection()->addAction("exportIE");
    m_actionsImplExportIE->setText(i18n("Export &Internet Explorer Bookmarks..."));
    connect(m_actionsImplExportIE, SIGNAL(triggered()), m_actionsImpl, SLOT(slotExportIE()));

    KAction* m_actionsImplExportMoz = actionCollection()->addAction("exportMoz");
    m_actionsImplExportMoz->setIcon(KIcon("mozilla"));
    m_actionsImplExportMoz->setText(i18n("Export &Mozilla Bookmarks..."));
    connect(m_actionsImplExportMoz, SIGNAL(triggered()), m_actionsImpl, SLOT(slotExportMoz()));
}

ActionsImpl::ActionsImpl(QObject* parent, KBookmarkModel* model)
    : QObject(parent), m_model(model),
      m_testLinkHolder(new TestLinkItrHolder(this, model)),
      m_favIconHolder(new FavIconsItrHolder(this, model))
{
    Q_ASSERT(m_model);
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
        GlobalBookmarkManager::self()->saveAs(saveFilename);
}

void GlobalBookmarkManager::doExport(ExportType type, const QString & _path) {
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
    DeleteManyCommand *mcmd = new DeleteManyCommand(m_model, i18nc("(qtundo-format)", "Cut Items"), KEBApp::self()->selectedBookmarks() );
    commandHistory()->addCommand(mcmd);

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

    QUndoCommand *mcmd = CmdGen::insertMimeSource( m_model, i18nc("(qtundo-format)", "Paste"), QApplication::clipboard()->mimeData(), addr);
    commandHistory()->addCommand(mcmd);
}

/* -------------------------------------- */

void ActionsImpl::slotNewFolder()
{
    KEBApp::self()->bkInfo()->commitChanges();
    bool ok;
    QString str = KInputDialog::getText( i18nc( "@title:window", "Create New Bookmark Folder" ),
            i18n( "New folder:" ), QString(), &ok, KEBApp::self() );
    if (!ok)
        return;

    CreateCommand *cmd = new CreateCommand(m_model,
                                KEBApp::self()->insertAddress(),
                                str, "bookmark_folder", /*open*/ true);
    commandHistory()->addCommand(cmd);
}

void ActionsImpl::slotNewBookmark()
{
    KEBApp::self()->bkInfo()->commitChanges();
    // TODO - make a setCurrentItem(Command *) which uses finaladdress interface
    CreateCommand * cmd = new CreateCommand(m_model,
                                KEBApp::self()->insertAddress(),
                                QString(), "www", KUrl("http://"));
    commandHistory()->addCommand(cmd);
}

void ActionsImpl::slotInsertSeparator()
{
    KEBApp::self()->bkInfo()->commitChanges();
    CreateCommand * cmd = new CreateCommand(m_model, KEBApp::self()->insertAddress());
    commandHistory()->addCommand(cmd);
}

void ActionsImpl::slotImport() {
    KEBApp::self()->bkInfo()->commitChanges();
    qDebug() << "ActionsImpl::slotImport() where sender()->name() == "
               << sender()->objectName() << endl;
    ImportCommand* import
        = ImportCommand::performImport(m_model, sender()->objectName(), KEBApp::self());
    if (!import)
        return;
    commandHistory()->addCommand(import);
    //FIXME select import->groupAddress
}

// TODO - this is getting ugly and repetitive. cleanup!

void ActionsImpl::slotExportOpera() {
    KEBApp::self()->bkInfo()->commitChanges();
    GlobalBookmarkManager::self()->doExport(GlobalBookmarkManager::OperaExport); }
void ActionsImpl::slotExportHTML() {
    KEBApp::self()->bkInfo()->commitChanges();
    GlobalBookmarkManager::self()->doExport(GlobalBookmarkManager::HTMLExport); }
void ActionsImpl::slotExportIE() {
    KEBApp::self()->bkInfo()->commitChanges();
    GlobalBookmarkManager::self()->doExport(GlobalBookmarkManager::IEExport); }
void ActionsImpl::slotExportNS() {
    KEBApp::self()->bkInfo()->commitChanges();
    GlobalBookmarkManager::self()->doExport(GlobalBookmarkManager::NetscapeExport); }
void ActionsImpl::slotExportMoz() {
    KEBApp::self()->bkInfo()->commitChanges();
    GlobalBookmarkManager::self()->doExport(GlobalBookmarkManager::MozillaExport); }

/* -------------------------------------- */

void ActionsImpl::slotCancelFavIconUpdates() {
    m_favIconHolder->cancelAllItrs();
}

void ActionsImpl::slotCancelAllTests() {
    m_testLinkHolder->cancelAllItrs();
}

void ActionsImpl::slotTestAll() {
    m_testLinkHolder->insertIterator(
            new TestLinkItr(m_testLinkHolder, KEBApp::self()->allBookmarks()));
}

void ActionsImpl::slotUpdateAllFavIcons() {
    m_favIconHolder->insertIterator(
            new FavIconsItr(m_favIconHolder, KEBApp::self()->allBookmarks()));
}

ActionsImpl::~ActionsImpl() {
    delete m_favIconHolder;
    delete m_testLinkHolder;
}

/* -------------------------------------- */

void ActionsImpl::slotTestSelection() {
    KEBApp::self()->bkInfo()->commitChanges();
    m_testLinkHolder->insertIterator(new TestLinkItr(m_testLinkHolder, KEBApp::self()->selectedBookmarksExpanded()));
}

void ActionsImpl::slotUpdateFavIcon() {
    KEBApp::self()->bkInfo()->commitChanges();
    m_favIconHolder->insertIterator(new FavIconsItr(m_favIconHolder, KEBApp::self()->selectedBookmarksExpanded()));
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
    KEBMacroCommand *mcmd = new KEBMacroCommand(i18nc("(qtundo-format)", "Recursive Sort"));
    KBookmarkGroupList lister(GlobalBookmarkManager::self()->mgr());
    QList<KBookmark> bookmarks = lister.getList(bk.toGroup());
    bookmarks << bk.toGroup();
    for (QList<KBookmark>::ConstIterator it = bookmarks.constBegin(); it != bookmarks.constEnd(); ++it) {
        new SortCommand(m_model, "", (*it).address(), mcmd);
    }
    commandHistory()->addCommand(mcmd);
}

void ActionsImpl::slotSort() {
    KEBApp::self()->bkInfo()->commitChanges();
    KBookmark bk = KEBApp::self()->firstSelected();
    Q_ASSERT(bk.isGroup());
    SortCommand *cmd = new SortCommand(m_model, i18nc("(qtundo-format)", "Sort Alphabetically"), bk.address());
    commandHistory()->addCommand(cmd);
}

/* -------------------------------------- */

void ActionsImpl::slotDelete() {
    KEBApp::self()->bkInfo()->commitChanges();
    DeleteManyCommand *mcmd = new DeleteManyCommand(m_model, i18nc("(qtundo-format)", "Delete Items"), KEBApp::self()->selectedBookmarks());
    commandHistory()->addCommand(mcmd);
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
    QUndoCommand *mcmd = CmdGen::setAsToolbar(m_model, bk);
    commandHistory()->addCommand(mcmd);
}

void ActionsImpl::slotChangeIcon() {
    KEBApp::self()->bkInfo()->commitChanges();
    KBookmark bk = KEBApp::self()->firstSelected();
    const QString newIcon = KIconDialog::getIcon(KIconLoader::Small, KIconLoader::Place, false, 0, false, KEBApp::self());
    if (newIcon.isEmpty())
        return;
    EditCommand *cmd = new EditCommand(m_model, bk.address(), -1, newIcon);

    commandHistory()->addCommand(cmd);
}

void ActionsImpl::slotExpandAll()
{
    KEBApp::self()->expandAll();
}

void ActionsImpl::slotCollapseAll()
{
    KEBApp::self()->collapseAll();
}

CommandHistory* ActionsImpl::commandHistory()
{
    return m_model->commandHistory();
}

#include "actionsimpl.moc"
