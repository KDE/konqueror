/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>
                 2003       Sven Leiber <s.leiber@web.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "knewmenu.h"
#include "knewmenu_p.h"
#include "konq_operations.h"

#include <QDir>
#include <QVBoxLayout>
#include <QList>
#include <kactioncollection.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kdirwatch.h>
#include <kicon.h>
#include <kcomponentdata.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kprotocolinfo.h>
#include <kprotocolmanager.h>
#include <kmenu.h>
#include <krun.h>
#include <kio/copyjob.h>
#include <kio/jobuidelegate.h>
#include <kio/renamedialog.h>
#include <kio/netaccess.h>
#include <kio/fileundomanager.h>

#include <kpropertiesdialog.h>
#include <ktemporaryfile.h>
#include <utime.h>

// For KUrlDesktopFileDlg
#include <QLayout>
#include <klineedit.h>
#include <kurlrequester.h>
#include <QLabel>

// Singleton, with data shared by all KNewMenu instances
class KNewMenuSingleton
{
public:
    KNewMenuSingleton()
        : templatesList(0),
          templatesVersion(0),
          filesParsed(false),
          dirWatch(0)
    {
    }
    ~KNewMenuSingleton()
    {
        delete templatesList;
        delete dirWatch;
    }

    /**
     * For entryType
     * LINKTOTEMPLATE: a desktop file that points to a file or dir to copy
     * TEMPLATE: a real file to copy as is (the KDE-1.x solution)
     * SEPARATOR: to put a separator in the menu
     * 0 means: not parsed, i.e. we don't know
     */
    enum EntryType { Unknown, LinkToTemplate = 1, Template, Separator };

    struct Entry {
        QString text;
        QString filePath; // empty for Separator
        QString templatePath; // same as filePath for Template
        QString icon;
        EntryType entryType;
        QString comment;
    };
    // NOTE: only filePath is known before we call parseFiles

    /**
     * List of all template files. It is important that they are in
     * the same order as the 'New' menu.
     */
    typedef QList<Entry> EntryList;
    EntryList * templatesList;

    /**
     * Is increased when templatesList has been updated and
     * menu needs to be re-filled. Menus have their own version and compare it
     * to templatesVersion before showing up
     */
    int templatesVersion;

    /**
     * Set back to false each time new templates are found,
     * and to true on the first call to parseFiles
     */
    bool filesParsed;
    KDirWatch * dirWatch;
};

K_GLOBAL_STATIC(KNewMenuSingleton, kNewMenuGlobals)

class KNewMenuPrivate
{
public:
    KNewMenuPrivate()
        : menuItemsVersion(0)
    {}
    KActionCollection * m_actionCollection;
    QWidget *m_parentWidget;
    KActionMenu *m_menuDev;
    QAction* m_newDirAction;

    int menuItemsVersion;

    /**
     * When the user pressed the right mouse button over an URL a popup menu
     * is displayed. The URL belonging to this popup menu is stored here.
     */
    KUrl::List popupFiles;

    QString m_tempFileToDelete; // set when a tempfile was created for a Type=URL desktop file

    /**
     * The action group that our actions belong to
     */
    QActionGroup* m_newMenuGroup;

    class StrategyInterface;
    class UrlDesktopFileStrategy;
    class SymLinkStrategy;
    class OtherDesktopFileStrategy;
    class RealFileOrDirStrategy;
};

KNewMenu::KNewMenu( KActionCollection *parent, QWidget* parentWidget, const QString& name )
    : KActionMenu( KIcon("document-new"), i18n( "Create New" ), parentWidget )
{
    // Don't fill the menu yet
    // We'll do that in slotCheckUpToDate (should be connected to aboutToShow)
    d = new KNewMenuPrivate;
    d->m_newMenuGroup = new QActionGroup(this);
    connect(d->m_newMenuGroup, SIGNAL(triggered(QAction*)), this, SLOT(slotActionTriggered(QAction*)));
    d->m_actionCollection = parent;
    d->m_parentWidget = parentWidget;
    d->m_newDirAction = 0;

    d->m_actionCollection->addAction( name, this );

    makeMenus();
}

KNewMenu::~KNewMenu()
{
    //kDebug(1203) << "KNewMenu::~KNewMenu " << this;
    delete d;
}

void KNewMenu::makeMenus()
{
    d->m_menuDev = new KActionMenu( KIcon("drive-removable-media"), i18n( "Link to Device" ), this );
}

void KNewMenu::slotCheckUpToDate( )
{
    KNewMenuSingleton* s = kNewMenuGlobals;
    //kDebug(1203) << "KNewMenu::slotCheckUpToDate() " << this
    //              << " : menuItemsVersion=" << d->menuItemsVersion
    //              << " s->templatesVersion=" << s->templatesVersion;
    if (d->menuItemsVersion < s->templatesVersion || s->templatesVersion == 0) {
        //kDebug(1203) << "KNewMenu::slotCheckUpToDate() : recreating actions";
        // We need to clean up the action collection
        // We look for our actions using the group
        foreach (QAction* action, d->m_newMenuGroup->actions())
            delete action;

        if (!s->templatesList) { // No templates list up to now
            s->templatesList = new KNewMenuSingleton::EntryList;
            slotFillTemplates();
            parseFiles();
        }

        // This might have been already done for other popupmenus,
        // that's the point in s->filesParsed.
        if ( !s->filesParsed )
            parseFiles();

        fillMenu();

        d->menuItemsVersion = s->templatesVersion;
    }
}

void KNewMenu::parseFiles()
{
    KNewMenuSingleton* s = kNewMenuGlobals;
    //kDebug(1203) << "KNewMenu::parseFiles()";
    s->filesParsed = true;
    KNewMenuSingleton::EntryList::iterator templ = s->templatesList->begin();
    const KNewMenuSingleton::EntryList::iterator templ_end = s->templatesList->end();
    for ( ; templ != templ_end; ++templ )
    {
        QString iconname;
        QString filePath = (*templ).filePath;
        if ( !filePath.isEmpty() )
        {
            QString text;
            QString templatePath;
            // If a desktop file, then read the name from it.
            // Otherwise (or if no name in it?) use file name
            if ( KDesktopFile::isDesktopFile( filePath ) ) {
                KDesktopFile desktopFile(  filePath );
                const KConfigGroup config = desktopFile.desktopGroup();
                text = config.readEntry("Name");
                (*templ).icon = config.readEntry("Icon");
                (*templ).comment = config.readEntry("Comment");
                QString type = config.readEntry( "Type" );
                if ( type == "Link" )
                {
                    templatePath = config.readPathEntry("URL", QString());
                    if ( templatePath[0] != '/' && !templatePath.startsWith("__"))
                    {
                        if ( templatePath.startsWith("file:/") )
                            templatePath = KUrl(templatePath).toLocalFile();
                        else
                        {
                            // A relative path, then (that's the default in the files we ship)
                            QString linkDir = filePath.left( filePath.lastIndexOf( '/' ) + 1 /*keep / */ );
                            //kDebug(1203) << "linkDir=" << linkDir;
                            templatePath = linkDir + templatePath;
                        }
                    }
                }
                if ( templatePath.isEmpty() )
                {
                    // No URL key, this is an old-style template
                    (*templ).entryType = KNewMenuSingleton::Template;
                    (*templ).templatePath = (*templ).filePath; // we'll copy the file
                } else {
                    (*templ).entryType = KNewMenuSingleton::LinkToTemplate;
                    (*templ).templatePath = templatePath;
                }

            }
            if (text.isEmpty())
            {
                text = KUrl(filePath).fileName();
                if ( text.endsWith(".desktop") )
                    text.truncate( text.length() - 8 );
            }
            (*templ).text = text;
            /*kDebug(1203) << "Updating entry with text=" << text
                          << "entryType=" << (*templ).entryType
                          << "templatePath=" << (*templ).templatePath;*/
        }
        else {
            (*templ).entryType = KNewMenuSingleton::Separator;
        }
    }
}

void KNewMenu::fillMenu()
{
    //kDebug(1203) << "KNewMenu::fillMenu()";
    menu()->clear();
    d->m_menuDev->menu()->clear();
    d->m_newDirAction = 0;

    QSet<QString> seenTexts;
    // these shall be put at special positions
    QAction* linkURL = 0;
    QAction* linkApp = 0;
    QAction* linkPath = 0;

    KNewMenuSingleton* s = kNewMenuGlobals;
    int i = 1;
    KNewMenuSingleton::EntryList::const_iterator templ = s->templatesList->constBegin();
    const KNewMenuSingleton::EntryList::const_iterator templ_end = s->templatesList->constEnd();
    for ( ; templ != templ_end; ++templ, ++i)
    {
        if ((*templ).entryType != KNewMenuSingleton::Separator) {
            // There might be a .desktop for that one already, if it's a kdelnk
            // This assumes we read .desktop files before .kdelnk files ...

            // In fact, we skip any second item that has the same text as another one.
            // Duplicates in a menu look bad in any case.

            const bool bSkip = seenTexts.contains((*templ).text);
            if ( bSkip ) {
                kDebug(1203) << "KNewMenu: skipping" << (*templ).filePath;
            } else {
                seenTexts.insert((*templ).text);
                //const KNewMenuSingleton::Entry entry = s->templatesList->at( i-1 );

                const QString templatePath = (*templ).templatePath;
                // The best way to identify the "Create Directory", "Link to Location", "Link to Application" was the template
                if (templatePath.endsWith("emptydir")) {
                    QAction * act = new QAction( this );
                    d->m_newDirAction = act;
                    act->setIcon( KIcon((*templ).icon) );
                    act->setText( (*templ).text );
                    act->setActionGroup( d->m_newMenuGroup );
                    menu()->addAction( act );

                    QAction *sep = new QAction(this);
                    sep->setSeparator( true );
                    menu()->addAction( sep );
                }
                else
                {
                    QAction * act = new QAction(this);
                    act->setData( i );
                    act->setIcon( KIcon((*templ).icon) );
                    act->setText( (*templ).text );
                    act->setActionGroup( d->m_newMenuGroup );

                    //kDebug() << templatePath << (*templ).filePath;

                    if (templatePath.endsWith("/URL.desktop")) {
                        linkURL = act;
                    } else if (templatePath.endsWith("/Program.desktop")) {
                        linkApp = act;
                    } else if ((*templ).filePath.endsWith("/linkPath.desktop")) {
                        linkPath = act;
                    } else if (KDesktopFile::isDesktopFile(templatePath)) {
                        KDesktopFile df(templatePath);
                        if (df.readType() == "FSDevice")
                            d->m_menuDev->menu()->addAction( act );
                        else
                            menu()->addAction( act );
                    }
                    else
                    {
                        menu()->addAction( act );
                    }
                }
            }
        } else { // Separate system from personal templates
            Q_ASSERT( (*templ).entryType != 0 );

            QAction *sep = new QAction( this );
            sep->setSeparator( true );
            menu()->addAction( sep );
        }
    }

    QAction *sep = new QAction( this );
    sep->setSeparator( true );
    menu()->addAction( sep );
    if ( linkURL ) menu()->addAction( linkURL );
    if ( linkPath ) menu()->addAction( linkPath );
    if ( linkApp ) menu()->addAction( linkApp );
    Q_ASSERT(d->m_menuDev);
    menu()->addAction( d->m_menuDev );

}

void KNewMenu::slotFillTemplates()
{
    KNewMenuSingleton* s = kNewMenuGlobals;
    //kDebug(1203) << "KNewMenu::slotFillTemplates()";
    // Ensure any changes in the templates dir will call this
    if ( ! s->dirWatch ) {
        s->dirWatch = new KDirWatch;
        const QStringList dirs = d->m_actionCollection->componentData().dirs()->resourceDirs("templates");
        for ( QStringList::const_iterator it = dirs.constBegin() ; it != dirs.constEnd() ; ++it ) {
            //kDebug(1203) << "Templates resource dir:" << *it;
            s->dirWatch->addDir( *it );
        }
        connect ( s->dirWatch, SIGNAL( dirty( const QString & ) ),
                  this, SLOT ( slotFillTemplates() ) );
        connect ( s->dirWatch, SIGNAL( created( const QString & ) ),
                  this, SLOT ( slotFillTemplates() ) );
        connect ( s->dirWatch, SIGNAL( deleted( const QString & ) ),
                  this, SLOT ( slotFillTemplates() ) );
        // Ok, this doesn't cope with new dirs in KDEDIRS, but that's another story
    }
    ++s->templatesVersion;
    s->filesParsed = false;

    s->templatesList->clear();

    // Look into "templates" dirs.
    const QStringList files = d->m_actionCollection->componentData().dirs()->findAllResources("templates");
    QMap<QString, KNewMenuSingleton::Entry> slist; // used for sorting
    Q_FOREACH(const QString& file, files) {
        //kDebug(1203) << file;
        if (file[0] != '.') {
            KNewMenuSingleton::Entry e;
            e.filePath = file;
            e.entryType = KNewMenuSingleton::Unknown; // not parsed yet

            // Put Directory first in the list (a bit hacky),
            // and TextFile before others because it's the most used one.
            // This also sorts by user-visible name.
            // The rest of the re-ordering is done in fillMenu.
            const KDesktopFile config(file);
            QString key = config.desktopGroup().readEntry("Name");
            if (file.endsWith("Directory.desktop")) {
                key.prepend('0');
            } else if (file.endsWith("TextFile.desktop")) {
                key.prepend('1');
            } else {
                key.prepend('2');
            }
            slist.insert(key, e);
        }
    }
    (*s->templatesList) += slist.values();
}

void KNewMenu::createDirectory()
{
    if (d->popupFiles.isEmpty())
       return;

    KIO::SimpleJob* job = KonqOperations::newDir(d->m_parentWidget, d->popupFiles.first());
    if (job) {
        // We want the error handling to be done by slotResult so that subclasses can reimplement it
        job->ui()->setAutoErrorHandlingEnabled(false);
        connect( job, SIGNAL( result( KJob * ) ),
                 SLOT( slotResult( KJob * ) ) );
    }

}

class KNewMenuPrivate::StrategyInterface
{
public:
    void setParentWidget(QWidget* widget) { m_parentWidget = widget; }
    void setPopupFiles(const KUrl::List& urls) { m_popupUrls = urls; }

    virtual void execute(const KNewMenuSingleton::Entry& entry) = 0;
    virtual ~StrategyInterface() {}

    QString chosenFileName() const { return m_chosenFileName; }
    // If empty, no copy is performed.
    QString sourceFileToCopy() const { return m_src; }
    QString tempFileToDelete() const { return m_tempFileToDelete; }

    bool checkSourceExists(const QString& src);

protected:
    QWidget* m_parentWidget;
    QString m_chosenFileName;
    QString m_src;
    QString m_tempFileToDelete;
    KUrl::List m_popupUrls;
};

// The strategy used for "url" desktop files
class KNewMenuPrivate::UrlDesktopFileStrategy : public KNewMenuPrivate::StrategyInterface
{
public:
    virtual void execute(const KNewMenuSingleton::Entry& entry);
};

// The strategy used when creating a symlink
class KNewMenuPrivate::SymLinkStrategy : public KNewMenuPrivate::StrategyInterface
{
public:
    virtual void execute(const KNewMenuSingleton::Entry& entry);
};

// The strategy used for other desktop files than Type=Link. Example: Application, Device.
class KNewMenuPrivate::OtherDesktopFileStrategy : public KNewMenuPrivate::StrategyInterface
{
public:
    virtual void execute(const KNewMenuSingleton::Entry& entry);
};

// The strategy used for "real files or directories" (the common case)
class KNewMenuPrivate::RealFileOrDirStrategy : public KNewMenuPrivate::StrategyInterface
{
public:
    virtual void execute(const KNewMenuSingleton::Entry& entry);
};

bool KNewMenuPrivate::StrategyInterface::checkSourceExists(const QString& src)
{
    if (!QFile::exists(src)) {
        kWarning(1203) << src << "doesn't exist" ;
        KMessageBox::sorry(m_parentWidget, i18n("<qt>The template file <b>%1</b> does not exist.</qt>", src));
        return false;
    }
    return true;
}

void KNewMenuPrivate::UrlDesktopFileStrategy::execute(const KNewMenuSingleton::Entry& entry)
{
    // entry.comment contains i18n("Enter link to location (URL):"). JFYI :)
    KUrlDesktopFileDlg dlg(i18n("File name:"), entry.comment, m_popupUrls.first(), m_parentWidget);
    // TODO dlg.setCaption( i18n( ... ) );
    if (!dlg.exec())
        return;

    m_chosenFileName = dlg.fileName();
    const KUrl linkUrl = dlg.url(); // the url to put in the file
    if ( m_chosenFileName.isEmpty() || linkUrl.isEmpty() )
        return;

    // It's a "URL" desktop file; we need to make a temp copy of it, to modify it
    // before copying it to the final destination [which could be a remote protocol]
    KTemporaryFile tmpFile;
    tmpFile.setAutoRemove(false); // done below
    if (!tmpFile.open()) {
        kError() << "Couldn't create temp file!";
        return;
    }

    if (!checkSourceExists(entry.templatePath)) {
        return;
    }

    // First copy the template into the temp file
    QFile file(entry.templatePath);
    if (!file.open(QIODevice::ReadOnly)) {
        kError() << "Couldn't open template" << entry.templatePath;
        return;
    }
    const QByteArray data = file.readAll();
    tmpFile.write(data);
    const QString tempFileName = tmpFile.fileName();
    Q_ASSERT(!tempFileName.isEmpty());
    tmpFile.close();

    KDesktopFile df(tempFileName);
    KConfigGroup group = df.desktopGroup();
    group.writeEntry("Icon", KProtocolInfo::icon(linkUrl.protocol()));
    group.writePathEntry("URL", linkUrl.prettyUrl());
    df.sync();
    m_src = tempFileName;
    m_tempFileToDelete = tempFileName;
}

void KNewMenuPrivate::SymLinkStrategy::execute(const KNewMenuSingleton::Entry& entry)
{
    KUrlDesktopFileDlg dlg(i18n("File name:"), entry.comment, m_popupUrls.first(), m_parentWidget);
    // TODO dlg.setCaption( i18n( ... ) );
    if (!dlg.exec())
        return;
    m_chosenFileName = dlg.fileName();
    KUrl linkUrl = dlg.url(); // the url to put in the file
    if (m_chosenFileName.isEmpty() || linkUrl.isEmpty())
        return;

    if (linkUrl.isRelative())
        m_src = linkUrl.url();
    else if (linkUrl.isLocalFile())
        m_src = linkUrl.toLocalFile();
    else {
        KMessageBox::sorry(m_parentWidget, i18n("Basic links can only point to local files or directories.\nPlease use \"Link to Location\" for remote URLs."));
        return;
    }
}

void KNewMenuPrivate::OtherDesktopFileStrategy::execute(const KNewMenuSingleton::Entry& entry)
{
    if (!checkSourceExists(entry.templatePath)) {
        return;
    }

    KUrl::List::const_iterator it = m_popupUrls.constBegin();
    for ( ; it != m_popupUrls.constEnd(); ++it )
    {
        //kDebug(1203) << "first arg=" << entry.templatePath;
        //kDebug(1203) << "second arg=" << (*it).url();
        //kDebug(1203) << "third arg=" << entry.text;
        QString text = entry.text;
        text.remove( "..." ); // the ... is fine for the menu item but not for the default filename

        KUrl defaultFile( *it );
        defaultFile.addPath( KIO::encodeFileName( text ) );
        if ( defaultFile.isLocalFile() && QFile::exists( defaultFile.toLocalFile() ) )
            text = KIO::RenameDialog::suggestName( *it, text);

        const KUrl templateUrl(entry.templatePath);
        KPropertiesDialog dlg(templateUrl, *it, text, m_parentWidget);
        dlg.exec();
    }
    // We don't set m_src here -> there will be no copy, we are done.
}

void KNewMenuPrivate::RealFileOrDirStrategy::execute(const KNewMenuSingleton::Entry& entry)
{
    // The template is not a desktop file
    // Show the small dialog for getting the destination filename
    QString text = entry.text;
    text.remove("..."); // the ... is fine for the menu item but not for the default filename

    KUrl defaultFile(m_popupUrls.first());
    defaultFile.addPath(KIO::encodeFileName(text));
    if (defaultFile.isLocalFile() && QFile::exists(defaultFile.toLocalFile()))
        text = KIO::RenameDialog::suggestName(m_popupUrls.first(), text);

    bool ok;
    const QString name = KInputDialog::getText(QString(), entry.comment,
                                               text, &ok, m_parentWidget);
    if ( ok ) {
        m_chosenFileName = name;
        m_src = entry.templatePath;
    }
}

void KNewMenu::slotActionTriggered(QAction* action)
{
    trigger(); // was for kdesktop's slotNewMenuActivated() in kde3 times. Can't hurt to keep it...

    if (action == d->m_newDirAction) {
        createDirectory();
        return;
    }
    const int id = action->data().toInt();
    Q_ASSERT(id > 0);

    KNewMenuSingleton* s = kNewMenuGlobals;
    const KNewMenuSingleton::Entry entry = s->templatesList->at( id - 1 );
    //kDebug(1203) << "sFile=" << sFile;

    const bool createSymlink = entry.templatePath == "__CREATE_SYMLINK__";
    KNewMenuPrivate::StrategyInterface* strategy = 0;

    if (createSymlink) {
        strategy = new KNewMenuPrivate::SymLinkStrategy;
    } else if (KDesktopFile::isDesktopFile(entry.templatePath)) {
        KDesktopFile df(entry.templatePath);
        if (df.readType() == "Link") {
            strategy = new KNewMenuPrivate::UrlDesktopFileStrategy;
        } else { // any other desktop file (Device, App, etc.)
            strategy = new KNewMenuPrivate::OtherDesktopFileStrategy;
        }
    } else {
        strategy = new KNewMenuPrivate::RealFileOrDirStrategy;
    }
    strategy->setParentWidget(d->m_parentWidget);
    strategy->setPopupFiles(d->popupFiles);
    strategy->execute(entry);
    d->m_tempFileToDelete = strategy->tempFileToDelete();
    const QString src = strategy->sourceFileToCopy();
    const QString chosenFileName = strategy->chosenFileName();
    delete strategy;

    if (src.isEmpty())
        return;
    KUrl uSrc(src);

    // The template is not a desktop file [or it's a URL one]
    // Copy it.
    KUrl::List::const_iterator it = d->popupFiles.constBegin();
    for (; it != d->popupFiles.constEnd(); ++it)
    {
        KUrl dest( *it );
        dest.addPath(KIO::encodeFileName(chosenFileName));

        KUrl::List lstSrc;
        lstSrc.append(uSrc);
        KIO::Job* kjob;
        if (createSymlink) {
            kjob = KIO::symlink(src, dest);
            // This doesn't work, FileUndoManager registers new links in copyingLinkDone,
            // which KIO::symlink obviously doesn't emit... Needs code in FileUndoManager.
            //KIO::FileUndoManager::self()->recordJob(KIO::FileUndoManager::Link, lstSrc, dest, kjob);
        } else {
            //kDebug(1203) << "KNewMenu : KIO::copyAs(" << uSrc.url() << "," << dest.url() << ")";
            KIO::CopyJob * job = KIO::copyAs(uSrc, dest);
            job->setDefaultPermissions(true);
            kjob = job;
            KIO::FileUndoManager::self()->recordJob(KIO::FileUndoManager::Copy, lstSrc, dest, job);
        }
        kjob->ui()->setWindow(d->m_parentWidget);
        connect(kjob, SIGNAL(result(KJob*)),
                SLOT(slotResult(KJob*)));
    }
}

void KNewMenu::slotResult( KJob * job )
{
    if (job->error()) {
        static_cast<KIO::Job*>( job )->ui()->showErrorMessage();
    } else {
        // Was this a copy or a mkdir?
        KIO::CopyJob* copyJob = ::qobject_cast<KIO::CopyJob*>(job);
        if (copyJob) {
            const KUrl destUrl = copyJob->destUrl();
            const KUrl localUrl = KIO::NetAccess::mostLocalUrl(destUrl, d->m_parentWidget);
            if (localUrl.isLocalFile()) {
                // Normal (local) file. Need to "touch" it, kio_file copied the mtime.
                (void) ::utime(QFile::encodeName(localUrl.toLocalFile()), 0);
            }
            emit itemCreated(destUrl);
        } else if (KIO::SimpleJob* simpleJob = ::qobject_cast<KIO::SimpleJob*>(job)) {
            // Can be mkdir or symlink
            kDebug() << "Emit itemCreated" << simpleJob->url();
            emit itemCreated(simpleJob->url());
        }
    }
    if (!d->m_tempFileToDelete.isEmpty())
        QFile::remove(d->m_tempFileToDelete);
}

void KNewMenu::setPopupFiles(const KUrl::List& files)
{
    d->popupFiles = files;
    if (files.isEmpty()) {
        d->m_newMenuGroup->setEnabled(false);
    } else {
        KUrl firstUrl = files.first();
        if (KProtocolManager::supportsWriting(firstUrl)) {
            d->m_newMenuGroup->setEnabled(true);
            if (d->m_newDirAction) {
                d->m_newDirAction->setEnabled(KProtocolManager::supportsMakeDir(firstUrl)); // e.g. trash:/
            }
        } else {
            d->m_newMenuGroup->setEnabled(true);
        }
    }
}

//////////

KUrlDesktopFileDlg::KUrlDesktopFileDlg(const QString& textFileName, const QString& textUrl, const KUrl& dirUrl, QWidget *parent)
    : KDialog( parent )
{
    setButtons( Ok | Cancel | User1 );
    setButtonGuiItem( User1, KStandardGuiItem::clear() );
    showButtonSeparator( true );

    initDialog(textFileName, QString(), textUrl, QString(), dirUrl);
}

void KUrlDesktopFileDlg::initDialog(const QString& textFileName, const QString& defaultName, const QString& textUrl, const QString& defaultUrl, const KUrl& dirUrl)
{
    QFrame *plainPage = new QFrame( this );
    setMainWidget( plainPage );

    QVBoxLayout * topLayout = new QVBoxLayout( plainPage );
    topLayout->setMargin( 0 );
    topLayout->setSpacing( spacingHint() );

    // First line: filename
    KHBox * fileNameBox = new KHBox( plainPage );
    topLayout->addWidget( fileNameBox );

    QLabel * label = new QLabel( textFileName, fileNameBox );
    m_leFileName = new KLineEdit( fileNameBox );
    m_leFileName->setMinimumWidth(m_leFileName->sizeHint().width() * 3);
    label->setBuddy(m_leFileName);  // please "scheck" style
    m_leFileName->setText( defaultName );
    m_leFileName->setSelection(0, m_leFileName->text().length()); // autoselect
    connect( m_leFileName, SIGNAL(textChanged(const QString&)),
             SLOT(slotNameTextChanged(const QString&)) );

    // Second line: url
    KHBox * urlBox = new KHBox( plainPage );
    topLayout->addWidget( urlBox );
    label = new QLabel( textUrl, urlBox );
    m_urlRequester = new KUrlRequester( defaultUrl, urlBox);
    m_urlRequester->setStartDir(dirUrl);
    m_urlRequester->setMode( KFile::File | KFile::Directory );

    m_urlRequester->setMinimumWidth( m_urlRequester->sizeHint().width() * 3 );
    connect( m_urlRequester->lineEdit(), SIGNAL(textChanged(const QString&)),
             SLOT(slotURLTextChanged(const QString&)) );
    label->setBuddy(m_urlRequester);  // please "scheck" style

    m_urlRequester->setFocus();
    enableButtonOk( !defaultName.isEmpty() && !defaultUrl.isEmpty() );
    connect( this, SIGNAL(user1Clicked()), this, SLOT(slotClear()) );
    m_fileNameEdited = false;
}

KUrl KUrlDesktopFileDlg::url() const
{
    if ( result() == QDialog::Accepted ) {
        return m_urlRequester->url();
    }
    else
        return KUrl();
}

QString KUrlDesktopFileDlg::fileName() const
{
    if ( result() == QDialog::Accepted )
        return m_leFileName->text();
    else
        return QString();
}

void KUrlDesktopFileDlg::slotClear()
{
    m_leFileName->clear();
    m_urlRequester->clear();
    m_fileNameEdited = false;
}

void KUrlDesktopFileDlg::slotNameTextChanged( const QString& )
{
    m_fileNameEdited = true;
    enableButtonOk( !m_leFileName->text().isEmpty() && !m_urlRequester->url().isEmpty() );
}

void KUrlDesktopFileDlg::slotURLTextChanged( const QString& )
{
    if ( !m_fileNameEdited )
    {
        // use URL as default value for the filename
        // (we copy only its filename if protocol supports listing,
        // but for HTTP we don't want tons of index.html links)
        KUrl url( m_urlRequester->url() );
        kDebug() << url << url.isValid() << url.isLocalFile() << url.fileName();
        if (KProtocolManager::supportsListing(url) && !url.fileName().isEmpty())
            m_leFileName->setText( url.fileName() );
        else
            m_leFileName->setText( url.url() );
        m_fileNameEdited = false; // slotNameTextChanged set it to true erroneously
    }
    enableButtonOk( !m_leFileName->text().isEmpty() && !m_urlRequester->url().isEmpty() );
}


#include "knewmenu.moc"
#include "knewmenu_p.moc"
