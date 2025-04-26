/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2008 Eduardo Robles Elvira <edulix@gmail.com>
    SPDX-FileCopyrightText: 2025 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQSESSIONDIALOG_H
#define KONQSESSIONDIALOG_H

#include <QObject>
#include <QStringList>
#include <QString>
#include <QTreeWidget>
#include <QDialog>
#include <QFontMetrics>

#include <kconfig.h>
#include <konqprivate_export.h>
#include <config-konqueror.h>
#include <KX11Extras>
#include <KConfigGroup>

class KonqMainWindow;
class QDialogButtonBox;
class QTreeWidgetItem;

/**
 * @brief Tree widget used to show a list of sessions and their contents
 *
 * The tree contains sessions, windows and views, with sesssions containing windows
 * and windows containing views. Sessions and windows are checkable so that the user
 * can choose to restore only some sessions or windows. It's not currently possible
 * to only restore some views.
 */
class SessionTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:

    /**
     * @brief Constructor
     *
     * @param parent the parent widget
     */
    SessionTreeWidget(QWidget* parent = nullptr);
    ~SessionTreeWidget(); //!< Destructor

    /**
     * @brief Fills the tree with the sessions contained in the given files
     *
     * @param sessionFilePaths a list of the files containing the sessions to
     * display in the tree
     */
    void fill(const QStringList &sessionFilePaths);

    /**
     * @brief Returns the URL corresponding to one of the entries in the tree
     *
     * Only itmes representing views have URLs associated with them.
     *
     * @param item the item to retrieve the URL for
     * @return a string with the URL associated to @p item or an empty string if
     * @p item is `nullptr` or it doesn't represent a view
     * @note This function assumes that the url is stored in the first column
     */
    static QString url(QTreeWidgetItem *item);

    /**
     * @brief Returns an unique identifier for one of the entries in the tree
     *
     * @param item the item to retrieve the unique identifier for
     * @param column the column where the identifier is stored
     */
    static QString viewId(QTreeWidgetItem *item, int column = 0);

    /**
     * @brief A list of windows which should be discarded according to the user's
     * choices
     *
     * A window should be discarded if the user left unchecked in the widget.
     */
    QStringList discardedWindowList() const;

Q_SIGNALS:

    /**
     * @brief Signal emitted when the number of sessions the user wants to restore changes
     *
     * The sessions to restore are those whose items are checked in the tree widget
     */
    void sessionCountChanged(int count);

private Q_SLOTS:

    /**
     * @brief Slot called when an item in the tree changes
     *
     * It ensures that:
     * - when the user checks or unchecks an item, its children all get checked or unchecked
     * - an item representing a session is checked if any of its children or grandchildren are checked
     * and it's unchecked all of them are unchecked.
     *
     * @param item the item which has changed
     * @param column the column which has changed
     */
    void slotItemChanged(QTreeWidgetItem *item, int column);

private:

    /**
     * @brief Struct representing an unique identifier
     */
    struct WindowId {
        QString session; //!< The name of the session the window belongs to
        QString window; //!< The name of the window
    };

    /**
     * @brief Enum describing custom roles used by this class
     */
    enum CustomRoles {
        IdRole = Qt::UserRole, //!< Role containing an item id
        UrlRole //!< Role containing a view's URL
    };

    /**
     * @brief Enum describing what an item represents
     */
    enum ItemLevel {
        SessionLevel = 0, //!< An item representing a session
        WindowLevel, //!< An item representing a window
        ViewLevel //!< An item representing a view
    };

    /**
     * @brief Fills the tree according to the contents of a session file
     *
     * @param sessionFile the path of the session file to read session contents from
     */
    void fillSession(const QString &sessionFile);

    /**
     * @brief Creates an item representing a view
     * @param windowItem the item representing the window containing the view.
     *  The item for the view will become a child of this item
     * @param windowId the identifier of the parent window
     * @param windowGroup the configuration group to read view information from
     * @param key The key of the group containing the view information. It should start with
     *  `_CurrentHistoryItem`
     * @return an item corresponding to the view described by @p windowGroup
     */
    QTreeWidgetItem* createViewItem(QTreeWidgetItem *windowItem, const WindowId &windowId, const KConfigGroup &windowGroup, const QString &key);


    /**
     * @brief Updates the checked items count
     *
     * This function increases or decreases #m_sessionItemsCount by one depending on whether @p it
     * is checked or not and does the same for the entry in #m_checkedSessionItems corresponding
     * to the parent item of @p it.
     *
     * @param it the item to use to determine whether to increase or decrease counts
     * @param column the column to use to determine the checked status of the item
     */
    void updateCounts(QTreeWidgetItem *it, int column);

    /**
     * @brief The level of an item
     *
     * @param item the item to retrieve the level for
     * @return the level (SessionLevel, WindowLevel, ViewLevel) of @p item
     */
    ItemLevel itemLevel(QTreeWidgetItem *item);

    /**
     * @brief An identifier for a view which is unique among sessions and windows
     * @param sessionFile the path of the session file where the view is described
     * @param windowId the id of the window containing the view
     * @param viewId an identifier for the view which is unique in the window @p windowId
     * @return an identifier for the view @p viewId which is unique among sessions and windows
     */
    QString fullViewId(const QString &sessionFile, const QString &windowId, const QString &viewId);

private:
    QFontMetrics m_fontMetrics; //!< Information about the font used for the widget
    int m_minWidth = 0; //!< The minimum with that the widget should have
    QString m_genericToolTip; //!< The the text for the tool tip of session and window items
    int m_sessionItemsCount = 0; //!< The number of checked items
    QHash<QTreeWidgetItem *, int> m_checkedSessionItems; //!< The number of checked windows items for each session item
    static constexpr ItemLevel s_maxDisplayedLevel = WindowLevel; //!< The maximum level of items which have children
    static constexpr ItemLevel s_lastLevel = ViewLevel; //!< The maximum level of item to be shown
    static constexpr double s_maxRelativeWidth = 0.5; //!< The maximum with of the widget relative to the screen
};

/**
 * @brief Dialog which lets the user choose which sessions to restore
 *
 * The dialog shows a list of sessions, windows and views and allows the user to
 * choose what to restore (including not restoring anything). It also allows the
 * user to choose skipping restoring sessions so that he'll be asked again later.
 *
 * The dialog also provides a "don't show again" check box which the user can check
 * to avoid being shown the dialog every time
 */
class SessionRestoreDialog : public QDialog
{
    Q_OBJECT
public:

    /**
     * @brief Constructor
     * @param sessionFilePaths a list of the files containing available sessions to restore
     * @param parent the parent widget
     */
    explicit SessionRestoreDialog(const QStringList &sessionFilePaths, QWidget *parent = nullptr);
    ~SessionRestoreDialog() override; //!< Destructor

    /**
     * @brief Whether there aren't sessions available
     *
     * @return `true` if there aren't available sessions and `false` if there is at least one session
     */
    bool isEmpty() const;

    /**
     * @brief Returns the list of session discarded/unselected by the user.
     *
     * @return A list of the session which the user didn't check
     */
    QStringList discardedWindowList() const;

    /**
     * @brief Whether the "don't show again" checkbox is checked
     * @return `true` if the "don't show" checkbox is checked and `false` if it's unchecked
     */
    bool isDontShowChecked() const;

    /**
     * Returns true if the corresponding session restore dialog should be shown.
     *
     * @param dontShowAgainName the name that identify the session restore dialog box.
     * @param result if not null, it will be set to the result that was chosen the last
     * time the dialog box was shown. This is only useful if the restore dialog box should
     * be shown.
     */
    static bool shouldBeShown(const QString &dontShowAgainName, int *result);

    /**
     * Save the fact that the session restore dialog should not be shown again.
     *
     * @param dontShowAgainName the name that identify the session restore dialog. If
     * empty, this method does nothing.
     * @param result the value (Yes or No) that should be used as the result
     * for the message box.
     */
    static void saveDontShow(const QString &dontShowAgainName, int result);

private Q_SLOTS:
    /**
     * @brief Slot called when the "don't show again" check box is clicked
     *
     * It stores the checked status of the check box
     * @param checked whether the check box has been turned on or off
     */
    void slotClicked(bool checked);

    /**
     * @brief Shows a context menu allowing the user to copy the URL of a view
     *
     * The context menu is only shown if the user is right-clicking an item in
     * the sessions widget which corresponds to a view.
     * @param pos the point where the user asked to display the context menu
     */
    void showContextMenu(const QPoint &pos);

    /**
     * @brief Updates the state of the "Restore sessions" button
     *
     * The button is enabled if the user chose to restore at least one session and
     * disabled otherwise.
     *
     * @param sessionCount the number of sessions the user chose to restore in
     * the session tree widget
     */
    void updateApplyButton(int sessionCount);

private:
    /**
     * @brief Creates the user interface
     */
    void createUi();

private:
    SessionTreeWidget *m_treeWidget; //!< The widget where the user chooses the sessions to restore
    QDialogButtonBox *m_buttonBox; //!< The buttons to confirm the choices
    bool m_dontShowChecked = false; //!< Whether or not the user checked the "Don't show again" check box
};

#endif /* KONQSESSIONDIALOG_H */
