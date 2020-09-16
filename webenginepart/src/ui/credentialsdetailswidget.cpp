/*
 * This file is part of the KDE project.
 *
 * Copyright 2020  Stefano Crocco <posta@stefanocrocco.it>
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

#include "credentialsdetailswidget.h"

CredentialsDetailsWidget::CredentialsDetailsWidget(QWidget* parent)
    : QWidget(parent), m_ui(new Ui::CredentialsDetailsWidget), m_model(new WebFieldsDataModel(false, this))
{
    m_ui->setupUi(this);
    m_ui->fields->setModel(m_model);
    m_ui->fields->toggleDetails(false);
    m_ui->fields->togglePasswords(false);
    m_ui->fields->toggleToolTips(false);
    m_ui->fields->horizontalHeader()->hide();
    connect(m_ui->showPasswords, &QCheckBox::toggled, m_ui->fields, &WebFieldsDataView::togglePasswords);
}

CredentialsDetailsWidget::~CredentialsDetailsWidget()
{
}

void CredentialsDetailsWidget::setForms(const WebEngineWallet::WebFormList& forms)
{
    m_model->setForms(forms);
}

void CredentialsDetailsWidget::clear()
{
    m_model->clearForms();
}
