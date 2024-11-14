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
     * @return the index corresponding to the source chosen by the user or an invalid index if the user chose to block
     * the page from capturing the screen
     */
    QModelIndex choice() const;

private Q_SLOTS:

    /**
     * @brief Enables or disables the OK button depending on whether or not the user chose a source
     */
    void updateOkStatus();

private:
    /**
     * @brief Helper function returning the current index of the combo box mapped to its original model
     * @return The value returned by calling `mapToSource` on #m_model passing it the current index in the combo box.
     * It will be one of #m_windowsModel, #m_screensModel or #m_defaultLineModel
     */
    QModelIndex currentSourceIndex() const;

private:
    QScopedPointer<Ui::CaptureSourceChooserDlg> m_ui; //!< The UI object
    QPointer<QAbstractListModel> m_windowsModel; //!< The model containing the list of possible windows to capture
    QPointer<QAbstractListModel> m_screensModel; //!< The model containing the list of possible screens to capture
    QStandardItemModel *m_defaultLineModel; //!< The model containing the default entry for the combo box
    QConcatenateTablesProxyModel *m_model; //!< The model which merges the contents of the other models to insert them in the combo box
};

}

#endif // WEBENGINE_CAPTURESOURCECHOOSERDLG_H
