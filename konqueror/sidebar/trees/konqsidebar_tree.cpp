#include "konqsidebar_tree.h"
#include <kdesktopfile.h>
#include "konqsidebar_tree.moc"
#include "konq_sidebartree.h"
#include <kvbox.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kinputdialog.h>
#include <kiconloader.h>
#include <k3listviewsearchline.h>

#include <QtGui/QClipboard>
#include <QtGui/QToolButton>
#include <QtGui/QApplication>

KonqSidebar_Tree::KonqSidebar_Tree(const KComponentData &componentData, QWidget *parent,
                                   const QString &desktopName_, const KConfigGroup& configGroup)
    : KonqSidebarPlugin(componentData, parent, configGroup)
{
	const ModuleType virt = configGroup.readEntry("X-KDE-TreeModule", QString()) == "Virtual" ? VIRT_Folder : VIRT_Link;
        QString path;
	if (virt == VIRT_Folder)
            path = configGroup.readEntry("X-KDE-RelURL", QString());
        else {
            // The whole idea of using the same desktop file for the module
            // and for the toplevel item is broken. When renaming the toplevel item,
            // the module isn't renamed until the next konqueror restart (!).
            // We probably want to get rid of toplevel items when there's only one?
            path = KStandardDirs::locate("data", desktopName_); // ### this breaks global/local merging!
        }

	widget = new KVBox( parent );
        // TODO use QVBoxLayout

	if (configGroup.readEntry("X-KDE-SearchableTreeModule", false)) {
		KVBox* searchLine = new KVBox(widget);
		tree = new KonqSidebarTree(this, widget, virt, path);
		new K3ListViewSearchLineWidget(tree,searchLine);
	}
	else {
		tree = new KonqSidebarTree(this, widget, virt, path);
	}

	connect(tree, SIGNAL( openUrlRequest(const KUrl &, const KParts::OpenUrlArguments&, const KParts::BrowserArguments &)),
		this,SIGNAL( openUrlRequest(const KUrl &, const KParts::OpenUrlArguments&, const KParts::BrowserArguments &)));

	connect(tree,SIGNAL(createNewWindow(const KUrl &, const KParts::OpenUrlArguments &, const KParts::BrowserArguments &)),
		this,SIGNAL(createNewWindow(const KUrl &, const KParts::OpenUrlArguments &, const KParts::BrowserArguments &)));

	connect(tree,SIGNAL(popupMenu( const QPoint &, const KUrl &, const QString &, mode_t )),
		this,SIGNAL(popupMenu( const QPoint &, const KUrl &, const QString &, mode_t )));

	connect(tree,SIGNAL(popupMenu( const QPoint &, const KFileItemList & )),
		this,SIGNAL(popupMenu( const QPoint &, const KFileItemList & )));

	connect(tree,SIGNAL(enableAction( const char *, bool )),
		this,SIGNAL(enableAction( const char *, bool)));

}


KonqSidebar_Tree::~KonqSidebar_Tree(){;}

void* KonqSidebar_Tree::provides(const QString &) {return 0;}

//void KonqSidebar_Tree::emitStatusBarText (const QString &) {;}

QWidget *KonqSidebar_Tree::getWidget(){return widget;}

void KonqSidebar_Tree::handleURL(const KUrl &url)
    {
	emit started( 0 );
        tree->followURL( url );
        emit completed();
    }

void KonqSidebar_Tree::cut()
{
    QMimeData* mimeData = new QMimeData;
    if ( static_cast<KonqSidebarTreeItem*>(tree->selectedItem())->populateMimeData( mimeData, true ) )
        QApplication::clipboard()->setMimeData( mimeData );
    else
        delete mimeData;
}

void KonqSidebar_Tree::copy()
{
    QMimeData* mimeData = new QMimeData;
    if ( static_cast<KonqSidebarTreeItem*>(tree->selectedItem())->populateMimeData( mimeData, false ) )
        QApplication::clipboard()->setMimeData( mimeData );
    else
        delete mimeData;
}

void KonqSidebar_Tree::paste()
{
    if (tree->currentItem())
        tree->currentItem()->paste();
}

void KonqSidebar_Tree::trash()
{
    if (tree->currentItem())
        tree->currentItem()->trash();
}

void KonqSidebar_Tree::del()
{
    if (tree->currentItem())
        tree->currentItem()->del();
}

void KonqSidebar_Tree::rename()
{
    Q_ASSERT( tree->currentItem() );
    if (tree->currentItem())
        tree->currentItem()->rename();
}

class KonqSidebarTreeFactory
{
public:
    virtual ~KonqSidebarTreeFactory() {}

    KonqSidebarPlugin* createModule(const KComponentData &componentData, QWidget *parent,
                                    const QString &desktopname, const KConfigGroup& configGroup,
                                    const QVariant& unused) {
        Q_UNUSED(unused);
        return new KonqSidebar_Tree(componentData, parent, desktopname, configGroup);
    }


};

extern "C"
{
    KDE_EXPORT void*  create_konqsidebar_tree(const KComponentData &componentData, QWidget *parent,
                                              const QString &desktopname, const KConfigGroup& configGroup)
    {
        return new KonqSidebar_Tree(componentData, parent, desktopname, configGroup);
    }
}

extern "C"
{
    KDE_EXPORT bool add_konqsidebar_tree(QString* fn, QString*, QMap<QString,QString> *map)
    {
        const QStringList list = KGlobal::dirs()->findAllResources("data",
                                                                   "konqsidebartng/dirtree/*.desktop",
                                                                   KStandardDirs::NoDuplicates);
        QStringList names;
        for (QStringList::ConstIterator it=list.constBegin();it!=list.constEnd();++it)
        {
            KConfig _sc( *it, KConfig::SimpleConfig );
            KConfigGroup sc(&_sc, "Desktop Entry");
            names << sc.readEntry("Name");
        }

        // TODO this should also ask for the URL!
	QString item = KInputDialog::getItem( i18n( "Select Type" ),
                                              i18n( "Select type:" ), names );
	if (item.isEmpty())
            return false;

        const int id = names.indexOf(item);
        if (id == -1) return false;
        KConfig ksc2(list.at(id), KConfig::SimpleConfig);
        KConfigGroup desktopGroup(&ksc2, "Desktop Entry");
        map->insert("Type","Link");
        map->insert("Icon",desktopGroup.readEntry("Icon"));
        map->insert("Name",desktopGroup.readEntry("Name"));
        map->insert("Open","false");
        map->insert("URL",desktopGroup.readEntry("X-KDE-Default-URL"));
        map->insert("X-KDE-KonqSidebarModule","konqsidebar_tree");
        map->insert("X-KDE-TreeModule",desktopGroup.readEntry("X-KDE-TreeModule"));
        map->insert("X-KDE-TreeModule-ShowHidden",desktopGroup.readEntry("X-KDE-TreeModule-ShowHidden"));
        *fn = QString::fromLatin1("dirtree%1.desktop");
        return true;
    }
}
