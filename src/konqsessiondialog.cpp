/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2008 Eduardo Robles Elvira <edulix@gmail.com>
    SPDX-FileCopyrightText: 2025 Raphael Rosch <kde-dev@insaner.com>
    SPDX-FileCopyrightText: 2025 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqsessiondialog.h"
#include "konqmisc.h"
#include "konqmainwindow.h"
#include "konqviewmanager.h"
#include "konqsettings.h"
#include "konqsessionmanager.h"

#include "konqdebug.h"
#include <KLocalizedString>
#include <KWindowInfo>
#include <KX11Extras>
#include <KSharedConfig>

#include <QUrl>
#include <QIcon>
#include <ksqueezedtextlabel.h>

#include <QPushButton>
#include <QCheckBox>
#include <QFileInfo>
#include <QtAlgorithms>
#include <QDirIterator>
#include <QDir>
#include <QFile>
#include <QSize>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QScrollBar>
#include <QStandardPaths>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <KGuiItem>
#include <QScreen>
#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QMessageBox>

SessionTreeWidget::SessionTreeWidget(QWidget* parent) : QTreeWidget(parent), m_fontMetrics(font()),
    m_genericToolTip{i18nc("@tooltip:session list", "Uncheck the sessions or windows you do not want to be restored")}
{
    setHeaderHidden(true);
    setContextMenuPolicy(Qt::CustomContextMenu);  // enable right-click
    setSelectionMode(QTreeWidget::NoSelection);
}

SessionTreeWidget::~SessionTreeWidget() noexcept
{
}

QString SessionTreeWidget::fullViewId(const QString& sessionFile, const QString& windowId, const QString& viewId)
{
    return sessionFile + windowId + viewId;
}

void SessionTreeWidget::fill(const QStringList &sessionFilePaths)
{
    //Avoid calling slotItemChanged for each change made when filling the tree
    disconnect(this, &SessionTreeWidget::itemChanged, this, &SessionTreeWidget::slotItemChanged);
    m_minWidth = width();
    for (const QString &sessionFile: sessionFilePaths) {
        fillSession(sessionFile);
    }

    const int borderWidth = width() - viewport()->width() + verticalScrollBar()->height();
    m_minWidth += borderWidth;

    // limit the width to a fixed fraction (s_maxRelativeWidth) of screen width
    const QRect desktop = screen()->geometry();
    int maxAllowedWidth = qRound(desktop.width() * s_maxRelativeWidth);
    m_minWidth = std::min(m_minWidth, maxAllowedWidth);
    setMinimumWidth(m_minWidth);

    connect(this, &SessionTreeWidget::itemChanged, this, &SessionTreeWidget::slotItemChanged);
    resizeColumnToContents(0);
}

void SessionTreeWidget::fillSession(const QString& sessionFile)
{
    QFileInfo fileInfo(sessionFile);
    QString sessionName = fileInfo.fileName();
    qCDebug(KONQUEROR_LOG) << sessionFile;
    QRegularExpression trailingDigitsRE(R"(\d+$)");
    QTreeWidgetItem *sessionItem = new QTreeWidgetItem(this);
    sessionItem->setText(0, i18nc("@item:treewidget", "Session %1", sessionName));
    sessionItem->setToolTip(0, m_genericToolTip);
    sessionItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    sessionItem->setCheckState(0, Qt::Checked);
    sessionItem->setExpanded(true);

    KConfig config(sessionFile, KConfig::SimpleConfig);
    const QList<KConfigGroup> windowGroups = KonqSessionManager::windowConfigGroups(config);
    for (const KConfigGroup &windowGroup: windowGroups) {
        QTreeWidgetItem *windowItem = new QTreeWidgetItem(sessionItem);
        const QString windowId = windowGroup.name();
        // To avoid a recursive search, let's do linear search on Foo_CurrentHistoryItem=1
        for (const QString &key: windowGroup.keyList()) {
            createViewItem(windowItem, {sessionFile, windowId}, windowGroup, key);
        }
        if (windowItem->childCount() > 0) {
            QRegularExpressionMatch trailingDigitsMatch = trailingDigitsRE.match(windowId);
            // I assume this is done this way for the benefit of i18n. Otherwise it would be easier to just use what is in the session file
            windowItem->setText(0, i18nc("@item:treewidget", "Window %1", trailingDigitsMatch.captured(0).toInt())); // FIXME: what if for some reason this is broken and there is no match?
            windowItem->setToolTip(0, m_genericToolTip);
            windowItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
            windowItem->setData(0, IdRole, KonqSessionManager::fullWindowId(sessionFile, windowId));
            windowItem->setCheckState(0, Qt::Checked);
            windowItem->setExpanded(true);
            m_sessionItemsCount++;
            m_checkedSessionItems.insert(sessionItem, sessionItem->childCount());
        } else {
            sessionItem->removeChild(windowItem);
            delete windowItem;
        }
    }
}

QTreeWidgetItem* SessionTreeWidget::createViewItem(QTreeWidgetItem *windowItem, const WindowId &windowId, const KConfigGroup &windowGroup, const QString &key)
{
    if (!key.endsWith(QLatin1String("_CurrentHistoryItem"))) {
        return nullptr;
    }

    const QString viewId = key.left(key.length() - qstrlen("_CurrentHistoryItem"));
    const QString historyIndex = windowGroup.readEntry(key, QString());
    const QString prefix = "HistoryItem" + viewId + '_' + historyIndex;
    // Ignore the sidebar views
    if (windowGroup.readEntry(prefix + "StrServiceName", QString()).startsWith(QLatin1String("konq_sidebar"))) {
        return nullptr;
    }
    const QString url = windowGroup.readEntry(prefix + "Url", QString());
    const QString title = windowGroup.readEntry(prefix + "Title", QString());
    qCDebug(KONQUEROR_LOG) << viewId << url << title;
    const QString displayText = (title.trimmed().isEmpty() ? url : title);
    if (displayText.isEmpty()) {
        return nullptr;
    }
    QTreeWidgetItem *item = new QTreeWidgetItem(windowItem);
    item->setText(0, displayText);
    item->setToolTip(0, url);
    item->setData(0, IdRole, fullViewId(windowId.session, windowId.window, viewId));
    item->setData(0, UrlRole, url);    // "hidden" data to be pulled by the context menu handler
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    item->setIcon(0, QIcon::fromTheme("view-restore")); // could also be replaced with just a bullet point, or nothing

    m_minWidth = std::max(m_minWidth, m_fontMetrics.horizontalAdvance(displayText));
    return item;
}

QString SessionTreeWidget::url(QTreeWidgetItem* item)
{
    return item ? item->data(0, UrlRole).toString() : QString{};
}

QString SessionTreeWidget::viewId(QTreeWidgetItem* item, int column)
{
    return item ? item->data(column, IdRole).toString() : QString{};
}

QStringList SessionTreeWidget::discardedWindowList() const
{
    QStringList windows;
    for (int s = 0; s < invisibleRootItem()->childCount(); ++s) {
        QTreeWidgetItem *session = invisibleRootItem()->child(s);
        for (int w = 0; w < session->childCount(); ++w) {
            QTreeWidgetItem *window = session->child(w);
            if (window->checkState(0) == Qt::Unchecked) {
                windows << viewId(window);
            }
        }
    }
    return windows;
}

void SessionTreeWidget::updateCounts(QTreeWidgetItem* it, int column)
{
    Qt::CheckState state = it->checkState(column);
    QTreeWidgetItem *parentIt = it->parent();
    QString id = viewId(it, column);
    int increment = state == Qt::Checked ? 1 : -1;
    m_sessionItemsCount += increment;
    m_checkedSessionItems[parentIt] += increment;
}

SessionTreeWidget::ItemLevel SessionTreeWidget::itemLevel(QTreeWidgetItem* item)
{
    int level = 0;
    for (QTreeWidgetItem *tmpItem = item; tmpItem->parent();  tmpItem = tmpItem->parent()) {
        ++level;
    }
    return level <= ViewLevel ? static_cast<ItemLevel>(level) : ViewLevel;
}

void SessionTreeWidget::slotItemChanged(QTreeWidgetItem* item, int column)
{
    Q_ASSERT(item);

    const bool blocked = item->treeWidget()->blockSignals(true);

    ItemLevel level = itemLevel(item);
    const int itemChildCount = item->childCount();
    if (level < s_maxDisplayedLevel && itemChildCount > 0) { // toggle child items
        for (int i = 0; i < itemChildCount; ++i) {
            QTreeWidgetItem *childItem = item->child(i);
            if (childItem && childItem->checkState(column) != item->checkState(column)) {
                childItem->setCheckState(column, item->checkState(column));
                updateCounts(childItem, column);
            }
        }
    }

    if (level > SessionLevel) { // toggle parent item
        updateCounts(item, column);
    }

    QTreeWidgetItem *parentItem = level > SessionLevel ? item->parent() : item;

    const int numCheckSessions = m_checkedSessionItems.value(parentItem);
    Qt::CheckState state = parentItem->checkState(column);
    if (state == Qt::Checked && numCheckSessions == 0) {
        parentItem->setCheckState(column, Qt::Unchecked);
    } else if (state == Qt::Unchecked && numCheckSessions > 0) {
        parentItem->setCheckState(column, Qt::Checked);
    }

    item->treeWidget()->blockSignals(blocked);
    emit sessionCountChanged(m_sessionItemsCount);
}


SessionRestoreDialog::SessionRestoreDialog(const QStringList &sessionFilePaths, QWidget *parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("restoresession"));
    createUi();
    Q_ASSERT(!sessionFilePaths.isEmpty());
    // Collect info from the sessions to restore
    m_treeWidget->fill(sessionFilePaths);

    // Do not connect the sessionCountChanged signal until after the treewidget
    // is completely populated to prevent the firing of the itemChanged
    // signal while in the process of adding the original session items.
    connect(m_treeWidget, &SessionTreeWidget::sessionCountChanged, this, &SessionRestoreDialog::updateApplyButton);
}

SessionRestoreDialog::~SessionRestoreDialog()
{
}

void SessionRestoreDialog::createUi()
{
    setWindowTitle(i18nc("@title:window", "Restore Session?"));
    setModal(true);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *hLayout = new QHBoxLayout();
    hLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(hLayout, 5);

    QIcon icon = QIcon::fromTheme(QLatin1String("dialog-warning"));
    if (!icon.isNull()) {
        QLabel *iconLabel = new QLabel(this);
        iconLabel->setPixmap(icon.pixmap(style()->pixelMetric(QStyle::PM_MessageBoxIconSize)));
        QVBoxLayout *iconLayout = new QVBoxLayout();
        iconLayout->addStretch(1);
        iconLayout->addWidget(iconLabel);
        iconLayout->addStretch(5);
        hLayout->addLayout(iconLayout, 0);
        hLayout->addSpacing(style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing));
    }

    const QString text(i18n("Konqueror did not close correctly. Would you like to restore these previous sessions?"));
    QLabel *messageLabel = new QLabel(text, this);
    Qt::TextInteractionFlags flags = (Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    messageLabel->setTextInteractionFlags(flags);
    messageLabel->setWordWrap(true);

    hLayout->addWidget(messageLabel, 5);

    m_treeWidget = new SessionTreeWidget(this);
        // handle right-click context menu
    connect(m_treeWidget, &QTreeWidget::customContextMenuRequested, this, &SessionRestoreDialog::showContextMenu);

    mainLayout->addWidget(m_treeWidget, 50);
    messageLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    QCheckBox *checkbox = new QCheckBox(i18n("Do not ask again"), this);
    connect(checkbox, &QCheckBox::clicked, this, &SessionRestoreDialog::slotClicked);
    mainLayout->addWidget(checkbox);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::No|QDialogButtonBox::Yes);
    mainLayout->addWidget(m_buttonBox);
    QPushButton *yesButton = m_buttonBox->button(QDialogButtonBox::Yes);
    QPushButton *noButton = m_buttonBox->button(QDialogButtonBox::No);
    QPushButton *cancelButton = m_buttonBox->button(QDialogButtonBox::Cancel);

    connect(yesButton, &QPushButton::clicked, this, [this]() { done(QDialogButtonBox::Yes); });
    connect(noButton, &QPushButton::clicked, this, [this]() { done(QDialogButtonBox::No); });
    connect(cancelButton, &QPushButton::clicked, this, [this]() { reject(); });

    KGuiItem::assign(yesButton, KGuiItem(i18nc("@action:button yes", "Restore Session"), QStringLiteral("window-new")));
    KGuiItem::assign(noButton, KGuiItem(i18nc("@action:button no", "Do Not Restore"), QStringLiteral("dialog-close")));
    KGuiItem::assign(cancelButton, KGuiItem(i18nc("@action:button ask later", "Ask Me Later"), QStringLiteral("chronometer")));

    yesButton->setDefault(true);
    yesButton->setFocus();
}

bool SessionRestoreDialog::isEmpty() const
{
    return m_treeWidget->topLevelItemCount() == 0;
}

QStringList SessionRestoreDialog::discardedWindowList() const
{
    return m_treeWidget->discardedWindowList();
}

bool SessionRestoreDialog::isDontShowChecked() const
{
    return m_dontShowChecked;
}

void SessionRestoreDialog::slotClicked(bool checked)
{
    m_dontShowChecked = checked;
}

void SessionRestoreDialog::showContextMenu(const QPoint &pos) {
    QTreeWidgetItem *item = m_treeWidget->itemAt(pos);
    if (!item) return;

    QString copyThis = m_treeWidget->url(item);

    // Only present the tooltip if it's a view with a URL
    if (!copyThis.isEmpty()) {
        QMenu context_menu;
        int screenWidth = m_treeWidget->screen()->geometry().width();
        QFontMetrics metrics(m_treeWidget->font());

        QString clipboardPrompt = i18nc("@tooltip:copy url to clipboard", "Copy url to clipboard:  %1", copyThis);
        const Qt::TextElideMode elideMode = QUrl(copyThis).isLocalFile() ? Qt::ElideMiddle : Qt::ElideRight;
        QString clipboardPromptFit = metrics.elidedText(clipboardPrompt, elideMode, screenWidth);

        QAction *copyAction = context_menu.addAction(clipboardPromptFit);
        connect(copyAction, &QAction::triggered, this, [copyThis]() {
            QApplication::clipboard()->setText(copyThis);
        });

        context_menu.exec(m_treeWidget->mapToGlobal(pos));
    }
}

void SessionRestoreDialog::updateApplyButton(int sessionCount)
{
    m_buttonBox->button(QDialogButtonBox::Yes)->setEnabled(sessionCount>0);
}

void SessionRestoreDialog::saveDontShow(const QString &dontShowAgainName, int result)
{
    if (dontShowAgainName.isEmpty()) {
        return;
    }

    KConfigGroup::WriteConfigFlags flags = KConfig::Persistent;
    if (dontShowAgainName[0] == ':') {
        flags |= KConfigGroup::Global;
    }

    KConfigGroup cg(KSharedConfig::openConfig().data(), "Notification Messages");
    cg.writeEntry(dontShowAgainName, result == QDialogButtonBox::Yes, flags);
    cg.sync();
}

bool SessionRestoreDialog::shouldBeShown(const QString &dontShowAgainName, int *result)
{
    if (dontShowAgainName.isEmpty()) {
        return true;
    }

    KConfigGroup cg(KSharedConfig::openConfig().data(), "Notification Messages");
    const QString dontAsk = cg.readEntry(dontShowAgainName, QString()).toLower();

    if (dontAsk == QLatin1String("yes") || dontAsk == QLatin1String("true")) {
        if (result) {
            *result = QDialogButtonBox::Yes;
        }
        return false;
    }

    if (dontAsk == QLatin1String("no") || dontAsk == QLatin1String("false")) {
        if (result) {
            *result = QDialogButtonBox::No;
        }
        return false;
    }

    return true;
}
