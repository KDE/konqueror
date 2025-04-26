/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2008 Eduardo Robles Elvira <edulix@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef __konq_sessiondlg_h__
#define __konq_sessiondlg_h__

#include <QDialog>

#include <QMap>
#include <QString>
#include <QUrl>

class KonqViewManager;
class KonqMainWindow;

/**
 * @brief Session management dialog
 *
 * This dialog allows the user to add, modify, rename and delete konqueror sessions.
 *
 * It contains a list of saved session names, buttons to operate on them, a check box
 * to decide whether opening tabs in the current window and buttons to close the dialog
 * or open one of the saved sessions.
 */
class KonqSessionDlg : public QDialog
{
    Q_OBJECT
public:
    /**
     * @brief Constructor
     * @param manager the view manager associated with the main window which asked for the dialog
     * @param parent the parent widget
     */
    explicit KonqSessionDlg(KonqViewManager *manager, QWidget *parent = nullptr);
    ~KonqSessionDlg() override; //!< Destructor

protected Q_SLOTS:
    /**
     * @brief Opens the selected session and closes the dialog
     */
    void slotOpen();

    /**
     * @brief Renames the selected section
     *
     * If no session is selected, this function does nothing.
     * @param the new name to give to the selected session. If empty, the user will
     * be shown a dialog to choose a new name
     */
    void slotRename(QUrl dirpathTo = QUrl());

    /**
     * @brief Creates a new session
     *
     * It shows the user a KonqNewSessionDlg dialog to create a new session
     */
    void slotNew();

    /**
     * @brief Deletes the selected session
     *
     * @note The user is **not** asked for confirmation
     * @note The session is deleted immediately, not when closing the dialog
     */
    void slotDelete();

    /**
     * @brief Shows a KonqNewSessionDlg to update the contents of the currently selected session
     *
     * If the user confirms the dialog, the old contents of the selected session will be
     * replaced with the contents of the current window (or windows).
     */
    void slotSave();

    /**
     * @brief Enables and disables buttons depending on whether the user selected a session
     */
    void slotSelectionChanged();

private:
    class KonqSessionDlgPrivate;
    KonqSessionDlgPrivate *const d; //!< The d-pointer
};

/**
 * @brief Dialog where the user can choose how to create or update a session
 *
 * In this dialog, the user can choose a name for the session and whether to save
 * the contents of only the current window or of all open Konqueror windows.
 *
 * The dialog can be used both to create a new session or to update an existing
 * session. The only difference is that in the second case the user won't be asked
 * for confirmation when overwriting an existing session.
 */
class KonqNewSessionDlg : public QDialog
{
    Q_OBJECT
public:
    /**
     * @brief Enum describing whether the dialog is being used to create a new session
     * or to update an existing session
     */
    enum Mode {
        NewFile, //!< The dialog is being used to create a new session
        ReplaceFile //!< The dialog is being used to update an existing session
    };

    /**
     * @brief Constructor
     * @param parent the parent widget
     * @param mainWindow the window which asked to show the dialog. If the user chooses
     * to only save the contents of the current window, this is the window whose
     * contents will be saved
     * @param sessionName the name to suggest for the new session. It can be empty
     * @param mode how the dialog should act
     */
    explicit KonqNewSessionDlg(QWidget *parent, KonqMainWindow *mainWindow, QString sessionName = QString(), Mode mode = NewFile);
    ~KonqNewSessionDlg() override; //!< Destructor

protected Q_SLOTS:
    /**
     * @brief Creates a new session according with the contents of the dialog
     *
     * This function asks the session manager to create a new session with the name
     * chosen by the user and the contents of the current window or of all the windows
     * depending on the user's choice.
     *
     * If the dialog is in NewFile mode and a session with the chosen name already
     * exists, the user is asked for confirmation before overwriting the existing
     * session.
     */
    void slotAddSession();

    /**
     * @brief Enables or disables the "Ok" button depending on whether a session
     * name has been chosen
     *
     * @param text the session name chosen by the user. If empty, the "Ok" button
     * will be disabled.
     */
    void slotTextChanged(const QString &text);
private:
    class KonqNewSessionDlgPrivate;
    KonqNewSessionDlgPrivate *const d; //!< The d-pointer
};

#endif
