/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2021 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef WEBENGINEPART_CERTIFICATEERRORDIALOGMANAGER_H
#define WEBENGINEPART_CERTIFICATEERRORDIALOGMANAGER_H

#include <QObject>
#include <QWebEngineCertificateError>
#include <QPointer>

class WebEnginePage;

namespace KonqWebEnginePart {

    class WebEnginePartCertificateErrorDlg;

    /**
     * @brief Class which takes care of queuing dialogs reporting certificate errors.
     *
     * Since calls to WebEnginePage::certificateError are made asynchronously, without
     * queuing it could happen that a second dialog is shown when there's another one
     * still visible. Since the dialogs are modal and they can cover each other, this can
     * cause issues for the user. To avoid this, this class ensures that only dialogs from
     * different windows can be shown at the same time.
     *
     * @internal The whole issue is caused by the use of @c QDialog::open instead of
     * @c QDialog::exec to display the dialog: since @c exec is synchronous, it will prevent
     * displaying a second dialog before the first has been closed.
     */
    class CertificateErrorDialogManager : public QObject
    {
        Q_OBJECT

    public:
        /**
         * @brief Constructor
         *
         * @param parent the parent object
         */
        CertificateErrorDialogManager(QObject *parent=nullptr);

        /**
         * @brief Destructor
         *
         */
        ~CertificateErrorDialogManager();

        /**
         * @brief Displays a dialog regarding the certificate error or queues the certificate error
         *
         * If there's no currently displayed certificate error dialog for the window containing the page,
         * the dialog for the new certificate is immediately shown, otherwise the certificate error
         * is put in queue waiting to be displayed as soon as there aren't other dialogs for that
         * window.
         *
         * In all cases, the appropriate function is called on the certificate: @c ignoreCertificateError
         * or @c rejectCertificate if the dialog was displayed immediately or @c defer if it was queued.
         * @param _ce the certificate error
         * @param page the page which caused the certificate error
         * @return @b true if the certificate was overridable and @b false if it wasn't overridable.
         * @note In case the certificate wasn't overridable, @c rejectCertificate will be called on the
         * certificate.
         * @note In Qt6 the return value is ignored
         * @todo When removing compatibility with KF5, change the return value to `void`
         */
        bool handleCertificateError(const QWebEngineCertificateError  &_ce, WebEnginePage *page);

    private:

        /**
         * @brief Struct used to encapuslate information about certificate errors
         */
        struct CertificateErrorData {
            /**
             * @brief The certificate error
             */
            QWebEngineCertificateError error;
            /**
             * @brief The page which has caused the certificate error
             */
            QPointer<WebEnginePage> page;
        };

        /**
         * @brief Whether the decision to always ignore the given error is recorded in the user settings
         *
         * @param ce the certificate error
         * @return @b true if the user has decided to always ignore the error and @b false otherwise
         */
        static bool userAlreadyChoseToIgnoreError(const QWebEngineCertificateError  &ce);

        /**
         * @brief Records the user's choice to forever ignore the given error
         *
         * @param ce the error
         */
        static void recordIgnoreForeverChoice(const QWebEngineCertificateError &ce);

        /**
         * @brief The window (toplevel widget) associated to the given page
         *
         * @param page the page to retrieve the window for
         * @return the window associated with @p page or @b nullptr if either @p page or its @c view are @b nullptr
         */
        static QWidget* windowForPage(WebEnginePage *page);

        /**
         * @brief Displays a dialog for the given certificate error unless a certificate error dialog
         * for the same window is already visible.
         *
         * @param data the data describing the certificate error
         * @return @b true if the dialog was displayed and @b false if it wasn't
         * @note the dialog is displayed asynchronously
         */
        bool displayDialogIfPossible(const CertificateErrorData &data);

        /**
         * @brief Displays a dialog for the given certificate error
         *
         * The dialog is displayed asynchronously (using @c QDialog::open). The dialog result is used
         * in dialogClosed.
         *
         * @warning This function doesn't check whether a dialog for the given window
         * is already visible: it assumes that such check has been done by the caller.
         * @warning This function @b doesn't use windowForPage to determine which window the page belongs to:
         * it always uses @p window.
         *
         * @internal This function stores the new dialog and the corresponding window in #m_dialogs
         *
         * @param data the data describing the certificate error
         * @param window the window the dialog should be displayed in
         */
        void displayDialog(const CertificateErrorData &data, QWidget *window);


    private slots:
        /**
         * @brief Removes a dialog from the list and displays the next dialog (if any) for the same window
         *
         * @param obj the dialog to remove. It is a @c QObject rather than a WebEnginePartCertificateErrorDlg
         * because this slot is called in response to the dialog @c destroyed signal, which has a @c QObject* as argument
         */
        void removeDestroyedDialog(QObject *dlg);

        /**
         * @brief Removes any dialog from the given window from the list
         *
         * @param obj the window. It is a @c QObject rather than a @c QWidget
         * because this slot is called in response to the window @c destroyed signal, which has a @c QObject* as argument
         */
        void removeDestroyedWindow(QObject *window);

        /**
         * @brief Applies the choice made by the user in the given dialog
         *
         * This function is called in response to the dialog's @b finished signal.
         * It calls @c ignoreCertificateError or @c rejectCertificate on the certificate error (retrieved using
         * WebEnginePartCertificateErrorDlg::certificateError) and, if the user chose to always ignore the error,
         * records the choice.
         *
         * This function also deletes the dialog (using @c deleteLater).
         *
         * @note This function @b doesn't display the next dialog for the associated window: that task is left to
         * removeDestroyedDialog, which is called in response to the dialog destruction.
         *
         * @param dlg the dialog
         */
        void applyUserChoice(WebEnginePartCertificateErrorDlg *dlg);

        /**
         * @brief Displays the certificate error dialog for the next error caused by a page associated with the given window
         *
         * If no such certificate error exists, nothing is done.
         *
         * @param window the window to display the next error for.
         */
        void displayNextDialog(QWidget *window);

    private:

        /**
         * @brief A list of all the queued certificate errors together with the corresponding pages
         *
         * When a dialog for a certificate error is created, the corresponding entry is removed from the list
         * @internal The entry is removed when the dialog is created, not when it's destroyed
         *
         */
        QVector<CertificateErrorData> m_certificates;

        /**
         * @brief A hash associating each certificate error dialog with its window
         *
         * If the window is destroyed, the corresponding entry is removed from the list.
         * @note Keys and values are @c QObject* rather than more specific types because they're used
         * in slots connected with @c QObject::destroyed signals, whose argument is a @c QObject*.
         */
        QHash<QObject*, QObject*> m_dialogs;

    };

}

#endif // WEBENGINEPART_CERTIFICATEERRORDIALOGMANAGER_H
