/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2001 Carsten Pfeiffer <pfeiffer@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQ_COMBO_H
#define KONQ_COMBO_H

#include <khistorycombobox.h>

class QEvent;
class QKeyEvent;
class QPixmap;
class KCompletion;
class KConfig;

/**
 * @brief Class representing the combo box in the location bar, where the user enters the URL
 *
 * It provides:
 * - a security icon to show whether the current page is encrypted or not
 * - handling of movement/deletion by word in the line edit which is more suitable for URLs than the default
 * - on-demand loading of pixmaps for items
 * - saving and restoring of history to a configuration file
 * - starting a drag operation from an item's pixmap
 * - keeping track of whether the current item in the combo box is permanent or temporary. A permanent
 * item is an item which the user has "confirmed" by pressing `return`, while a temporary one hasn't
 * been as yet confirmed.
 *
 * @internal
 * This class inherits `KHistoryComboBox` instead of `KComboBox` only for the up/down keyboard handling
 */
class KonqCombo : public KHistoryComboBox
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     *
     * @param parent the parent widget
     */
    explicit KonqCombo(QWidget *parent);

    ~KonqCombo() override; //!< Destructor

    /**
     * @brief Initializes this combo box and fills it with items
     * @param completion the completion object to be used by the combo box
     */
    void init(KCompletion *completion);

    // determines internally if it's temporary or final
    /**
     * @brief Sets the URL shown in the combo box
     *
     * The URL is considered temporary or permanent depending on the value of #m_returnPressed
     * @param url the URL to insert
     */
    void setURL(const QString &url);

    /**
     * @brief Insert an URL as temporary
     *
     * If a different temporary item exists, it is made permanent by calling applyPermanent().
     *
     * @param url the URL to insert
     * @param pix the pixmap to use for the new item
     */
    void setTemporary(const QString &url, const QPixmap &pix);

    /**
     * @brief Insert an URL as temporary
     * @overload void setTemporary(const QString &url)
     */
    void setTemporary(const QString &);

    /**
     * @brief Makes the temporary item permanent and resets the temporary item
     *
     * @param makeCurrent whether the new temporary item should become the current
     * item in the combo box
     */
    void clearTemporary(bool makeCurrent = true);

    /**
     * @brief Removes an URL from the history
     *
     * This also makes the temporary item permanent and makes the current text into
     * a temporary item
     *
     * @param url the URL to remove from history
     */
    void removeURL(const QString &url);

    /**
     * @brief Inserts a permanent URL
     *
     * This is used in response to the addToCombo DBus signal emitted by other Konqueror windows
     * when a permanent URL is inserted in their combo box
     *
     * @param url the URL to add
     */
    void insertPermanent(const QString &url);

    /**
     * @brief Updates the pixmaps for all items
     */
    void updatePixmaps();

    /**
     * @brief Fills the combo box with entries read from the configuration file
     */
    void loadItems();

    /**
     * @brief Writes the contents of the combo box to the configuration file
     *
     * The first item in the combo box is only written if it's permanent.
     */
    void saveItems();

    /**
     * @brief Sets the configuration object to store the contents of the combo box
     * @param cfg the configuration object
     */
    static void setConfig(KConfig *cfg);

    /**
     * @brief Displays the popup
     *
     * This works as `QComboBox::showPopup()` except that, before displaying the popup,
     * it creates the pixmap for each item which doesn't have it yet
     */
    virtual void popup();

    /**
     * @brief Sets the security level of the current URL
     *
     * If the new security level is different from the old one, the widget is updated
     *
     * @param pageSecurity the new page security level. It must be one of the values in KonqMainWindow::PageSecurity
     */
    void setPageSecurity(int pageSecurity);

    /**
     * @brief Inserts an item in the combo box
     *
     * It works exactly as `QComboBox::insertItem()` except that the arguments are in a different order,
     * which allows to give the index a default value.
     *
     * @note This function works on a lower level than setUrl(), insertPermanent() and setTemporary() as it
     * only takes care of inserting the item in the combo box and doesn't consider the item's meaning. Because of this,
     * it should only be called by other members of this class
     *
     * @param pixmap the pixmap for the item
     * @param text the text of the item
     * @param index the position in the combo box where to insert the item. A negative value means inserting the item
     * at the beginning
     * @param title the title of the item
     */
    void insertItem(const QPixmap &pixmap, const QString &text, int index = -1, const QString &title = QString());
    /**
     * @overload insertItem(const QString &text, int index = -1, const QString &title = QString())
     *
     * @param text the text of the item
     * @param index the position in the combo box where to insert the item. A negative value means inserting the item
     * at the beginning
     * @param title the title of the item
     */
    void insertItem(const QString &text, int index = -1, const QString &title = QString());

protected:
    /**
     * @brief Override of `KHistoryComboBox::keyPressEvent()`
     *
     * It works as the base class version, except that it sets the text as a temporary URL
     * when pressing the up and down keys
     * @param e the event
     */
    void keyPressEvent(QKeyEvent *e) override;

    /**
     * @brief Improves handling of `Ctrl+Del/Backspace`, `Ctrl+Left/Right` and mouse double click on the line edit
     *
     * It uses selectWord() rather than jumping or deleting up to the next/previous space as `QLineEdit` does and
     * selects the whole content of the line edit on a mouse double click.
     *
     * @param o the object receving the event
     * @param ev the event
     * @return `true` if the event was handled and `false` otherwise
     */
    bool eventFilter(QObject *o, QEvent *ev) override;

    /**
     * @brief Override of `KHistoryComboBox::mousePressEvent()`
     *
     * If the user clicked the pixmap, it records the position where the click happened in #m_dragStart.
     *
     * If the user clicked on the security icon, emits the showPageSecurity() signal
     *
     * @param e the event
     */
    void mousePressEvent(QMouseEvent *e) override;

    /**
     * @brief Override of `KHistoryComboBox::mouseMoveEvent()`
     *
     * It starts a drag operation if the user clicked on the pixmap
     *
     * @param e the event
     */
    void mouseMoveEvent(QMouseEvent *e) override;

    /**
     * @brief Override of `KHistoryComboBox::paintEvent()`
     *
     * It works as the base class version except that it displays the security icon, if any
     *
     * @param e the event
     */
    void paintEvent(QPaintEvent *e) override;

    /**
     * @brief Acts on the contents of the line edit until a word break
     *
     * Depending on the event @p e, "acting" on the line edit can mean: moving, selecting or
     * deleting until the next or previous word break.
     *
     * This function considers the following characters as word breaking: space,
     * `/`, `.`, `?`, `#`, `:`. This is because they're used to separate the different
     * part of URLs
     *
     * @param e the key event describing what actions should be taken
     */
    void selectWord(QKeyEvent *e);

Q_SIGNALS:
    /**
     * @brief Signal emitted when an item is activated
     *
     * It's used as `QComboBox::activated()` except that it also passes the keyboard modifiers
     * active when the item was activated
     *
     * @param text the text of the activated item
     * @param mods the keyboard modifiers when the item was activated
     */
    void activated(const QString &text, Qt::KeyboardModifiers mods);

    /**
     * @brief Signal emitted when the user clicks on the security icon
     */
    void showPageSecurity();

private Q_SLOTS:

    /**
     * @brief Emits the `comboCleared` DBus signal
     */
    void slotCleared();

    /**
     * @brief Loads the icon for an item
     *
     * @param index the index of the item to set the icon for
     */
    void slotSetIcon(int index);

    /**
     * @brief Slot called when the combo box is activated
     *
     * It makes the temporary item permanent and emits the activated() signal
     */
    void slotActivated(const QString &text);

    /**
     * @brief Slot called when the text in the line edit widget is edited
     *
     * It ensures that the line edit widget doesn't contain newline characters
     * @param text the text in the line edit widget
     */
    void slotTextEdited(const QString &text);

    /**
     * @brief Slot called when `return` is pressed in the line edit widget if autocompletion is disabled
     *
     * It activates the text in the line edit
     */
    void slotReturnPressed();

    /**
     * @brief Slot called when the completion mode for the combo box changes
     *
     * This function connects the `KComboBox::returnPressed()` signal to the slotActivated() slot
     * if autocompletion has been disabled and disconnects it if it has been enabled
     * @param mode the new completion mode
     */
    void slotCompletionModeChanged(KCompletion::CompletionMode mode);

private:
    /**
     * @brief Updates the content of an item
     *
     * If neither the text nor the pixmap have changed, nothing is done.
     *
     * @param pix the new pixmap of the item
     * @param t the new text of the item
     * @param index the index of the item to update
     * @param title the new title of the item, to be stored as data
     */
    void updateItem(const QPixmap &pix, const QString &t, int index, const QString &title);

    /**
     * @brief Stores information about the current state of the combo box to restore them later
     *
     * This function stores the cursor position, current text, selected text and current index
     * of the combo box. This is called before doing operations which may change the state of the
     * combo box but that should be transparent to the user.
     */
    void saveState();

    /**
     * @brief Restores the state of the combo box using the information stored by saveState()
     *
     * This is called after doing operations which may change the state of the
     * combo box but that should be transparent to the user, so that they leave the combo box
     * as it were before.
     */
    void restoreState();

    /**
     * @brief Makes the temporary item permanent
     *
     * This is called before a new temporary item is created. It takes care to ensure that the
     * maximum number of items in the combo box is respected by removing the oldest ones, if needed
     */
    void applyPermanent();

    /**
     * @brief The text of the temporary item
     *
     * @return the text of the temporary item
     */
    QString temporaryItem() const
    {
        return itemText(temporary);
    }

    /**
     * @brief Removes from the combo box duplicates of the temporary item starting from the given index
     *
     * @note When deciding if an item is a duplicate, a trailing / is ignored
     *
     * @param index the index from which to start removing duplicates
     */
    void removeDuplicates(int index);

    bool m_returnPressed; //!< Whether the temporary item was activated since the last call to setURL()

    //WARNING: I'm not sure the API documentation for this variable is correct
    bool m_permanent; //!< Whether or not the first item in the combo box is permanent
    int m_cursorPos; //!< The saved cursor position, to be used by restoreState()
    int m_currentIndex; //!< The saved current index, to be used by restoreState()
    QString m_currentText; //!< The saved current text, to be used by restoreState()
    QString m_selectedText; //!< The saved selected text, to be used by restoreState()
    QPoint m_dragStart; //!< The starting position of a drag operation started on an item pixmap
    int m_pageSecurity; //!< The current page security

    /**
     * @brief Fills the given style option objects with information from this object
     *
     * Only the following fields are filled:
     * - `editable`
     * - `frame`
     * - `iconSize`
     * - `currentIcon`
     * - `currentText`
     * @param combo the object to fill
     */
    void getStyleOption(QStyleOptionComboBox *combo);

    static KConfig *s_config; //!< The configuration object where to save the history
    static const int temporary; //!< The index of the temporary item
};

#endif // KONQ_COMBO_H
