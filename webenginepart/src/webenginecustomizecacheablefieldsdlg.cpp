/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <posta@stefanocrocco.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "webenginecustomizecacheablefieldsdlg.h"

#include <QDialogButtonBox>
#include <QStandardItem>
#include <QCheckBox>

#include "ui_webenginecustomizecacheablefieldsdlg.h"
#include "webfieldsdataview.h"

using WebForm = WebEngineWallet::WebForm;
using WebFormList = WebEngineWallet::WebFormList;
using WebField = WebEngineWallet::WebForm::WebField;
using WebFieldType = WebEngineWallet::WebForm::WebFieldType;

WebEngineCustomizeCacheableFieldsDlg::WebEngineCustomizeCacheableFieldsDlg(const WebEngineWallet::WebFormList &forms, const OldCustomizationData &oldCustomization, QWidget* parent):
    QDialog(parent), m_model(new WebFieldsDataModel(true, this)),
    m_ui(new Ui::WebEngineCustomizeCacheableFieldsDlg)
{
    m_ui->setupUi(this);
    connect(m_ui->showPasswords, &QCheckBox::toggled, m_ui->fields, &WebFieldsDataView::togglePasswords);
    connect(m_ui->showDetails, &QCheckBox::toggled, m_ui->fields, &WebFieldsDataView::toggleDetails);
    m_model->setForms(forms);
    addChecksToPreviouslyChosenItems(forms, oldCustomization);
    m_ui->fields->setModel(m_model);
    m_ui->fields->horizontalHeader()->setStretchLastSection(true);
    m_ui->fields->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_ui->fields->toggleDetails(m_ui->showDetails->isChecked());
}

WebFormList WebEngineCustomizeCacheableFieldsDlg::selectedFields() const
{
    return m_model->checkedFields();
}

bool WebEngineCustomizeCacheableFieldsDlg::immediatelyCacheData() const
{
    return m_ui->immediatelyCacheData->isChecked();
}

void WebEngineCustomizeCacheableFieldsDlg::addChecksToPreviouslyChosenItems(const WebEngineWallet::WebFormList &forms, const OldCustomizationData &data)
{
    bool autoCheck = data.isEmpty();
    int row = 0;
    for (int i = 0; i < forms.length(); ++i) {
        const WebForm &form = forms.at(i);
        QStringList oldCustomInThisForm = data.value(form.name);
        for (int j = 0; j < form.fields.length(); ++j) {
            WebField field = form.fields.at(j);
            QStandardItem *chosen = m_model->item(row,  WebFieldsDataModel::ChosenCol);
            bool checked = false;
            if (autoCheck) {
                checked = !field.value.isEmpty() && !field.readOnly && !field.disabled && field.autocompleteAllowed;
            } else {
                checked = oldCustomInThisForm.contains(field.name);
            }
            chosen->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
            ++row;
        }
    }
}
