/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <posta@stefanocrocco.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef CREDENTIALSDETAILSWIDGET_H
#define CREDENTIALSDETAILSWIDGET_H

#include <QWidget>
#include <QScopedPointer>

#include "ui_credentialsdetailswidget.h"
#include "wallet/webenginewallet.h"

class WebFieldsDataModel;

class CredentialsDetailsWidget : public QWidget
{
    Q_OBJECT

public:
    CredentialsDetailsWidget(QWidget* parent);
    ~CredentialsDetailsWidget() override;

    void setForms(const WebEngineWallet::WebFormList &forms);
    void clear();

private:
    QScopedPointer<Ui::CredentialsDetailsWidget> m_ui;
    WebFieldsDataModel *m_model;
};

#endif // CREDENTIALSDETAILSWIDGET_H
