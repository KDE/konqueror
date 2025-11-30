/*
    SPDX-FileCopyrightText: 2025 Stefano Crocc <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KCM_SPEEDDIALCONFIG_H
#define KCM_SPEEDDIALCONFIG_H

#include "ui_speeddialconfig.h"

#include "konqsettings.h"

#include <KCModule>

struct SpeedDialEntry;

class QStandardItemModel;
class QStandardItem;
class QPersistentModelIndex;
class QPixmap;

class KJob;

/**
 * @brief KCModule for customizing speed dial entries
 *
 * The module consists of a QTreeView to display entries and several buttons to
 * operate on them.
 */
class SpeedDialConfigModule : public KCModule
{
    Q_OBJECT

public:

    /**
     * @brief Constructor
     *
     * @param parent the parent object
     * @param md the plugin metadata for the object
     */
    SpeedDialConfigModule(QObject *parent, const KPluginMetaData &md={});
    ~SpeedDialConfigModule() override; //!< Destructor

    /**
     * @brief Override of KCModule::load()
     *
     * It fills the list of existing speed dial entries
     */
    void load() override;

    /**
     * @brief Override of KCModule::save()
     *
     * It writes the list of speed dial entries to the configuration file.
     */
    void save() override;

    /**
     * @brief Override of KCModule::defaults()
     *
     * It resets the list of speed dial entry to the default state (no entry)
     */
    void defaults() override;

    /**
     * @brief The list of entries chosen by the user
     * @return the list of entries chosen by the user
     */
    QList<Konq::Settings::SpeedDialEntry> entries() const;

private:

    using Entry = Konq::Settings::SpeedDialEntry; //!< Shortcut to access Konq::Settings::SpeedDialEntry

    /**
     * @brief Inserts a row for the given entyr in the model
     *
     * @param name the entry to insert the row for
     * @param row the row where the entry should be inserted. If negative, it's
     * inserted as the last row in the model
     */
    QStandardItem* insertRow(const Entry &entry, int row = -1);

    /**
     * @brief The item in the given row showing the entry name
     * @param row the row number
     * @return the item in the given row showing the entry name or `nullptr` if no such row exists
     */
    QStandardItem* nameItem(int row) const;

    /**
     * @brief The item in the given row showing the entry url
     * @param row the row number
     * @return the item in the given row showing the entry url or `nullptr` if no such row exists
     */
    QStandardItem* urlItem(int row) const;

    /**
     * @brief Enum describing the custom roles used in this model
     */
    enum Roles {
        UrlRole = Qt::UserRole, //!< A role for the URL associated with an entry
        IconNameRole, //!< A role for storing the url of the icon associated with the entry
    };

    /**
     * @brief An enum to describe what is shown in each column
     */
    enum Columns {
        NameColumn = 0, //!< The column containing the name of the entry
        UrlColumn //!< The column containing the URL of the entry
    };

    /**
     * @brief The size of the icons shown in the list
     * @return the size of the icons shown in the list
     */
    static constexpr int s_pixmapSize = 16;

    /**
     * @brief Displays the icon associated with the given entry
     *
     * The icon is displayed in the name column as a decoration.
     *
     * If the icon is a remote icon or favicon and it hasn't been downloaded yet,
     * nothing is done.
     *
     * @param row the row where the icon should be displayed
     * @param entry the entry to display the icon for
     */
    void displayItemIcon(int row, const Entry &entry);

    /**
     * @brief The name of the entry in the given row
     * @param row the row to retrieve the name for
     * @return the name of the entry in the row @p row
     */
    QString entryName(int row) const;

    /**
     * @brief The URL of the entry in the given row
     * @param row the row to retrieve the URL for
     * @return the URL of the entry in the row @p row
     */
    QUrl entryUrl(int row) const;

    /**
     * @brief The URL of the icon for the entry in the given row
     * @param row the row to retrieve the icon URL for
     * @return the URL of the icon for the entry in the row @p row
     */
    QUrl entryIcon(int row) const;

    /**
     * @brief Changes the name of the given item
     *
     * It also changes the tooltip of the item so that it's equal to @p name
     * @param item the item to set the name for
     * @param name the new name of the item
     */
    void setEntryName(QStandardItem *it, const QString &name);

    /**
     * @brief Changes the URL of the given item
     *
     * It also changes the tooltip of the item so that it's equal to @p url and
     * stores @p url in the UrlRole of the item.
     * @param item the item to set the URL for
     * @param url the new URL of the item
     */
    void setEntryUrl(QStandardItem *it, const QUrl &url);

    /**
     * @brief Displays the icon for a given entry
     *
     * The icon is displayed in the NameColumn column.
     * @param row the row corresponding to the entry
     * @param icon the icon to show
     */
    void setEntryPixmap(int row, const QPixmap &icon);

    /**
     * @brief Returns a list of all selected items
     *
     * @return a list of all selected items
     */
    QStandardItem* selectedItem() const;
private Q_SLOTS:

    /**
     * @brief Allows the user to add a new entry
     *
     * The user is shown a dialog where he can input the data for the new entry,
     * then adds that entry to the list.
     */
    void addEntry();

    /**
     * @brief Removes the selected speed dial entry from the list
     *
     * The user is asked for confirmation before removing the entry
     *
     * If no entry is selected in the list, nothing is done.
     */
    void removeEntry();

    /**
     * @brief Allows the user to change the selected entry
     *
     * It calls editEntry() with the selected entry row. If no entry is selected it does nothing
     */
    void editSelectedEntry();

    /**
     * @brief Moves the selected entry one step up in the list
     *
     * If no entry is selected in the list, nothing is done.
     */
    void moveEntryUp();

    /**
     * @brief Moves the selected entry one step down in the list
     *
     * If no entry is selected in the list, nothing is done.
     */
    void moveEntryDown();

    /**
     * @brief Updates the status of buttons which depend on an item being selected
     *
     * The "edit entry" and "remove entry" buttons are enabled if one entry is selected; the
     * "move entry up" and "move entry down" buttons are enabled only if the selected
     * entry isn't respectively the first and the last in the list.
     */
    void updateButtonsStatus();

    /**
     * @brief Allows the user to edit an entry
     *
     * The user is shown a dialog where he can choose to change the given entry.
     * If the user confirms the dialog, the changes are applied to the list.
     *
     * @param row the row of the entry to edit.
     */
    void editEntry(int row);

    /**
     * @brief Moves the selected entry a number of steps in the list
     *
     * If moving the entry the required number of steps would put it before the beginning
     * or after the end of the list, it's moved to the beginning or end of the list instead.
     *
     * @param steps the number of steps to move the entry. If this is negative,
     * the entry is moved upwards, otherwise it's moved downwards
     */
    void moveSelectedEntry(int steps);

    /**
     * @brief Updates the list if something else changes the speed dial settings
     *
     * If @p cause is `this`, nothing is done
     *
     * @param cause the object which caused the change in the speed dial settings.
     */
    void updateEntryList(QObject *cause);

    /**
     * @brief Slot called when the remote pixmap for an entry has been downloaded
     *
     * It changes the pixmap shown for the rows corresponding to @p entry.
     *
     * @note This only changes the pixmap shown in the list: it doesn't affect the
     * value stored in the IconNameRole. This is because @p iconEntry will be the
     * path of a cached version of the icon
     * @param entry the entry to update the pixmap for
     */
    void updatePixmap(const Entry &entry, const QUrl &cachedIconUrl);

private:
    Ui::SpeedDialConfigModule m_ui; //!< The UI object
    QStandardItemModel *m_model; //!< The model used to display entries
};

#endif // KCM_SPEEDDIALCONFIG_H

