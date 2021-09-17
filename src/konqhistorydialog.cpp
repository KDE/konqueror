/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2009 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqhistorydialog.h"
#include "konqhistoryview.h"

#include "konqhistory.h"
#include "konqmainwindow.h"
#include "konqmainwindowfactory.h"
#include "konqurl.h"

#include <QAction>
#include <QTimer>
#include <QMenu>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QModelIndex>
#include <QTreeView>

#include <KWindowConfig>
#include <kactioncollection.h>
#include <kguiitem.h>
#include <QIcon>
#include <KLocalizedString>
#include <klineedit.h>
#include <ktoggleaction.h>
#include <KSharedConfig>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>

KonqHistoryDialog::KonqHistoryDialog(KonqMainWindow *parent)
    : QDialog(parent), m_mainWindow(parent), m_settings(KonqHistorySettings::self())
{
    setWindowTitle(i18nc("@title:window", "History"));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_historyView = new KonqHistoryView(this);
    connect(m_historyView->treeView(), &QTreeView::activated, this, &KonqHistoryDialog::slotOpenIndex);
    connect(m_historyView, &KonqHistoryView::openUrlInNewWindow, this, &KonqHistoryDialog::slotOpenWindow);
    connect(m_historyView, &KonqHistoryView::openUrlInNewTab, this, &KonqHistoryDialog::slotOpenTab);
    connect(m_historyView, &KonqHistoryView::openUrlInCurrentTab, this, &KonqHistoryDialog::slotOpenCurrentTab);
    connect(m_settings, &KonqHistorySettings::settingsChanged, this, &KonqHistoryDialog::reparseConfiguration);

    KActionCollection *collection = m_historyView->actionCollection();

    QToolBar *toolBar = new QToolBar(this);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    QToolButton *sortButton = new QToolButton(toolBar);
    sortButton->setText(i18nc("@action:inmenu Parent of 'By Name' and 'By Date'", "Sort"));
    sortButton->setIcon(QIcon::fromTheme(QStringLiteral("view-sort-ascending")));
    sortButton->setPopupMode(QToolButton::InstantPopup);
    sortButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    QMenu *sortMenu = new QMenu(sortButton);
    sortMenu->addAction(collection->action(QStringLiteral("byName")));
    sortMenu->addAction(collection->action(QStringLiteral("byDate")));
    sortButton->setMenu(sortMenu);
    toolBar->addWidget(sortButton);
    toolBar->addSeparator();
    toolBar->addAction(collection->action(QStringLiteral("preferences")));

    mainLayout->addWidget(toolBar);
    mainLayout->addWidget(m_historyView);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    mainLayout->addWidget(buttonBox);

    create(); // required by windowHandle()
    KWindowConfig::restoreWindowSize(windowHandle(), KSharedConfig::openConfig()->group("History Dialog"));

    reparseConfiguration();

    // give focus to the search line edit when opening the dialog (#240513)
    m_historyView->lineEdit()->setFocus();
}

KonqHistoryDialog::~KonqHistoryDialog()
{
    KConfigGroup group(KSharedConfig::openConfig(), "History Dialog");
    KWindowConfig::saveWindowSize(windowHandle(), group);
}

QSize KonqHistoryDialog::sizeHint() const
{
    return QSize(500, 400);
}

void KonqHistoryDialog::slotOpenWindow(const QUrl &url)
{
    KonqMainWindow *mw = KonqMainWindowFactory::createNewWindow(url);
    mw->show();
}

void KonqHistoryDialog::slotOpenTab(const QUrl &url)
{
    m_mainWindow->openMultiURL(QList<QUrl>() << url);
}

void KonqHistoryDialog::slotOpenCurrentTab(const QUrl& url)
{
    m_mainWindow->openFilteredUrl(url.toString());
}

void KonqHistoryDialog::slotOpenCurrentOrNewTab(const QUrl& url)
{
    QUrl currentUrl(m_mainWindow->currentURL());
    if (KonqUrl::hasKonqScheme(currentUrl) || currentUrl.isEmpty()) {
        slotOpenCurrentTab(url);
    } else {
        slotOpenTab(url);
    }
}

// Called when activating a row
void KonqHistoryDialog::slotOpenIndex(const QModelIndex &index)
{
    const QUrl url = m_historyView->urlForIndex(index);
    if (!url.isValid()) {
        return;
    }
    switch (m_defaultAction) {
        case KonqHistorySettings::Action::Auto:
            slotOpenCurrentOrNewTab(url);
            break;
        case KonqHistorySettings::Action::OpenNewTab:
            slotOpenTab(url);
            break;
        case KonqHistorySettings::Action::OpenCurrentTab:
            slotOpenCurrentTab(url);
            break;
        case KonqHistorySettings::Action::OpenNewWindow:
            slotOpenWindow(url);
            break;
    }
}

void KonqHistoryDialog::reparseConfiguration()
{
    m_defaultAction = m_settings->m_defaultAction;
}
