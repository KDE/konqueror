/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2009 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQ_HISTORYDIALOG_H
#define KONQ_HISTORYDIALOG_H

#include <QDialog>

#include "konqhistorysettings.h"

class KonqMainWindow;
class KonqHistoryView;
class QModelIndex;
class QUrl;

/**
 * @brief The dialog where the user can navigate history
 *
 * The dialog contains a list of URLs recently visited, grouped by domain, a line edit
 * which the user can use to filter them and buttons to change sorting and configure history.
 *
 * What happens when the user clicks on an history entry depends on the configuration
 * settings in the history KCM: the entry can be always opened in the current tab,
 * always opened in a new tab or determined automatically (in the current tab if it's empty
 * and in a new one otherwise). A context menu is available for more options
 *
 * The main widget of the dialog is a KonqHistoryView, so see its documentation for
 * more information.
 */
class KonqHistoryDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     *
     * @p parent the main window
     */
    KonqHistoryDialog(KonqMainWindow *parent = nullptr);
    ~KonqHistoryDialog() override; //!< Destructor

    /**
     * @brief Override of `QDialog::sizeHint()`
     *
     * @return a fixed size of 500x400
     */
    QSize sizeHint() const override;

private Q_SLOTS:
    /**
     * @brief Opens the given URL in a new window
     * @param url the URL to open
     */
    void slotOpenWindow(const QUrl &url);

    /**
     * @brief Opens the given URL in a new tab
     * @param url the URL to open
     */
    void slotOpenTab(const QUrl &url);

    /**
     * @brief Opens the given URL in the current tab
     * @param url the URL to open
     */
    void slotOpenCurrentTab(const QUrl &url);

    /**
     * @brief Opens the given URL in a new tab or in the current tab
     *
     * If the current tab is empty (it's URL is empty or is a `konq` URL), the URL
     * will be opened in the current tab, otherwise it will be opened in a new tab
     *
     * @param url the URL to open
     */
    void slotOpenCurrentOrNewTab(const QUrl &url);

    /**
     * @brief Slot called when an index in the tree view is activated
     *
     * Depending on the user settings, it opens the URL associated with the index
     * in a new tab, in the current tab or in a tab automatically determined (as
     * described for slotOpenCurrentOrNewTab()).
     *
     * @param index the activated index
     */
    void slotOpenIndex(const QModelIndex &index);

    /**
     * @brief Reads again the configuration files
     */
    void reparseConfiguration();

private:

    KonqHistoryView *m_historyView; //!< The history view which is the dialog main widget
    KonqMainWindow *m_mainWindow; //!< The main window where this dialog has been opened from

    KonqHistorySettings *m_settings; //!< The object containing history configuration
    KonqHistorySettings::Action m_defaultAction; //!< The action to carry out when the user activates an history entries
};

#endif // KONQ_HISTORYDIALOG_H
