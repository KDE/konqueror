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
