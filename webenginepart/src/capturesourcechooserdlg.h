/*
  This file is part of the KDE project
  SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>

  SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef WEBENGINE_CAPTURESOURCECHOOSERDLG_H
#define WEBENGINE_CAPTURESOURCECHOOSERDLG_H

#include <QDialog>
#include <QScopedPointer>
#include <QAbstractListModel>
#include <QPointer>

class QConcatenateTablesProxyModel;
class QStandardItemModel;

namespace WebEngine {

namespace Ui
{
class CaptureSourceChooserDlg;
}

/**
 * @brief Dialog which allows the user to choose the source when a web page asks to capture the screen
 *
 * This dialog is supposed to be called from a slot connected to the `WebEnginePage::desktopMediaRequested()` signal.
 *
 * @todo Once the Qt bug https://bugreports.qt.io/browse/QTBUG-136111 has been fixed, change the dialog so that there's
 * a single combo box containing both windows and screens, using `QConcatenateTablesProxyModel` and a single Ok button.
 */
class CaptureSourceChooserDlg : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param url The URL of the page requesting the capture. It's only used to show the user who
     * is asking to capture the screen
     * @param windowsModel a model containing information on the available windows
     * @param screensModel a model containing information on the available screens
     * @param parent the parent widget
     */
    CaptureSourceChooserDlg(const QUrl& url, QAbstractListModel* windowsModel, QAbstractListModel* screensModel, QWidget* parent);

    /**
     * @brief Destructor
     */
    ~CaptureSourceChooserDlg();

    /**
     * @brief The capture source chosen by the user
     *
     * To find out whether the user chose a window or the whole screen, callers must compare the `model()` of the returned
     * index with the two models passed to the constructor.
     *
     * @warning The user decides what to share by closing the dialog using one of the `Share Screen`, `Share Window` or
     * `Share None` buttons. Before that happens, this function behaves as if the user had chosen to share nothing.
     *
     * @return the index corresponding to the source chosen by the user or an invalid index if the user chose to block
     * the page from capturing the screen
     */
    QModelIndex choice() const;

private Q_SLOTS:

    /**
     * @brief Enables or disables the `Share Window` and `Share Screen` buttons depending on the combo boxes status
     */
    void updateShareBtnStatus();

private:

    /**
     * @brief Enum describing what the user chose to share
     */
    enum class Choice {
        ShareWindow, //!< The user chose to share a window
        ShareScreen, //!< The user chose to share a screen
        ShareNone //!< The user chose not to share anything
    };

private:
    Choice m_choice = Choice::ShareNone; //!< The choice made by the user when closing the dialog
    QScopedPointer<Ui::CaptureSourceChooserDlg> m_ui; //!< The UI object
    QPointer<QAbstractListModel> m_windowsModel; //!< The model containing the list of possible windows to capture
    QPointer<QAbstractListModel> m_screensModel; //!< The model containing the list of possible screens to capture
    QPointer<QPushButton> m_shareScreenBtn; //!< The button to close the dialog choosing to share a screen
    QPointer<QPushButton> m_shareWindowBtn; //!< The button to close the dialog choosing to share a window
};

}

#endif // WEBENGINE_CAPTURESOURCECHOOSERDLG_H
