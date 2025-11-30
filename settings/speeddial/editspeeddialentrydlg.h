// This file is part of the KDE project
// SPDX-FileCopyrightText: 2025 Stefano Crocco <stefano.crocco@alice.it>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef EDITSPEEDDIALENTRYDLG_H
#define EDITSPEEDDIALENTRYDLG_H

#include "konqsettings.h"

#include <QDialog>
#include <QScopedPointer>
#include <QUrl>

namespace Ui
{
class EditSpeedDialEntryDlg;
}

/**
 * @brief Dialog where the user can create a new speed dial entry or edit an existing entry
 *
 * The dialog allows the user to choose the name, the URL and the icon for the entry.
 *
 * The URL can be either remote or local and, when the dialog is closed, is passed through
 * two URI filters: `kshorturifilter`, which allows to enter shorter URLs (for example `kde.org`
 * instead of `https://kde.org` or `~` instead of the full path to the home directory) and `fixuphosturifilter`
 * which appends `www.` to the host name of an `http` URL if the original URL doesn't exist.
 *
 * There can be three choices for icons:
 * - a local icon which can be either a full path or the name of an icon provided by the current theme
 * - a remote icon chosen by the user
 * - the favicon associated to the chosen URL.
 */
class EditSpeedDialEntryDlg : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     *
     * @param parent the parent widget
     */
    EditSpeedDialEntryDlg(QWidget* parent = nullptr);

    using Entry = Konq::Settings::SpeedDialEntry;

    /**
     * @brief Constructor for a dialog to edit an existing entry
     *
     * @param entry the entry to edit
     * @param parent the parent widget
     */
    EditSpeedDialEntryDlg(const Entry &entry, QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~EditSpeedDialEntryDlg();

    using MaybeEntry = std::optional<Entry>;

    /**
     * @brief The entry chosen by the user
     *
     * This should only be called after the dialog has been closed because accept()
     * applies some URI filters to the URL entered by the user.
     *
     * @return The entry chosen by the user or `std::nullopt` if the user didn't finish
     * entering the required data
     * @note The icon can be specified in the following ways:
     *  - an empty string if the user decided to use the favicon
     *  - a string representation of the URL of a remote file
     *  - the path of a local file
     *  - the name of an icon to load using `QIcon::fromTheme()`
     */
    MaybeEntry entry() const;

    /**
     * @brief Displays a dialog to create a new entry
     *
     * @param parent the parent widget for the dialog
     * @return the new entry or `std::nullopt` if the user canceled the dialog
     */
    static MaybeEntry newEntry(QWidget *parent = nullptr);

protected Q_SLOTS:
    /**
     * @brief Override of `QDialog::accept()`
     *
     * It applies the `kshorturifilter` and `fixuphosturifilter` URI filters before
     * closing the dialog.
     */
    void accept() override;

private Q_SLOTS:
    /**
     * @brief Enables or disables the Ok button depending on whether all required information
     * has been entered
     *
     * The Ok button is enabled if both the name and the URL have been entered and
     * it's disabled otherwise.
     */
    void updateOkBtn();

private:
    QScopedPointer<Ui::EditSpeedDialEntryDlg> m_ui; //!< The UI object
};

#endif // EDITSPEEDDIALENTRYDLG_H
