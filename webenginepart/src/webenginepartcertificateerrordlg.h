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

#ifndef WEBENGINEPARTCERTIFICATEERRORDLG_H
#define WEBENGINEPARTCERTIFICATEERRORDLG_H

#include <QDialog>
#include <QWebEngineCertificateError>

class QAbstractButton;

namespace Ui
{
class WebEnginePartCertificateErrorDlg;
}

/**
 * @todo write docs
 */
class WebEnginePartCertificateErrorDlg : public QDialog
{
    Q_OBJECT

public:
    WebEnginePartCertificateErrorDlg(const QWebEngineCertificateError& error, QWidget* parent);
    ~WebEnginePartCertificateErrorDlg();
    enum class UserChoice{DontIgnoreError, IgnoreErrorOnce, IgnoreErrorForever};
    UserChoice userChoice() const;
    QWebEngineCertificateError certificateError() const;

private slots:
    void displayCertificate(int idx);
    void updateUserChoice(QAbstractButton *btn);

private:
    Ui::WebEnginePartCertificateErrorDlg *m_ui;
    QWebEngineCertificateError m_error;
    UserChoice m_choice;
};

#endif // WEBENGINEPARTCERTIFICATEERRORDLG_H
