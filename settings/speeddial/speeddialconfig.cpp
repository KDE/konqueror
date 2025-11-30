/*
    SPDX-FileCopyrightText: 2025 Stefano Crocc <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "speeddialconfig.h"
#include "editspeeddialentrydlg.h"

#include "konqsettings.h"
#include "interfaces/speeddial.h"
#include "interfaces/browser.h"

#include <KSharedConfig>
#include <KPluginFactory>
#include <KLocalizedString>
#include <KConfigGroup>
#include <KIO/FavIconRequestJob>
#include <KIO/StoredTransferJob>
#include <KIO/FileJob>
#include <KIconLoader>

#include <QStandardItemModel>
#include <QStandardItem>
#include <QPushButton>
#include <QPixmap>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QDBusMessage>
#include <QDBusConnection>

K_PLUGIN_CLASS_WITH_JSON(SpeedDialConfigModule, "kcm_speeddial.json")

using namespace Qt::Literals::StringLiterals;

SpeedDialConfigModule::SpeedDialConfigModule(QObject *parent, const KPluginMetaData &md)
    : KCModule(parent, md), m_model{new QStandardItemModel(this)}
{
    m_ui.setupUi(widget());
    m_model->setHorizontalHeaderLabels({i18n("Name"), i18n("Url")});
    m_ui.entries->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_ui.entries->setModel(m_model);

    connect(m_ui.addEntryBtn, &QPushButton::clicked, this, &SpeedDialConfigModule::addEntry);
    connect(m_ui.editEntryBtn, &QPushButton::clicked, this, &SpeedDialConfigModule::editSelectedEntry);
    connect(m_ui.removeEntryBtn, &QPushButton::clicked, this, &SpeedDialConfigModule::removeEntry);
    connect(m_ui.moveUpBtn, &QPushButton::clicked, this, &SpeedDialConfigModule::moveEntryUp);
    connect(m_ui.moveDownBtn, &QPushButton::clicked, this, &SpeedDialConfigModule::moveEntryDown);
    connect(m_ui.entries->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SpeedDialConfigModule::updateButtonsStatus);
    connect(m_ui.entries, &QTreeView::doubleClicked, this, [this](const QModelIndex &idx){editEntry(idx.row());});

    KonqInterfaces::SpeedDial *sd = KonqInterfaces::SpeedDial::speedDial();
    if (sd) {
        connect(sd, &KonqInterfaces::SpeedDial::iconReady, this, &SpeedDialConfigModule::updatePixmap);
        connect(sd, &KonqInterfaces::SpeedDial::speedDialChanged, this, &SpeedDialConfigModule::updateEntryList);
    }

    updateButtonsStatus();
}

SpeedDialConfigModule::~SpeedDialConfigModule()
{
}

QStandardItem* SpeedDialConfigModule::nameItem(int row) const
{
    return m_model->item(row, NameColumn);
}

QStandardItem* SpeedDialConfigModule::urlItem(int row) const
{
    return m_model->item(row, UrlColumn);
}

QString SpeedDialConfigModule::entryName(int row) const
{
    QStandardItem *it = m_model->item(row, NameColumn);
    return it ? it->text() : QString{};
}

QUrl SpeedDialConfigModule::entryUrl(int row) const
{
    QStandardItem *it = m_model->item(row, UrlColumn);
    return it ? it->data(UrlRole).toUrl() : QUrl{};
}

QUrl SpeedDialConfigModule::entryIcon(int row) const
{
    QStandardItem *it = m_model->item(row, NameColumn);
    return it ? it->data(IconNameRole).toUrl() : QUrl{};
}

void SpeedDialConfigModule::setEntryName(QStandardItem* it, const QString& name)
{
    it->setText(name);
    it->setToolTip(name);
}

void SpeedDialConfigModule::setEntryUrl(QStandardItem* it, const QUrl &url)
{
    it->setText(url.toString());
    it->setToolTip(it->text());
    it->setData(url, UrlRole);
}

void SpeedDialConfigModule::setEntryPixmap(int row, const QPixmap& icon)
{
    nameItem(row)->setIcon(icon);
}

void SpeedDialConfigModule::updatePixmap(const Entry& entry, const QUrl& cachedIconUrl)
{
    for (int i = 0; i < m_model->rowCount(); ++i) {
        if (entry.name == entryName(i) && entry.url == entryUrl(i) && entry.iconUrl == entryIcon(i)) {
            QPixmap pix(cachedIconUrl.path());
            nameItem(i)->setIcon(pix);
        }
    }
}

QList<Konq::Settings::SpeedDialEntry> SpeedDialConfigModule::entries() const
{
    QList<Entry> entries;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        QString suffix = QString::number(i);
        entries.append({entryName(i), entryUrl(i), entryIcon(i)});
    }
    return entries;
}

void SpeedDialConfigModule::load()
{
    m_model->removeRows(0, m_model->rowCount());
    for (const Entry &e : Konq::Settings::self()->speedDialEntries()) {
        insertRow(e);
    }
    setNeedsSave(false);
}

void SpeedDialConfigModule::save()
{
    KonqInterfaces::SpeedDial *sd = KonqInterfaces::SpeedDial::speedDial();
    if (!sd) {
        return;
    }

    sd->setEntries(entries(), this);

    QDBusMessage message =
        QDBusMessage::createSignal(QStringLiteral("/KonqMain"), QStringLiteral("org.kde.Konqueror.Main"), QStringLiteral("reparseConfiguration"));
    QDBusConnection::sessionBus().send(message);
    setNeedsSave(false);
}

void SpeedDialConfigModule::defaults()
{
    bool willNeedSave = needsSave() || m_model->rowCount() > 0;
    m_model->clear();
    setNeedsSave(willNeedSave);
}

QStandardItem* SpeedDialConfigModule::insertRow(const Entry &entry, int row)
{
    QStandardItem *nameIt = new QStandardItem();
    setEntryName(nameIt, entry.name);
    nameIt->setData(entry.iconUrl, IconNameRole);
    QStandardItem *urlIt = new QStandardItem();
    setEntryUrl(urlIt, entry.url);

    QList<QStandardItem*> newRow{nameIt, urlIt};
    if (row > -1) {
        m_model->insertRow(row, newRow);
    } else {
        m_model->appendRow(newRow);
    }
    displayItemIcon(nameIt->row(), entry);
    return nameIt;
}

void SpeedDialConfigModule::displayItemIcon(int row, const Entry &entry)
{
    QUrl iconUrl = KonqInterfaces::SpeedDial::speedDial()->localIconUrlForEntry(entry, KIconLoader::SizeSmall);
    if (iconUrl.isEmpty()) {
        return;
    }
    QPixmap pix(iconUrl.path());
    if (pix.size().width() != s_pixmapSize) {
        pix = pix.scaledToWidth(s_pixmapSize);
    }
    setEntryPixmap(row, pix);
}

void SpeedDialConfigModule::addEntry()
{
    EditSpeedDialEntryDlg::MaybeEntry newEntry = EditSpeedDialEntryDlg::newEntry(widget());
    if (!newEntry) {
        return;
    }
    insertRow(*newEntry);
    setNeedsSave(true);
}

void SpeedDialConfigModule::updateButtonsStatus()
{
    bool hasSelection = m_ui.entries->selectionModel()->hasSelection();
    std::array<QPushButton*, 4> btns{m_ui.editEntryBtn, m_ui.removeEntryBtn, m_ui.moveUpBtn, m_ui.moveDownBtn};
    for (QPushButton *b : btns)  {
        b->setEnabled(hasSelection);
    }
    if (hasSelection && selectedItem()->row() == 0) {
        m_ui.moveUpBtn->setEnabled(false);
    } else if (hasSelection && selectedItem()->row() == m_model->rowCount() - 1) {
        m_ui.moveDownBtn->setEnabled(false);
    }
}

QStandardItem * SpeedDialConfigModule::selectedItem() const
{
    QModelIndexList selection = m_ui.entries->selectionModel()->selectedIndexes();
    return selection.isEmpty() ? nullptr : m_model->itemFromIndex(selection.first());
}

void SpeedDialConfigModule::editEntry(int row)
{
    QStandardItem *nameIt = nameItem(row);
    Entry e{entryName(row), entryUrl(row), entryIcon(row)};

    EditSpeedDialEntryDlg dlg(e, widget());
    if (dlg.exec() == QDialog::Rejected) {
        return;
    }

    Entry entry = *dlg.entry();

    setEntryName(nameIt, entry.name);

    QStandardItem *urlIt = urlItem(row);
    setEntryUrl(urlIt, entry.url);

    nameIt->setData(entry.iconUrl, IconNameRole);

    setNeedsSave(true);
    displayItemIcon(nameIt->row(), entry);
}

void SpeedDialConfigModule::editSelectedEntry()
{
    QStandardItem *it = selectedItem();
    if (!it) {
        return;
    }
    editEntry(it->row());
}

void SpeedDialConfigModule::removeEntry()
{
    QStandardItem *it = selectedItem();
    if (!it) {
        return;
    }
    if (QMessageBox::question(widget(), {}, i18n("Do you want to delete \"%1\"?", it->text())) == QMessageBox::No) {
        return;
    }
    m_model->removeRows(it->row(), 1);
    setNeedsSave(true);
}

void SpeedDialConfigModule::moveEntryUp()
{
    moveSelectedEntry(-1);
}

void SpeedDialConfigModule::moveEntryDown()
{
    moveSelectedEntry(1);
}

void SpeedDialConfigModule::moveSelectedEntry(int steps)
{
    QStandardItem *selected = selectedItem();
    if (!selected) {
        return;
    }
    int rowNumber = selected->row();
    int newRowNumber = rowNumber + steps;
    if (newRowNumber == 0) {
        newRowNumber = 0;
    } else if (int rowsNumber = m_model->rowCount(); newRowNumber >=rowsNumber) {
        newRowNumber = rowsNumber - 1;
    }
    QList<QStandardItem*> row = m_model->takeRow(rowNumber);
    m_model->insertRow(rowNumber + steps, row);
    m_ui.entries->selectionModel()->select(row.first()->index(), QItemSelectionModel::ClearAndSelect|QItemSelectionModel::Rows);
    setNeedsSave(true);
}

void SpeedDialConfigModule::updateEntryList(QObject *cause)
{
    //Do nothing if the changes in the speed dial have been caused by this KCM
    if (cause == this) {
        return;
    }

    //If there are no changes, simply reload the list. this is what will happen most of the times
    if (!needsSave()) {
        load();
        return;
    }

    QList<Entry> updatedList = Konq::Settings::self()->speedDialEntries();
    QList<Entry> existingEntries = entries();

    auto compareEntries = [](const Entry &e1, const Entry &e2) {
        return e1.name == e2.name && e1.url == e2.url;
    };
    for (const Entry &ne : updatedList) {
        auto found = std::find_if(existingEntries.cbegin(), existingEntries.cend(), [ne, compareEntries](const Entry &e){return compareEntries(e, ne);});
        if (found == existingEntries.cend()) {
            insertRow(ne);
        } else {
            existingEntries.removeOne(ne);
        }
    }

}


#include "speeddialconfig.moc"
