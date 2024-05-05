/*
 * This file is part of the KDE project.
 *
 * Copyright 2021  Stefano Crocco <posta@stefanocrocco.it>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "webenginepartcertificateerrordlg.h"
#include "ui_webenginepartcertificateerrordlg.h"


#include <KSslCertificateBox>

#include <QAbstractButton>
#include <QPushButton>

using namespace KonqWebEnginePart;

WebEnginePartCertificateErrorDlg::WebEnginePartCertificateErrorDlg(const QWebEngineCertificateError &error, QWidget* parent):
    QDialog(parent),
    m_ui(new Ui::WebEnginePartCertificateErrorDlg), m_error(error), m_choice(UserChoice::DontIgnoreError)
{
    m_ui->setupUi(this);
    connect(m_ui->buttons, &QDialogButtonBox::clicked, this, &WebEnginePartCertificateErrorDlg::updateUserChoice);
    connect(m_ui->showDetails, &QCheckBox::toggled, m_ui->details, &QWidget::setVisible);
    connect(m_ui->certificateChain, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &WebEnginePartCertificateErrorDlg::displayCertificate);

    m_ui->buttons->button(QDialogButtonBox::No)->setDefault(true);
    m_ui->buttons->button(QDialogButtonBox::Yes)->setText(i18nc("Ignore the certificate error for this URL only for now", "Yes, &once"));
    m_ui->buttons->button(QDialogButtonBox::YesToAll)->setText(i18nc("Ignore the certificate error for this URL now and in the future", "Yes, &forever"));
    m_ui->details->hide();

    //According to the documentation, QWebEngineCertificateError::description() returns a localized string
    QString translatedDesc = m_error.description().toUtf8();
    QString text = i18n("<p>The server <tt>%1</tt> failed the authenticity check. The error is:</p><p><tt>%2</tt></p>Do you want to ignore this error?",
                        m_error.url().host(), translatedDesc);
    m_ui->label->setText(text);
    for (const QSslCertificate &cert : m_error.certificateChain()) {
        m_ui->certificateChain->addItem(cert.subjectDisplayName());
    }
    setWindowTitle(i18nc("title of a dialog asking what to do about a SSL certificate error", "Certificate error"));
}

WebEnginePartCertificateErrorDlg::~WebEnginePartCertificateErrorDlg()
{
}

QWebEngineCertificateError WebEnginePartCertificateErrorDlg::certificateError() const
{
    return m_error;
}

WebEnginePartCertificateErrorDlg::UserChoice WebEnginePartCertificateErrorDlg::userChoice() const
{
    return m_choice;
}

void WebEnginePartCertificateErrorDlg::displayCertificate(int idx)
{
    m_ui->subjectData->setCertificate(m_error.certificateChain().at(idx), KSslCertificateBox::Subject);;
    m_ui->issuerData->setCertificate(m_error.certificateChain().at(idx), KSslCertificateBox::Issuer);;
}

void WebEnginePartCertificateErrorDlg::updateUserChoice(QAbstractButton* btn)
{
    QDialogButtonBox::StandardButton code = m_ui->buttons->standardButton(btn);
    switch (code) {
        case QDialogButtonBox::Yes:
            m_choice = UserChoice::IgnoreErrorOnce;
            break;
        case QDialogButtonBox::YesToAll:
            m_choice = UserChoice::IgnoreErrorForever;
            break;
        default:
            m_choice = UserChoice::DontIgnoreError;
    }
}
