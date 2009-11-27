/* This file is part of the KDE project
   Copyright 2009 Pino Toscano <pino@kde.org>
   Copyright 2009 Daivd Faure <faure@kde.org>

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

#include "konqhistoryview.h"
#include "konqhistory.h"
#include "konq_historyprovider.h"
#include "konqhistorymodel.h"
#include "konqhistoryproxymodel.h"
#include "konqhistorysettings.h"

#include <QClipboard>
#include <QApplication>
#include <QMenu>
#include <QTreeView>
#include <QVBoxLayout>

#include <kactioncollection.h>
#include <kaction.h>
#include <kdebug.h>
#include <kicon.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <krun.h>

KonqHistoryView::KonqHistoryView(QWidget* parent)
    : QWidget(parent)
    , m_searchTimer(0)
{
    m_treeView = new QTreeView(this);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setHeaderHidden(true);

    m_historyProxyModel = new KonqHistoryProxyModel(KonqHistorySettings::self(), m_treeView);
    connect(m_treeView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotContextMenu(QPoint)));
    m_historyModel = new KonqHistoryModel(m_historyProxyModel);
    m_treeView->setModel(m_historyProxyModel);
    m_historyProxyModel->setSourceModel(m_historyModel);
    m_historyProxyModel->sort(0);

    m_collection = new KActionCollection(this);
    m_collection->addAssociatedWidget(m_treeView); // make shortcuts work
    QAction *action = m_collection->addAction("open_new");
    action->setIcon(KIcon("window-new"));
    action->setText(i18n("Open in New &Window"));
    connect(action, SIGNAL(triggered()), this, SLOT(slotNewWindow()));

    action = m_collection->addAction("open_tab");
    action->setIcon(KIcon("tab-new"));
    action->setText(i18n("Open in New Tab"));
    connect(action, SIGNAL(triggered()), this, SLOT(slotNewTab()));

    action = m_collection->addAction("copylinklocation");
    action->setText(i18n("&Copy Link Address"));
    connect(action, SIGNAL(triggered()), this, SLOT(slotCopyLinkLocation()));

    action = m_collection->addAction("remove");
    action->setIcon(KIcon("edit-delete"));
    action->setText(i18n("&Remove Entry"));
    action->setShortcut(Qt::Key_Delete); // #135966
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
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

    KonqHistorySettings* settings = KonqHistorySettings::self();
    sortGroup->actions().at(settings->m_sortsByName ? 0 : 1)->setChecked(true);
    connect(sortGroup, SIGNAL(triggered(QAction *)), this, SLOT(slotSortChange(QAction *)));

    m_searchLineEdit = new KLineEdit(this);
    m_searchLineEdit->setClickMessage(i18n("Search in history"));
    m_searchLineEdit->setClearButtonShown(true);

    connect(m_searchLineEdit, SIGNAL(textChanged(QString)), this, SLOT(slotFilterTextChanged(QString)));

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(0);
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
        menu->addAction(m_collection->action("open_new"));
        menu->addAction(m_collection->action("open_tab"));
        menu->addAction(m_collection->action("copylinklocation"));
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
    guiitem.setIcon(KIcon("edit-clear-history"));

    if (KMessageBox::warningContinueCancel(this,
            i18n("Do you really want to clear the entire history?"),
            i18n("Clear History?"), guiitem)
        == KMessageBox::Continue) {
        KonqHistoryProvider::self()->emitClear();
    }
}

void KonqHistoryView::slotPreferences()
{
    // Run the history sidebar settings.
    KRun::run("kcmshell4 kcmhistory", KUrl::List(), this);
}

void KonqHistoryView::slotSortChange(QAction *action)
{
    if (!action) {
        return;
    }

    const int which = action->data().toInt();
    KonqHistorySettings* settings = KonqHistorySettings::self();
    settings->m_sortsByName = (which == 0);
    settings->applySettings();
}

void KonqHistoryView::slotFilterTextChanged(const QString &text)
{
    Q_UNUSED(text);
    if (!m_searchTimer) {
        m_searchTimer = new QTimer(this);
        m_searchTimer->setSingleShot(true);
        connect(m_searchTimer, SIGNAL(timeout()), this, SLOT(slotTimerTimeout()));
    }
    m_searchTimer->start(600);
}

void KonqHistoryView::slotTimerTimeout()
{
    m_historyProxyModel->setFilterFixedString(m_searchLineEdit->text());
}

QTreeView* KonqHistoryView::treeView() const
{
    return m_treeView;
}

void KonqHistoryView::slotNewWindow()
{
    const KUrl url = urlForIndex(m_treeView->currentIndex());
    if (url.isValid())
        emit openUrlInNewWindow(url);
}

void KonqHistoryView::slotNewTab()
{
    const KUrl url = urlForIndex(m_treeView->currentIndex());
    if (url.isValid())
        emit openUrlInNewTab(url);
}

KUrl KonqHistoryView::urlForIndex(const QModelIndex& index) const
{
    if (!index.isValid() || (index.data(KonqHistory::TypeRole).toInt() != KonqHistory::HistoryType)) {
        return KUrl();
    }

    return index.data(KonqHistory::UrlRole).value<KUrl>();
}

// Code taken from KHTMLPopupGUIClient::slotCopyLinkLocation
void KonqHistoryView::slotCopyLinkLocation()
{
    KUrl safeURL = urlForIndex(m_treeView->currentIndex());
    safeURL.setPass(QString());

    // Set it in both the mouse selection and in the clipboard
    QMimeData* mimeData = new QMimeData;
    safeURL.populateMimeData( mimeData );
    QApplication::clipboard()->setMimeData( mimeData, QClipboard::Clipboard );

    mimeData = new QMimeData;
    safeURL.populateMimeData( mimeData );
    QApplication::clipboard()->setMimeData( mimeData, QClipboard::Selection );
}

#include "konqhistoryview.moc"
