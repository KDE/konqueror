/* This file is part of FSView.
    SPDX-FileCopyrightText: 2002, 2003 Josef Weidendorfer <Josef.Weidendorfer@gmx.de>

    Some file management code taken from the Dolphin file manager:
    SPDX-FileCopyrightText: 2006-2009 Peter Penz <peter.penz19@mail.com>

    SPDX-License-Identifier: GPL-2.0-only
*/

/*
 * The KPart embedding the FSView widget
 */

#include "fsview_part.h"

#include <QClipboard>
#include <QTimer>
#include <QStyle>

#include <kfileitem.h>
#include <kpluginfactory.h>
#include <KPluginMetaData>

#include <kprotocolmanager.h>
#include <kio/copyjob.h>
#include <kio/deletejob.h>
#include <kio/paste.h>
#include <kmessagebox.h>
#include <kactionmenu.h>
#include <kactioncollection.h>
#include <kpropertiesdialog.h>
#include <KMimeTypeEditor>
#include <kio/jobuidelegate.h>
#include <KIO/FileUndoManager>
#include <KJobWidgets>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <ksharedconfig.h>
#include <KLocalizedString>
#include <KIO/ApplicationLauncherJob>
#include <KIO/JobUiDelegateFactory>

#include <QApplication>
#include <QMimeData>

#include "fsviewdebug.h"

K_PLUGIN_CLASS_WITH_JSON(FSViewPart, "fsview_part.json")

// FSJob, for progress

FSJob::FSJob(FSView *v)
    : KIO::Job()
{
    _view = v;
    connect(v, &FSView::progress, this, &FSJob::progressSlot);
}

void FSJob::kill(bool /*quietly*/)
{
    _view->stop();

    Job::kill();
}

void FSJob::progressSlot(int percent, int dirs, const QString &cDir)
{
    if (percent < 100) {
        emitPercent(percent, 100);
        slotInfoMessage(this, i18np("Read 1 folder, in %2",
                                    "Read %1 folders, in %2",
                                    dirs, cDir), QString());
    } else {
        slotInfoMessage(this, i18np("1 folder", "%1 folders", dirs), QString());
    }
}

// FSViewPart

FSViewPart::FSViewPart(QWidget *parentWidget,
                       QObject *parent,
                       const KPluginMetaData& metaData,
                       const QList<QVariant> & /* args */)
    : KParts::ReadOnlyPart(parent)
{
    setMetaData(metaData);
    _view = new FSView(new Inode(), parentWidget);
    _view->setWhatsThis(i18n("<p>This is the FSView plugin, a graphical "
                             "browsing mode showing filesystem utilization "
                             "by using a tree map visualization.</p>"
                             "<p>Note that in this mode, automatic updating "
                             "when filesystem changes are made "
                             "is intentionally <b>not</b> done.</p>"
                             "<p>For details on usage and options available, "
                             "see the online help under "
                             "menu 'Help/FSView Manual'.</p>"));

    _view->show();
    setWidget(_view);

    _ext = new FSViewBrowserExtension(this);
    _job = nullptr;

    _areaMenu = new KActionMenu(i18n("Stop at Area"),
                                actionCollection());
    actionCollection()->addAction(QStringLiteral("treemap_areadir"), _areaMenu);
    _depthMenu = new KActionMenu(i18n("Stop at Depth"),
                                 actionCollection());
    actionCollection()->addAction(QStringLiteral("treemap_depthdir"), _depthMenu);
    _visMenu = new KActionMenu(i18n("Visualization"),
                               actionCollection());
    actionCollection()->addAction(QStringLiteral("treemap_visdir"), _visMenu);

    _colorMenu = new KActionMenu(i18n("Color Mode"),
                                 actionCollection());
    actionCollection()->addAction(QStringLiteral("treemap_colordir"), _colorMenu);

    QAction *action;
    action = actionCollection()->addAction(QStringLiteral("help_fsview"));
    action->setText(i18n("&FSView Manual"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("fsview")));
    action->setToolTip(i18n("Show FSView manual"));
    action->setWhatsThis(i18n("Opens the help browser with the "
                              "FSView documentation"));
    connect(action, &QAction::triggered, this, &FSViewPart::showHelp);

    connect(_visMenu->menu(), &QMenu::aboutToShow,this, &FSViewPart::slotShowVisMenu);
    connect(_areaMenu->menu(), &QMenu::aboutToShow, this, &FSViewPart::slotShowAreaMenu);
    connect(_depthMenu->menu(), &QMenu::aboutToShow, this, &FSViewPart::slotShowDepthMenu);
    connect(_colorMenu->menu(), &QMenu::aboutToShow, this, &FSViewPart::slotShowColorMenu);

    // Both of these click signals are connected.  Whether a single or
    // double click activates an item is checked against the current
    // style setting when the click happens.
    connect(_view, &FSView::clicked, _ext, &FSViewBrowserExtension::itemSingleClicked);
    connect(_view, &FSView::doubleClicked, _ext, &FSViewBrowserExtension::itemDoubleClicked);

    connect(_view, &TreeMapWidget::returnPressed, _ext, &FSViewBrowserExtension::selected);
    connect(_view, QOverload<>::of(&TreeMapWidget::selectionChanged), this, &FSViewPart::updateActions);
    connect(_view, &TreeMapWidget::contextMenuRequested, this, &FSViewPart::contextMenu);

    connect(_view, &FSView::started, this, &FSViewPart::startedSlot);
    connect(_view, &FSView::completed, this, &FSViewPart::completedSlot);

    // Create common file management actions - this is necessary in KDE4
    // as these common actions are no longer automatically part of KParts.
    // Much of this is taken from Dolphin.
    // FIXME: Renaming didn't even seem to work in KDE3! Implement (non-inline) renaming
    // functionality.
    //QAction* renameAction = m_actionCollection->addAction("rename");
    //rename->setText(i18nc("@action:inmenu Edit", "Rename..."));
    //rename->setShortcut(Qt::Key_F2);

    QAction *moveToTrashAction = actionCollection()->addAction(QStringLiteral("move_to_trash"));
    moveToTrashAction->setText(i18nc("@action:inmenu File", "Move to Trash"));
    moveToTrashAction->setIcon(QIcon::fromTheme(QStringLiteral("user-trash")));
    actionCollection()->setDefaultShortcut(moveToTrashAction, QKeySequence(QKeySequence::Delete));
    connect(moveToTrashAction, &QAction::triggered, _ext, &FSViewBrowserExtension::trash);

    QAction *deleteAction = actionCollection()->addAction(QStringLiteral("delete"));
    deleteAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    deleteAction->setText(i18nc("@action:inmenu File", "Delete"));
    actionCollection()->setDefaultShortcut(deleteAction, QKeySequence(Qt::SHIFT | Qt::Key_Delete));
    connect(deleteAction, &QAction::triggered, _ext, &FSViewBrowserExtension::del);

    QAction *editMimeTypeAction = actionCollection()->addAction(QStringLiteral("editMimeType"));
    editMimeTypeAction->setText(i18nc("@action:inmenu Edit", "&Edit File Type..."));
    connect(editMimeTypeAction, &QAction::triggered, _ext, &FSViewBrowserExtension::editMimeType);

    QAction *propertiesAction = actionCollection()->addAction(QStringLiteral("properties"));
    propertiesAction->setText(i18nc("@action:inmenu File", "Properties"));
    propertiesAction->setIcon(QIcon::fromTheme(QStringLiteral("document-properties")));
    propertiesAction->setShortcut(Qt::ALT | Qt::Key_Return);
    connect(propertiesAction, &QAction::triggered, this, &FSViewPart::slotProperties);

    QTimer::singleShot(1, this, SLOT(showInfo()));

    updateActions();

    setXMLFile(QStringLiteral("fsview_part.rc"));
}

FSViewPart::~FSViewPart()
{
    qCDebug(FSVIEWLOG);

    delete _job;
    _view->saveFSOptions();
}

QString FSViewPart::componentName() const
{
    // also the part ui.rc file is in the program folder
    // TODO: change the component name to "fsviewpart" by removing this method and
    // adapting the folder where the file is placed.
    // Needs a way to also move any potential custom user ui.rc files
    // from fsview/fsview_part.rc to fsviewpart/fsview_part.rc
    return QStringLiteral("fsview");
}

void FSViewPart::showInfo()
{
    QString info;
    info = i18n("FSView intentionally does not support automatic updates "
                "when changes are made to files or directories, "
                "currently visible in FSView, from the outside.\n"
                "For details, see the 'Help/FSView Manual'.");

    KMessageBox::information(_view, info, QString(), QStringLiteral("ShowFSViewInfo"));
}

void FSViewPart::showHelp()
{
    const KService::Ptr helpCenter = KService::serviceByDesktopName(QStringLiteral("org.kde.khelpcenter"));
    auto job = new KIO::ApplicationLauncherJob(helpCenter);
    job->setUrls({QUrl(QStringLiteral("help:/konqueror/index.html#fsview"))});
    job->start();
}

void FSViewPart::startedSlot()
{
    _job = new FSJob(_view);
    _job->setUiDelegate(KIO::createDefaultJobUiDelegate());
    emit started(_job);
}

void FSViewPart::completedSlot(int dirs)
{
    if (_job) {
        _job->progressSlot(100, dirs, QString());
        delete _job;
        _job = nullptr;
    }

    KConfigGroup cconfig = _view->config()->group("MetricCache");
    _view->saveMetric(&cconfig);

    emit completed();
}

void FSViewPart::slotShowVisMenu()
{
    _visMenu->menu()->clear();
    _view->addVisualizationItems(_visMenu->menu(), 1301);
}

void FSViewPart::slotShowAreaMenu()
{
    _areaMenu->menu()->clear();
    _view->addAreaStopItems(_areaMenu->menu(), 1001, nullptr);
}

void FSViewPart::slotShowDepthMenu()
{
    _depthMenu->menu()->clear();
    _view->addDepthStopItems(_depthMenu->menu(), 1501, nullptr);
}

void FSViewPart::slotShowColorMenu()
{
    _colorMenu->menu()->clear();
    _view->addColorItems(_colorMenu->menu(), 1401);
}

bool FSViewPart::openFile() // never called since openUrl is reimplemented
{
    qCDebug(FSVIEWLOG) << localFilePath();
    _view->setPath(localFilePath());

    return true;
}

bool FSViewPart::openUrl(const QUrl &url)
{
    qCDebug(FSVIEWLOG) << url.path();

    if (!url.isValid()) {
        return false;
    }
    if (!url.isLocalFile()) {
        return false;
    }

    setUrl(url);
    emit setWindowCaption(this->url().toDisplayString(QUrl::PreferLocalFile));

    _view->setPath(this->url().path());

    return true;
}

bool FSViewPart::closeUrl()
{
    qCDebug(FSVIEWLOG);

    _view->stop();

    return true;
}

void FSViewPart::setNonStandardActionEnabled(const char *actionName, bool enabled)
{
    QAction *action = actionCollection()->action(actionName);
    action->setEnabled(enabled);
}

void FSViewPart::updateActions()
{
    int canDel = 0, canCopy = 0, canMove = 0;

    const auto selectedItems = _view->selection();
    for (TreeMapItem *item : selectedItems) {
        Inode *inode = static_cast<Inode *>(item);
        const QUrl u = QUrl::fromLocalFile(inode->path());
        canCopy++;
        if (KProtocolManager::supportsDeleting(u)) {
            canDel++;
        }
        if (KProtocolManager::supportsMoving(u)) {
            canMove++;
        }
    }

    // Standard KBrowserExtension actions.
    emit _ext->enableAction("copy", canCopy > 0);
    emit _ext->enableAction("cut", canMove > 0);
    // Custom actions.
    //setNonStandardActionEnabled("rename", canMove > 0 ); // FIXME
    setNonStandardActionEnabled("move_to_trash", (canDel > 0 && canMove > 0));
    setNonStandardActionEnabled("delete", canDel > 0);
    setNonStandardActionEnabled("editMimeType", _view->selection().count() == 1);
    setNonStandardActionEnabled("properties", _view->selection().count() == 1);

    const KFileItemList items = selectedFileItems();
    emit _ext->selectionInfo(items);

    if (canCopy > 0) {
        stateChanged(QStringLiteral("has_selection"));
    } else {
        stateChanged(QStringLiteral("has_no_selection"));
    }

    qCDebug(FSVIEWLOG) << "deletable" << canDel;
}

KFileItemList FSViewPart::selectedFileItems() const
{
    const auto selectedItems = _view->selection();
    KFileItemList items;
    items.reserve(selectedItems.count());
    for (TreeMapItem *item : selectedItems) {
        Inode *inode = static_cast<Inode *>(item);
        const QUrl u = QUrl::fromLocalFile(inode->path());
        const QString mimetype = inode->mimeType().name();
        const QFileInfo &info = inode->fileInfo();
        mode_t mode =
            info.isFile() ? S_IFREG :
            info.isDir() ? S_IFDIR :
            info.isSymLink() ? S_IFLNK : (mode_t) - 1;
        items.append(KFileItem(u, mimetype, mode));
     }
     return items;
}

void FSViewPart::contextMenu(TreeMapItem * /*item*/, const QPoint &p)
{
    int canDel = 0, canCopy = 0, canMove = 0;

    const auto selectedItems = _view->selection();
    for (TreeMapItem *item : selectedItems) {
        Inode *inode = static_cast<Inode *>(item);
        const QUrl u = QUrl::fromLocalFile(inode->path());

        canCopy++;
        if (KProtocolManager::supportsDeleting(u)) {
            canDel++;
        }
        if (KProtocolManager::supportsMoving(u)) {
            canMove++;
        }
    }

    QList<QAction *> editActions;
    KParts::BrowserExtension::ActionGroupMap actionGroups;
    KParts::BrowserExtension::PopupFlags flags = KParts::BrowserExtension::ShowUrlOperations |
            KParts::BrowserExtension::ShowProperties;

    bool addTrash = (canMove > 0);
    bool addDel = false;
    if (canDel == 0) {
        flags |= KParts::BrowserExtension::NoDeletion;
    } else {
        if (!url().isLocalFile()) {
            addDel = true;
        } else if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
            addTrash = false;
            addDel = true;
        } else {
            KSharedConfig::Ptr globalConfig = KSharedConfig::openConfig(QStringLiteral("kdeglobals"), KConfig::IncludeGlobals);
            KConfigGroup configGroup(globalConfig, "KDE");
            addDel = configGroup.readEntry("ShowDeleteCommand", false);
        }
    }

    if (addTrash) {
        editActions.append(actionCollection()->action(QStringLiteral("move_to_trash")));
    }
    if (addDel) {
        editActions.append(actionCollection()->action(QStringLiteral("delete")));
    }

// FIXME: rename is currently unavailable. Requires popup renaming.
//     if (canMove)
//       editActions.append(actionCollection()->action("rename"));
    actionGroups.insert(QStringLiteral("editactions"), editActions);

    const KFileItemList items = selectedFileItems();
    if (items.count() > 0)
        emit _ext->popupMenu(_view->mapToGlobal(p), items,
                             KParts::OpenUrlArguments(),
                             KParts::BrowserArguments(),
                             flags,
                             actionGroups);
}

void FSViewPart::slotProperties()
{
    QList<QUrl> urlList;
    if (view()) {
        urlList = view()->selectedUrls();
    }

    if (!urlList.isEmpty()) {
        KPropertiesDialog::showDialog(urlList.first(), view());
    }
}

// FSViewBrowserExtension

FSViewBrowserExtension::FSViewBrowserExtension(FSViewPart *viewPart)
    : KParts::BrowserExtension(viewPart)
{
    _view = viewPart->view();
}

FSViewBrowserExtension::~FSViewBrowserExtension()
{}

void FSViewBrowserExtension::del()
{
    const QList<QUrl> urls = _view->selectedUrls();
    KJobUiDelegate* baseUiDelegate = KIO::createDefaultJobUiDelegate(KJobUiDelegate::Flags{}, _view);
    KIO::JobUiDelegate* uiDelegate = qobject_cast<KIO::JobUiDelegate*>(baseUiDelegate);
    uiDelegate->setWindow(_view);
    if (uiDelegate && uiDelegate->askDeleteConfirmation(urls,
                                         KIO::JobUiDelegate::Delete, KIO::JobUiDelegate::DefaultConfirmation)) {
        KIO::Job *job = KIO::del(urls);
        KJobWidgets::setWindow(job, _view);
        job->uiDelegate()->setAutoErrorHandlingEnabled(true);
        connect(job, &KJob::result, this, &FSViewBrowserExtension::refresh);
    }
}

void FSViewBrowserExtension::trash()
{
    bool deleteNotTrash = ((QGuiApplication::keyboardModifiers() & Qt::ShiftModifier) != 0);
    if (deleteNotTrash) {
        del();
    } else {
        KJobUiDelegate* baseUiDelegate = KIO::createDefaultJobUiDelegate(KJobUiDelegate::Flags{}, _view);
        KIO::JobUiDelegate* uiDelegate = qobject_cast<KIO::JobUiDelegate*>(baseUiDelegate);
        uiDelegate->setWindow(_view);
        const QList<QUrl> urls = _view->selectedUrls();
        if (uiDelegate && uiDelegate->askDeleteConfirmation(urls,
                                             KIO::JobUiDelegate::Trash, KIO::JobUiDelegate::DefaultConfirmation)) {
            KIO::Job *job = KIO::trash(urls);
            KIO::FileUndoManager::self()->recordJob(KIO::FileUndoManager::Trash, urls, QUrl("trash:/"), job);
            KJobWidgets::setWindow(job, _view);
            job->uiDelegate()->setAutoErrorHandlingEnabled(true);
            connect(job, &KJob::result, this, &FSViewBrowserExtension::refresh);
        }
    }
}

void FSViewBrowserExtension::copySelection(bool move)
{
    QMimeData *data = new QMimeData;
    data->setUrls(_view->selectedUrls());
    KIO::setClipboardDataCut(data, move);
    QApplication::clipboard()->setMimeData(data);
}

void FSViewBrowserExtension::editMimeType()
{
    Inode *i = (Inode *) _view->selection().first();
    if (i) {
        KMimeTypeEditor::editMimeType(i->mimeType().name(), _view);
    }
}

// refresh treemap at end of KIO jobs
void FSViewBrowserExtension::refresh()
{
    // only need to refresh common ancestor for all selected items
    TreeMapItem *commonParent = _view->selection().commonParent();
    if (!commonParent) {
        return;
    }

    /* if commonParent is a file, update parent directory */
    if (!((Inode *)commonParent)->isDir()) {
        commonParent = commonParent->parent();
        if (!commonParent) {
            return;
        }
    }

    qCDebug(FSVIEWLOG) << "refreshing"
                       << ((Inode *)commonParent)->path();

    _view->requestUpdate((Inode *)commonParent);
}

void FSViewBrowserExtension::itemSingleClicked(TreeMapItem *i)
{
    if (_view->style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick)) {
        selected(i);
    }
}


void FSViewBrowserExtension::itemDoubleClicked(TreeMapItem *i)
{
    if (!_view->style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick)) {
        selected(i);
    }
}

void FSViewBrowserExtension::selected(TreeMapItem *i)
{
    if (!i) {
        return;
    }

    QUrl url = QUrl::fromLocalFile(((Inode *)i)->path());
    emit openUrlRequest(url);
}

#include "fsview_part.moc"
