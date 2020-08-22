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
#include <QLayout>
#include <QTableView>
#include <QStandardItemModel>
#include <QStandardItem>

#include "ui_webenginecustomizecacheablefieldsdlg.h"

using WebForm = WebEngineWallet::WebForm;
using WebFormList = WebEngineWallet::WebFormList;
using WebField = WebEngineWallet::WebForm::WebField;
using WebFieldType = WebEngineWallet::WebForm::WebFieldType;

WebEnginePartPasswordDelegate::WebEnginePartPasswordDelegate(QObject* parent): QStyledItemDelegate(parent)
{
}

bool WebEnginePartPasswordDelegate::isPassword(const QModelIndex& idx)
{
    return idx.data(WebEngineCustomizeCacheableFieldsDlg::PasswordRole).toBool();
}

QString WebEnginePartPasswordDelegate::passwordReplacement(const QStyleOptionViewItem& option, const QModelIndex& index)
{
        const QWidget *w = option.widget;
        QStyle *s = w->style();
        QChar passwdChar = s->styleHint(QStyle::StyleHint::SH_LineEdit_PasswordCharacter, &option, w);
        return QString(index.data().toString().length(), passwdChar);
}

void WebEnginePartPasswordDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (!isPassword(index)) {
        QStyledItemDelegate::paint(painter, option, index);
    } else {
        QString str = passwordReplacement(option, index);
        option.widget->style()->drawItemText(painter, option.rect, index.data(Qt::TextAlignmentRole).toInt(), option.palette, true, str);
    }
}

QSize WebEnginePartPasswordDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (!isPassword(index)) {
        return QStyledItemDelegate::sizeHint(option, index);
    } else {
        QString str = passwordReplacement(option, index);
        return option.widget->style()->itemTextRect(option.fontMetrics, option.rect, option.displayAlignment, true, str).size();
    }
}

WebEngineCustomizeCacheableFieldsDlg::WebEngineCustomizeCacheableFieldsDlg(const WebEngineWallet::WebFormList &forms, const QMap<QString, QStringList> &oldCustomization, QWidget* parent):
    QDialog(parent), m_forms(forms), m_model(new QStandardItemModel(this)), m_passwordDelegate(new WebEnginePartPasswordDelegate(this)),
    m_ui(new Ui::WebEngineCustomizeCacheableFieldsDlg)
{
    m_ui->setupUi(this);
    connect(m_ui->showPasswords, &QCheckBox::toggled, this, &WebEngineCustomizeCacheableFieldsDlg::toggleShowPasswords);
    connect(m_ui->showDetails, &QCheckBox::toggled, this, &WebEngineCustomizeCacheableFieldsDlg::toggleDetails);
    fillFieldTable(oldCustomization);
}

QList<QStandardItem *> WebEngineCustomizeCacheableFieldsDlg::createRowForField(const WebEngineWallet::WebForm::WebField& field)
{
    QString type = WebForm::fieldNameFromType(field.type, true);
    QStringList notes;
    if (field.readOnly) {
        notes << i18nc("web field has the readonly attribute", "read only");
    }
    if (!field.autocompleteAllowed) {
        notes << i18nc("web field has the autocomplete attribute set to off", "auto-completion off");
    }
    if (field.disabled) {
        notes << i18nc("web field is disabled", "disabled");
    }
    QString label = !field.label.isEmpty() ? field.label : field.name;
    QStringList contents{QString(), label, field.value, field.name, type, field.id, notes.join(", ")};
    QList<QStandardItem*> row;
    row.reserve(contents.size());
    auto itemFromString = [](const QString &s){
        QStandardItem *it = new QStandardItem(s);
        it->setTextAlignment(Qt::AlignCenter);
        return it;
    };
    std::transform(contents.constBegin(), contents.constEnd(), std::back_inserter(row), itemFromString);
    row[ValueCol]->setData(field.type == WebFieldType::Password, PasswordRole);
    row[ChosenCol]->setCheckable(true);
    QString toolTip = toolTipForField(field);
    row[LabelCol]->setToolTip(toolTip);
    row[ValueCol]->setToolTip(toolTip);
    return row;
}

void WebEngineCustomizeCacheableFieldsDlg::fillFieldTable(const QMap<QString, QStringList> &oldCustomization)
{
    bool autoCheck = oldCustomization.isEmpty();
    m_ui->fields->setModel(m_model);
    m_ui->fields->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_model->setHorizontalHeaderLabels(QStringList{"",
        i18nc("Label of a web field", "Field name"),
        i18nc("Value of a web field", "Field value"),
        i18nc("Name attribute of a web field", "Internal field name"),
        i18nc("Type of a web field", "Field type"),
        i18nc("The id of a web field", "Field id"),
        i18nc("Other details about a web field", "Details")});
    m_ui->fields->setItemDelegateForColumn(2, m_passwordDelegate);
    for (int i = 0; i < m_forms.length(); ++i) {
        const WebForm &form = m_forms.at(i);
        QStringList oldCustomInThisForm = oldCustomization.value(form.name);
        for (int j = 0; j < form.fields.length(); ++j) {
            WebField field = form.fields.at(j);
            QList<QStandardItem*> row = createRowForField(field);
            QStandardItem *chosen = row.at(ChosenCol);
            chosen->setData(i, FormRole);
            chosen->setData(j, FieldRole);
            bool checked = false;
            if (autoCheck) {
                checked = !field.value.isEmpty() && !field.readOnly && !field.disabled && field.autocompleteAllowed;
            } else {
                checked = oldCustomInThisForm.contains(field.name);
            }
            chosen->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
            m_model->appendRow(row);
        }
    }
    m_ui->fields->verticalHeader()->hide();
    m_ui->fields->horizontalHeader()->setStretchLastSection(true);
    m_ui->fields->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    toggleDetails(m_ui->showDetails->isChecked());
}

WebFormList WebEngineCustomizeCacheableFieldsDlg::selectedFields() const
{
    QMap<int, QVector<int>> fields;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        QStandardItem *it  = m_model->item(i, ChosenCol);
        if (it->checkState() == Qt::Checked) {
            fields[it->data(FormRole).toInt()].append(it->data(FieldRole).toInt());
        }
    }
    WebFormList lst;
    for (QMap<int, QVector<int>>::const_iterator it = fields.constBegin(); it != fields.constEnd(); ++it) {
        if (it.value().isEmpty()) {
            continue;
        }
        const WebForm &oldForm = m_forms.at(it.key());
        WebForm form(oldForm);
        form.fields.clear();
        for (int i : it.value()) {
            form.fields.append(oldForm.fields.at(i));
        }
        lst.append(form);
    }
    return lst;
}

void WebEngineCustomizeCacheableFieldsDlg::toggleDetails (bool show)
{
    for (int i = InternalNameCol; i <= DetailsCol; ++i) {
        m_ui->fields->setColumnHidden(i, !show);
    }
}

void WebEngineCustomizeCacheableFieldsDlg::toggleShowPasswords (bool show)
{
    //Do nothing if the item delegate setting is already correct. This should never happen
    if (show == (m_ui->fields->itemDelegateForColumn(ValueCol) == m_ui->fields->itemDelegate())) {
        return;
    }
    QAbstractItemDelegate *delegate = show ? m_ui->fields->itemDelegate() : m_passwordDelegate;
    m_ui->fields->setItemDelegateForColumn(ValueCol, delegate);
}

bool WebEngineCustomizeCacheableFieldsDlg::immediatelyCacheData() const
{
    return m_ui->immediatelyCacheData->isChecked();
}

QString WebEngineCustomizeCacheableFieldsDlg::toolTipForField(const WebEngineWallet::WebForm::WebField &field)
{
    QString type = WebForm::fieldNameFromType(field.type, true);
    const QString yes = i18nc("A statement about a field is true", "yes");
    const QString no = i18nc("A statement about a field is false", "no");
    auto boolToYesNo = [yes, no](bool val){return val ? yes : no;};
    QString toolTip = i18n(
        "<ul><li><b>Field internal name: </b>%1</li>"
        "<li><b>Field type: </b>%2</li>"
        "<li><b>Field id: </b>%3</li>"
        "<li><b>Field is read only: </b>%4</li>"
        "<li><b>Field is enabled: </b>%5</li>"
        "<li><b>Autocompletion is enabled: </b>%6</li>"
        "</ul>",
        field.name, type, field.id, boolToYesNo(field.readOnly), boolToYesNo(!field.disabled), boolToYesNo(field.autocompleteAllowed));
    return toolTip;
}
