/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <posta@stefanocrocco.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
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
