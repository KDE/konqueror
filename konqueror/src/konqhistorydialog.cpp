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
#include "konqhistoryview.h"

#include "konqhistory.h"
#include "konqmainwindow.h"
#include "konqmisc.h"

#include <QtCore/QTimer>
#include <QMenu>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QModelIndex>
#include <QTreeView>

#include <kaction.h>
#include <kactioncollection.h>
#include <kguiitem.h>
#include <klocale.h>
#include <klineedit.h>
#include <ktoggleaction.h>

KonqHistoryDialog::KonqHistoryDialog(KonqMainWindow *parent)
    : KDialog(parent), m_mainWindow(parent)
{
    setCaption(i18nc("@title:window", "History"));
    setButtons(KDialog::Close);

    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget());
    mainLayout->setMargin(0);

    m_historyView = new KonqHistoryView(mainWidget());
    connect(m_historyView->treeView(), SIGNAL(doubleClicked(QModelIndex)), this, SLOT(slotOpenWindowForIndex(QModelIndex)));
    connect(m_historyView, SIGNAL(openUrlInNewWindow(KUrl)), this, SLOT(slotOpenWindow(KUrl)));
    connect(m_historyView, SIGNAL(openUrlInNewTab(KUrl)), this, SLOT(slotOpenTab(KUrl)));

    KActionCollection* collection = m_historyView->actionCollection();

    QToolBar *toolBar = new QToolBar(mainWidget());
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    QToolButton *sortButton = new QToolButton(toolBar);
    sortButton->setText(i18nc("@action:inmenu Parent of 'By Name' and 'By Date'", "Sort"));
    sortButton->setIcon(KIcon("view-sort-ascending"));
    sortButton->setPopupMode(QToolButton::InstantPopup);
    sortButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    QMenu *sortMenu = new QMenu(sortButton);
    sortMenu->addAction(collection->action("byName"));
    sortMenu->addAction(collection->action("byDate"));
    sortButton->setMenu(sortMenu);
    toolBar->addWidget(sortButton);
    toolBar->addSeparator();
    toolBar->addAction(collection->action("preferences"));

    mainLayout->addWidget(toolBar);
    mainLayout->addWidget(m_historyView);

    restoreDialogSize(KGlobal::config()->group("History Dialog"));

    // give focus to the search line edit when opening the dialog (#240513)
    m_historyView->lineEdit()->setFocus();
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

void KonqHistoryDialog::slotOpenWindow(const KUrl& url)
{
    KonqMainWindow* mw = KonqMisc::createNewWindow(url);
    mw->show();
}

void KonqHistoryDialog::slotOpenTab(const KUrl& url)
{
    m_mainWindow->openMultiURL(KUrl::List() << url);
}

// Called when double-clicking on a row
void KonqHistoryDialog::slotOpenWindowForIndex(const QModelIndex& index)
{
    const KUrl url = m_historyView->urlForIndex(index);
    if (url.isValid()) {
        slotOpenWindow(url); // should we call slotOpenTab instead?
    }
}

#include "konqhistorydialog.moc"
