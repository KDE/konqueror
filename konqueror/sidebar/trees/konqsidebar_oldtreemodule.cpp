#include "konqsidebar_oldtreemodule.h"
#include <kdesktopfile.h>
#include "konq_sidebartree.h"
#include <kvbox.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kconfig.h>
#include <kpluginfactory.h>
#include <kinputdialog.h>
#include <kiconloader.h>
#include <knameandurlinputdialog.h>
#include <k3listviewsearchline.h>

#include <QAction>
#include <QClipboard>
#include <QToolButton>
#include <QApplication>

KonqSidebarOldTreeModule::KonqSidebarOldTreeModule(const KComponentData &componentData, QWidget *parent,
                                   const QString &desktopName_, const KConfigGroup& configGroup)
    : KonqSidebarModule(componentData, parent, configGroup)
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
            path = KStandardDirs::locate("data", "konqsidebartng/entries/" + desktopName_); // ### this breaks global/local merging!
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

	connect(tree, SIGNAL(openUrlRequest(KUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)),
		this,SIGNAL(openUrlRequest(KUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)));

	connect(tree,SIGNAL(createNewWindow(KUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)),
		this,SIGNAL(createNewWindow(KUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)));

        connect(tree, SIGNAL(copy()),
                this, SLOT(copy()));
        connect(tree, SIGNAL(cut()),
                this, SLOT(cut()));
        connect(tree, SIGNAL(paste()),
                this, SLOT(pasteToSelection()));
}


KonqSidebarOldTreeModule::~KonqSidebarOldTreeModule(){}

QWidget *KonqSidebarOldTreeModule::getWidget() { return widget; }

void KonqSidebarOldTreeModule::handleURL(const KUrl &url)
    {
	emit started( 0 );
        tree->followURL( url );
        emit completed();
    }

void KonqSidebarOldTreeModule::cut()
{
    QMimeData* mimeData = new QMimeData;
    if ( static_cast<KonqSidebarTreeItem*>(tree->selectedItem())->populateMimeData( mimeData, true ) )
        QApplication::clipboard()->setMimeData( mimeData );
    else
        delete mimeData;
}

void KonqSidebarOldTreeModule::copy()
{
    kDebug();
    QMimeData* mimeData = new QMimeData;
    if ( static_cast<KonqSidebarTreeItem*>(tree->selectedItem())->populateMimeData( mimeData, false ) ) {
        kDebug() << "setting" << mimeData->formats();
        QApplication::clipboard()->setMimeData( mimeData );
    } else
        delete mimeData;
}

void KonqSidebarOldTreeModule::paste()
{
    // Not implemented. Would be for pasting into the toplevel.
    kDebug() << "not implemented. Didn't think it would be called - tell me (David Faure)";
}

void KonqSidebarOldTreeModule::pasteToSelection()
{
    if (tree->currentItem())
        tree->currentItem()->paste();
}

class KonqSidebarTreePlugin : public KonqSidebarPlugin
{
public:
    KonqSidebarTreePlugin(QObject* parent, const QVariantList& args)
        : KonqSidebarPlugin(parent, args) {}
    virtual ~KonqSidebarTreePlugin() {}

    virtual KonqSidebarModule* createModule(const KComponentData &componentData, QWidget *parent,
                                            const KConfigGroup& configGroup,
                                            const QString &desktopname,
                                            const QVariant& unused)
    {
        Q_UNUSED(unused);
        return new KonqSidebarOldTreeModule(componentData, parent, desktopname, configGroup);
    }

    virtual QList<QAction*> addNewActions(QObject* parent,
                                          const QList<KConfigGroup>& existingModules,
                                          const QVariant& unused)
    {
        Q_UNUSED(unused);

        QStringList existingTreeModules;
        Q_FOREACH(const KConfigGroup& cfg, existingModules)
            existingTreeModules.append(cfg.readEntry("X-KDE-TreeModule", QString()));

        QList<QAction*> actions;
        const QStringList list = KGlobal::dirs()->findAllResources("data",
                                                                   "konqsidebartng/dirtree/*.desktop",
                                                                   KStandardDirs::NoDuplicates);
        Q_FOREACH(const QString& desktopFile, list) {
            KDesktopFile df(desktopFile);
            const KConfigGroup desktopGroup = df.desktopGroup();
            const bool hasUrl = !(desktopGroup.readEntry("X-KDE-Default-URL", QString()).isEmpty());
            const QString treeModule = desktopGroup.readEntry("X-KDE-TreeModule", QString());

            // Assumption: modules without a default URL, don't use URLs at all,
            // and therefore are "unique" (no point in having two bookmarks modules
            // or two history modules). Modules with URLs can be added multiple times.
            if (hasUrl || !existingTreeModules.contains(treeModule)) {
                const QString name = df.readName();

                QAction* action = new QAction(parent);
                //action->setText(i18nc("@action:inmenu Add folder sidebar module", "Folder"));
                action->setText(name);
                action->setData(desktopFile);
                action->setIcon(KIcon(df.readIcon()));
                actions.append(action);
            }
        }
        return actions;
    }

    virtual QString templateNameForNewModule(const QVariant& actionData,
                                             const QVariant& unused) const
    {
        Q_UNUSED(unused);
        // Example: /full/path/to/bookmarks_module.desktop -> bookmarks%1.desktop
        QString str = actionData.toString();
        str = str.mid(str.lastIndexOf('/')+1);
        str.replace(".desktop", "%1.desktop");
        str.remove("_module");
        return str;
    }

    virtual bool createNewModule(const QVariant& actionData, KConfigGroup& configGroup,
                                 QWidget* parentWidget,
                                 const QVariant& unused)
    {
        Q_UNUSED(unused);
        const KDesktopFile df(actionData.toString());
        const KConfigGroup desktopGroup = df.desktopGroup();
        KUrl url = desktopGroup.readEntry("X-KDE-Default-URL");
        KNameAndUrlInputDialog dlg(i18nc("@label", "Name:"), i18nc("@label", "Path or URL:"), KUrl(), parentWidget);
        dlg.setCaption(i18nc("@title:window", "Add folder sidebar module"));
        dlg.setSuggestedName(df.readName());
        if (!dlg.exec())
            return false;

        configGroup.writeEntry("Type", "Link");
        configGroup.writeEntry("Icon", df.readIcon());
        configGroup.writeEntry("Name", dlg.name());
        configGroup.writeEntry("Open", false);
        configGroup.writePathEntry("URL", dlg.url().url());
        configGroup.writeEntry("X-KDE-KonqSidebarModule", "konqsidebar_tree");
        configGroup.writeEntry("X-KDE-TreeModule", desktopGroup.readEntry("X-KDE-TreeModule"));
        configGroup.writeEntry("X-KDE-TreeModule-ShowHidden", desktopGroup.readEntry("X-KDE-TreeModule-ShowHidden"));
        return true;
    }
};

K_PLUGIN_FACTORY(KonqSidebarTreePluginFactory, registerPlugin<KonqSidebarTreePlugin>(); )
K_EXPORT_PLUGIN(KonqSidebarTreePluginFactory())
#include "konqsidebar_oldtreemodule.moc"
