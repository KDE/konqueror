// /* This file is part of the KDE project
//     SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>
// 
//     SPDX-License-Identifier: LGPL-2.0-or-later
// */

#ifndef COOKIEALERTDLG_H
#define COOKIEALERTDLG_H

#include "interfaces/cookiejar.h"

#include <QDialog>
#include <QNetworkCookie>

namespace Ui
{
class CookieAlertDlg;
}

class QPushButton;
class QAbstractButton;

/**
 * @brief Dialog asking the user what to do with a cookie
 *
 * The choices given by this dialog are:
 * - accept the cookie
 * - accept the cookie, but keeping it for this session only
 * - reject the cookie
 *
 * The user can also decide to have the above choice the default for all cookies
 * or for all cookies with the same domain as the original cookie.
 */
class CookieAlertDlg : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     *
     * @param cookie the cookie to decide what to do with
     * @param parent the parent widget
     */
    CookieAlertDlg(const QNetworkCookie &cookie, QWidget* parent=nullptr);

    ~CookieAlertDlg(); ///< Destructor

    /**
     * @brief What to do with the cookie
     *
     * @return one of the following values, according to the user's choice
     * - KonqInterfaces::CookieJar::CookieAdvice::Accept
     * - KonqInterfaces::CookieJar::CookieAdvice::AcceptForSession
     * - KonqInterfaces::CookieJar::CookieAdvice::Reject
     */
    KonqInterfaces::CookieJar::CookieAdvice choice() const;

    /**
     * @brief Enum describing which cookies should the user's choice be applied to
     */
    enum ApplyTo {
        This, ///< The choice must only be applied to the cookie described in the dialog
        Domain, ///< The choice must be applied to all cookies with the same domain as the one described in the dialog
        Cookies ///< The choice must become the default policy for all cookies
    };

    /**
     * @brief To which cookies should the user's choice be applied to
     * @return an enum describing to which cookies the user decided to apply his choice
     */
    ApplyTo applyTo() const;

private slots:
    /**
     * @brief Slot called when one of the dialog buttons is clicked
     *
     * It sets the appropriate value for #m_choice
     */
    void setChoice(QAbstractButton *btn);

private:
    Ui::CookieAlertDlg* m_ui; ///< The object representing the UI
    QPushButton* m_acceptBtn; ///< The button to accept the cookie
    QPushButton* m_acceptForSessionBtn; ///< The button to accept the cookie only for this session
    QNetworkCookie m_cookie; ///< The cookie the dialog is about
    KonqInterfaces::CookieJar::CookieAdvice m_choice = KonqInterfaces::CookieJar::CookieAdvice::Reject; ///< The choice made by the user
};

#endif // COOKIEALERTDLG_H
