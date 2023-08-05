// /* This file is part of the KDE project
//     SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>
// 
//     SPDX-License-Identifier: LGPL-2.0-or-later
// */

#include "cookiealertdlg.h"
#include "ui_cookiealertdlg.h"

#include <QDateTime>
#include <QPushButton>
#include <QDialogButtonBox>

using namespace KonqInterfaces;

CookieAlertDlg::CookieAlertDlg(const QNetworkCookie &cookie, QWidget* parent) : QDialog(parent), m_ui(new Ui::CookieAlertDlg), m_cookie(cookie)
{
    m_ui->setupUi(this);
    m_ui->header->setText(m_ui->header->text().arg(m_cookie.domain()));
    m_ui->name->setText(m_cookie.name());
    m_ui->value->setText(m_cookie.value());
    QString expirationDate = m_cookie.expirationDate().isValid() ? m_cookie.expirationDate().toString() : i18nc("@label the cookie expires when the browser session ends", "End of Session");
    m_ui->expires->setText(expirationDate);
    m_ui->path->setText(cookie.path());
    m_ui->domain->setText(cookie.domain());
    QString sec;
    if (cookie.isSecure()) {
        if (cookie.isHttpOnly()) {
            sec = i18nc("@label exposure string - the cookie may only be used by https servers", "Secure servers only");
        } else {
            sec = i18nc("@label exposure string - the cookie may be used by https servers AND client-side javascripts", "Secure servers, page scripts");
        }
    } else {
        if (cookie.isHttpOnly()) {
            sec = i18nc("@label exposure string - the cookie may only be used by http servers", "Servers");
        } else {
            sec = i18nc("@label exposure string - the cookie may be used by http servers AND client-side javascripts", "Servers, page scripts");
        }
    }
    m_ui->security->setText(sec);
    m_acceptBtn = new QPushButton(i18nc("@label accept cookie", "Accept"), this);
    m_acceptForSessionBtn = new QPushButton(i18nc("@label accept cookie for this session only", "Accept for this session"), this);
    m_ui->buttons->addButton(m_acceptBtn, QDialogButtonBox::AcceptRole);
    m_ui->buttons->addButton(m_acceptForSessionBtn, QDialogButtonBox::AcceptRole);
    m_ui->buttons->button(QDialogButtonBox::Cancel)->setText(i18nc("@label reject cookie", "Reject"));

    connect(m_ui->buttons, &QDialogButtonBox::clicked, this, &CookieAlertDlg::setChoice);
}

CookieAlertDlg::~CookieAlertDlg() noexcept
{
}

void CookieAlertDlg::setChoice(QAbstractButton* btn)
{
    if (btn == m_acceptBtn) {
        m_choice = CookieJar::CookieAdvice::Accept;
    } else if (btn == m_acceptForSessionBtn) {
        m_choice = CookieJar::CookieAdvice::AcceptForSession;
    } else {
        m_choice = CookieJar::CookieAdvice::Reject;
    }
}

CookieJar::CookieAdvice CookieAlertDlg::choice() const
{
    return m_choice;
}

CookieAlertDlg::ApplyTo CookieAlertDlg::applyTo() const
{
    if (m_ui->applyToAll->isChecked()) {
        return Cookies;
    } else if (m_ui->applyToDomain->isChecked()) {
        return Domain;
    } else {
        return This;
    }
}

