/* This file is part of the KDE project
   Copyright 2009 Pino Toscano <pino@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konqhistorydialog.h"

#include "konqhistory.h"
#include "konqhistorymanager.h"
#include "konqhistorymodel.h"
#include "konqhistoryproxymodel.h"
#include "konqhistorysettings.h"
#include "konqmisc.h"

#include <QtCore/QTimer>
#include <QtGui/QMenu>
#include <QtGui/QToolBar>
#include <QtGui/QToolButton>
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>

#include <kaction.h>
#include <kactioncollection.h>
#include <kguiitem.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <krun.h>
#include <ktoggleaction.h>

K_GLOBAL_STATIC_WITH_ARGS(KonqHistorySettings, s_settings, (0))

KonqHistoryDialog::KonqHistoryDialog(QWidget *parent)
    : KDialog(parent)
    , m_searchTimer(0)
{
    setCaption(i18n("History"));
    setButtons(KDialog::Close);

    if (!s_settings.exists()) {
        s_settings->readSettings(true);
    }

    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget());
    mainLayout->setMargin(0);

    m_treeView = new QTreeView(mainWidget());
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setHeaderHidden(true);
    m_historyProxyModel = new KonqHistoryProxyModel(s_settings, m_treeView);
    connect(m_treeView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotContextMenu(QPoint)));
    m_historyProxyModel->setDynamicSortFilter(true);
    m_historyProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_historyModel = new KonqHistoryModel(m_historyProxyModel);
    m_treeView->setModel(m_historyProxyModel);
    m_historyProxyModel->setSourceModel(m_historyModel);
    m_treeView->model()->sort(0);

    m_collection = new KActionCollection(this);
    QAction *action = m_collection->addAction("open_new");
    action->setIcon(KIcon("window-new"));
    action->setText(i18n("New &Window"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(slotNewWindow()));
    action = m_collection->addAction("remove");
    action->setIcon(KIcon("edit-delete"));
    action->setText(i18n("&Remove Entry"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(slotRemoveEntry()));
    action = m_collection->addAction("clear");
    action->setIcon(KIcon("edit-clear-history"));
    action->setText(i18n("C&lear History"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(slotClearHistory()));
    action = m_collection->addAction("preferences");
    action->setIcon(KIcon("configure"));
    action->setText(i18n("&Preferences..."));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(slotPreferences()));

    QActionGroup* sortGroup = new QActionGroup(this);
    sortGroup->setExclusive(true);

    action = m_collection->addAction("byName");
    action->setText(i18n("By &Name"));
    action->setCheckable(true);
    action->setData(qVariantFromValue(0));
    sortGroup->addAction(action);

    action = m_collection->addAction("byDate");
    action->setText(i18n("By &Date"));
    action->setCheckable(true);
    action->setData(qVariantFromValue(1));
    sortGroup->addAction(action);

    sortGroup->actions().at(s_settings->m_sortsByName ? 0 : 1)->setChecked(true);
    connect(sortGroup, SIGNAL(triggered(QAction *)), this, SLOT(slotSortChange(QAction *)));

    QToolBar *toolBar = new QToolBar(mainWidget());
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    QToolButton *sortButton = new QToolButton(toolBar);
    sortButton->setText(i18nc("@action:inmenu Parent of 'By Name' and 'By Date'", "Sort"));
    sortButton->setIcon(KIcon("view-sort-ascending"));
    sortButton->setPopupMode(QToolButton::InstantPopup);
    sortButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    QMenu *sortMenu = new QMenu(sortButton);
    sortMenu->addAction(m_collection->action("byName"));
    sortMenu->addAction(m_collection->action("byDate"));
    sortButton->setMenu(sortMenu);
    toolBar->addWidget(sortButton);
    toolBar->addSeparator();
    toolBar->addAction(m_collection->action("preferences"));

    m_searchLineEdit = new KLineEdit(mainWidget());
    m_searchLineEdit->setClickMessage(i18n("Search in history"));
    m_searchLineEdit->setClearButtonShown(true);

    connect(m_searchLineEdit, SIGNAL(textChanged(QString)), this, SLOT(slotFilterTextChanged(QString)));

    mainLayout->addWidget(toolBar);
    mainLayout->addWidget(m_searchLineEdit);
    mainLayout->addWidget(m_treeView);

    restoreDialogSize(KGlobal::config()->group("History Dialog"));
}

KonqHistoryDialog::~KonqHistoryDialog()
{
    KConfigGroup group(KGlobal::config(), "History Dialog");
    saveDialogSize(group);
}

QSize KonqHistoryDialog::sizeHint() const
{
    return QSize(500, 400);
}

void KonqHistoryDialog::slotContextMenu(const QPoint &pos)
{
    const QModelIndex index = m_treeView->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    const int nodeType = index.data(KonqHistory::TypeRole).toInt();

    QMenu *menu = new QMenu(this);

    if (nodeType == KonqHistory::HistoryType) {
        menu->addAction(m_collection->action("open_new"));
        menu->addSeparator();
    }

    menu->addAction(m_collection->action("remove"));
    menu->addAction(m_collection->action("clear"));
    menu->addSeparator();
    QMenu *sortMenu = menu->addMenu(i18nc("@action:inmenu Parent of 'By Name' and 'By Date'", "Sort"));
    sortMenu->addAction(m_collection->action("byName"));
    sortMenu->addAction(m_collection->action("byDate"));
    menu->addSeparator();
    menu->addAction(m_collection->action("preferences"));

    menu->exec(m_treeView->viewport()->mapToGlobal(pos));

    delete menu;
}

void KonqHistoryDialog::slotNewWindow()
{
    const QModelIndex index = m_treeView->currentIndex();
    if (!index.isValid() || (index.data(KonqHistory::TypeRole).toInt() != KonqHistory::HistoryType)) {
        return;
    }

    const KUrl url = index.data(KonqHistory::UrlRole).value<KUrl>();
    if (!url.isValid()) {
        return;
    }

    KonqMisc::createNewWindow(url);
}

void KonqHistoryDialog::slotRemoveEntry()
{
    const QModelIndex index = m_treeView->currentIndex();
    if (!index.isValid()) {
        return;
    }

    m_historyModel->deleteItem(m_historyProxyModel->mapToSource(index));
}

void KonqHistoryDialog::slotClearHistory()
{
    KGuiItem guiitem = KStandardGuiItem::clear();
    guiitem.setIcon(KIcon("edit-clear-history"));

    if (KMessageBox::warningContinueCancel(this,
            i18n("Do you really want to clear the entire history?"),
            i18n("Clear History?"), guiitem)
        == KMessageBox::Continue) {
        KonqHistoryManager::kself()->emitClear();
    }
}

void KonqHistoryDialog::slotPreferences()
{
    // Run the history sidebar settings.
    KRun::run("kcmshell4 kcmhistory", KUrl::List(), this);
}

void KonqHistoryDialog::slotSortChange(QAction *action)
{
    if (!action) {
        return;
    }

    const int which = action->data().toInt();
    switch (which) {
    case 0:
        s_settings->m_sortsByName = true;
        break;
    case 1:
        s_settings->m_sortsByName = false;
        break;
    }
    s_settings->applySettings();
}

void KonqHistoryDialog::slotFilterTextChanged(const QString &text)
{
    Q_UNUSED(text);
    if (!m_searchTimer) {
        m_searchTimer = new QTimer(this);
        m_searchTimer->setSingleShot(true);
        connect(m_searchTimer, SIGNAL(timeout()), this, SLOT(slotTimerTimeout()));
    }
    m_searchTimer->start(600);
}

void KonqHistoryDialog::slotTimerTimeout()
{
    m_historyProxyModel->setFilterFixedString(m_searchLineEdit->text());
}

#include "konqhistorydialog.moc"
