/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2009 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2009 Daivd Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqhistoryview.h"
#include "konqhistory.h"
#include "konq_historyprovider.h"
#include "konqhistorymodel.h"
#include "konqhistoryproxymodel.h"
#include "konqhistorysettings.h"

#include <KDialogJobUiDelegate>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QIcon>
#include <QLineEdit>
#include <QMenu>
#include <QMimeData>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>
#include <QActionGroup>

#include <kactioncollection.h>
#include "konqdebug.h"
#include <KLocalizedString>
#include <kmessagebox.h>

#include <KIO/CommandLauncherJob>

KonqHistoryView::KonqHistoryView(QWidget *parent)
    : QWidget(parent)
    , m_searchTimer(nullptr)
{
    m_treeView = new QTreeView(this);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setHeaderHidden(true);

    m_historyProxyModel = new KonqHistoryProxyModel(KonqHistorySettings::self(), m_treeView);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &KonqHistoryView::slotContextMenu);
    m_historyModel = new KonqHistoryModel(m_historyProxyModel);
    m_treeView->setModel(m_historyProxyModel);
    m_historyProxyModel->setSourceModel(m_historyModel);
    m_historyProxyModel->sort(0);

    m_collection = new KActionCollection(this);
    m_collection->addAssociatedWidget(m_treeView); // make shortcuts work
    QAction *action = m_collection->addAction(QStringLiteral("open_new"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("window-new")));
    action->setText(i18n("Open in New &Window"));
    connect(action, &QAction::triggered, this, &KonqHistoryView::slotNewWindow);

    action = m_collection->addAction(QStringLiteral("open_tab"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
    action->setText(i18n("Open in New Tab"));
    connect(action, &QAction::triggered, this, &KonqHistoryView::slotNewTab);

    action = m_collection->addAction(QStringLiteral("open_current_tab"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("window")));
    action->setText(i18n("Open in Current Tab"));
    connect(action, &QAction::triggered, this, &KonqHistoryView::slotCurrentTab);

    action = m_collection->addAction(QStringLiteral("copylinklocation"));
    action->setText(i18n("&Copy Link Address"));
    connect(action, &QAction::triggered, this, &KonqHistoryView::slotCopyLinkLocation);

    action = m_collection->addAction(QStringLiteral("remove"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    action->setText(i18n("&Remove Entry"));
    m_collection->setDefaultShortcut(action, QKeySequence(Qt::Key_Delete)); // #135966
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(action, &QAction::triggered, this, &KonqHistoryView::slotRemoveEntry);

    action = m_collection->addAction(QStringLiteral("clear"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear-history")));
    action->setText(i18n("C&lear History"));
    connect(action, &QAction::triggered, this, &KonqHistoryView::slotClearHistory);

    action = m_collection->addAction(QStringLiteral("preferences"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("configure")));
    action->setText(i18n("&Preferences..."));
    connect(action, &QAction::triggered, this, &KonqHistoryView::slotPreferences);

    QActionGroup *sortGroup = new QActionGroup(this);
    sortGroup->setExclusive(true);

    action = m_collection->addAction(QStringLiteral("byName"));
    action->setText(i18n("By &Name"));
    action->setCheckable(true);
    action->setData(QVariant::fromValue(0));
    sortGroup->addAction(action);

    action = m_collection->addAction(QStringLiteral("byDate"));
    action->setText(i18n("By &Date"));
    action->setCheckable(true);
    action->setData(QVariant::fromValue(1));
    sortGroup->addAction(action);

    KonqHistorySettings *settings = KonqHistorySettings::self();
    sortGroup->actions().at(settings->m_sortsByName ? 0 : 1)->setChecked(true);
    connect(sortGroup, &QActionGroup::triggered, this, &KonqHistoryView::slotSortChange);

    m_searchLineEdit = new QLineEdit(this);
    m_searchLineEdit->setPlaceholderText(i18n("Search in history"));
    m_searchLineEdit->setClearButtonEnabled(true);

    connect(m_searchLineEdit, &QLineEdit::textChanged, this, &KonqHistoryView::slotFilterTextChanged);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_searchLineEdit);
    mainLayout->addWidget(m_treeView);
}

void KonqHistoryView::slotContextMenu(const QPoint &pos)
{
    const QModelIndex index = m_treeView->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    const int nodeType = index.data(KonqHistory::TypeRole).toInt();

    QMenu *menu = new QMenu(this);

    if (nodeType == KonqHistory::HistoryType) {
        menu->addAction(m_collection->action(QStringLiteral("open_new")));
        menu->addAction(m_collection->action(QStringLiteral("open_tab")));
        menu->addAction(m_collection->action(QStringLiteral("open_current_tab")));
        menu->addAction(m_collection->action(QStringLiteral("copylinklocation")));
        menu->addSeparator();
    }

    menu->addAction(m_collection->action(QStringLiteral("remove")));
    menu->addAction(m_collection->action(QStringLiteral("clear")));
    menu->addSeparator();
    QMenu *sortMenu = menu->addMenu(i18nc("@action:inmenu Parent of 'By Name' and 'By Date'", "Sort"));
    sortMenu->addAction(m_collection->action(QStringLiteral("byName")));
    sortMenu->addAction(m_collection->action(QStringLiteral("byDate")));
    menu->addSeparator();
    menu->addAction(m_collection->action(QStringLiteral("preferences")));

    menu->exec(m_treeView->viewport()->mapToGlobal(pos));

    delete menu;
}

void KonqHistoryView::slotRemoveEntry()
{
    const QModelIndex index = m_treeView->currentIndex();
    if (!index.isValid()) {
        return;
    }

    // TODO undo/redo support
    m_historyModel->deleteItem(m_historyProxyModel->mapToSource(index));
}

void KonqHistoryView::slotClearHistory()
{
    KGuiItem guiitem = KStandardGuiItem::clear();
    guiitem.setIcon(QIcon::fromTheme(QStringLiteral("edit-clear-history")));

    if (KMessageBox::warningContinueCancel(this,
                                           i18n("Do you really want to clear the entire history?"),
                                           i18nc("@title:window", "Clear History?"), guiitem)
            == KMessageBox::Continue) {
        KonqHistoryProvider::self()->emitClear();
    }
}

void KonqHistoryView::slotPreferences()
{
    // Run the history sidebar settings.
    KIO::CommandLauncherJob *job = new KIO::CommandLauncherJob(QStringLiteral("kcmshell%1 kcmhistory").arg(KI18N_VERSION_MAJOR));
    job->setUiDelegate(new KDialogJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
    job->start();
}

void KonqHistoryView::slotSortChange(QAction *action)
{
    if (!action) {
        return;
    }

    const int which = action->data().toInt();
    KonqHistorySettings *settings = KonqHistorySettings::self();
    settings->m_sortsByName = (which == 0);
    settings->applySettings();
}

void KonqHistoryView::slotFilterTextChanged(const QString &text)
{
    Q_UNUSED(text);
    if (!m_searchTimer) {
        m_searchTimer = new QTimer(this);
        m_searchTimer->setSingleShot(true);
        connect(m_searchTimer, &QTimer::timeout, this, &KonqHistoryView::slotTimerTimeout);
    }
    m_searchTimer->start(600);
}

void KonqHistoryView::slotTimerTimeout()
{
    m_historyProxyModel->setFilterFixedString(m_searchLineEdit->text());
}

QTreeView *KonqHistoryView::treeView() const
{
    return m_treeView;
}

QLineEdit *KonqHistoryView::lineEdit() const
{
    return m_searchLineEdit;
}

void KonqHistoryView::slotNewWindow()
{
    const QUrl url = urlForIndex(m_treeView->currentIndex());
    if (url.isValid()) {
        emit openUrlInNewWindow(url);
    }
}

void KonqHistoryView::slotNewTab()
{
    const QUrl url = urlForIndex(m_treeView->currentIndex());
    if (url.isValid()) {
        emit openUrlInNewTab(url);
    }
}

void KonqHistoryView::slotCurrentTab()
{
    const QUrl url = urlForIndex(m_treeView->currentIndex());
    if (url.isValid()) {
        emit openUrlInCurrentTab(url);
    }
}

QUrl KonqHistoryView::urlForIndex(const QModelIndex &index) const
{
    if (!index.isValid() || (index.data(KonqHistory::TypeRole).toInt() != KonqHistory::HistoryType)) {
        return QUrl();
    }

    return index.data(KonqHistory::UrlRole).toUrl();
}

// Code taken from KHTMLPopupGUIClient::slotCopyLinkLocation
void KonqHistoryView::slotCopyLinkLocation()
{
    QUrl safeURL = urlForIndex(m_treeView->currentIndex()).adjusted(QUrl::RemovePassword);

    // Set it in both the mouse selection and in the clipboard
    QMimeData *mimeData = new QMimeData;
    mimeData->setUrls(QList<QUrl>() << safeURL);
    QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);
    QApplication::clipboard()->setMimeData(mimeData, QClipboard::Selection);
}
