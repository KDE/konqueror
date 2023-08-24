/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <posta@stefanocrocco.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "webfieldsdataview.h"

#include <KLocalizedString>

#include <QEvent>
#include <QHeaderView>

using WebForm = WebEngineWallet::WebForm;
using WebFormList = WebEngineWallet::WebFormList;
using WebField = WebEngineWallet::WebForm::WebField;
using WebFieldType = WebEngineWallet::WebForm::WebFieldType;

WebFieldsDataViewPasswordDelegate::WebFieldsDataViewPasswordDelegate(QObject* parent): QStyledItemDelegate(parent)
{
}

bool WebFieldsDataViewPasswordDelegate::isPassword(const QModelIndex& idx)
{
    return idx.data(WebFieldsDataModel::PasswordRole).toBool();
}

QString WebFieldsDataViewPasswordDelegate::passwordReplacement(const QStyleOptionViewItem& option, const QModelIndex& index)
{
        const QWidget *w = option.widget;
        QStyle *s = w->style();
        QChar passwdChar(s->styleHint(QStyle::StyleHint::SH_LineEdit_PasswordCharacter, &option, w));
        return QString(index.data().toString().length(), passwdChar);
}

void WebFieldsDataViewPasswordDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (!isPassword(index)) {
        QStyledItemDelegate::paint(painter, option, index);
    } else {
        QString str = passwordReplacement(option, index);
        option.widget->style()->drawItemText(painter, option.rect, index.data(Qt::TextAlignmentRole).toInt(), option.palette, true, str);
    }
}

QSize WebFieldsDataViewPasswordDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (!isPassword(index)) {
        return QStyledItemDelegate::sizeHint(option, index);
    } else {
        QString str = passwordReplacement(option, index);
        return option.widget->style()->itemTextRect(option.fontMetrics, option.rect, option.displayAlignment, true, str).size();
    }
}

WebFieldsDataModel::WebFieldsDataModel(bool checkableItems, QObject* parent): QStandardItemModel(parent), m_checkableItems(checkableItems)
{
    setHorizontalHeaderLabels({"",
        i18nc("Label of a web field", "Field name"),
        i18nc("Value of a web field", "Field value"),
        i18nc("Name attribute of a web field", "Internal field name"),
        i18nc("Type of a web field", "Field type"),
        i18nc("The id of a web field", "Field id"),
        i18nc("Other details about a web field", "Details")});
}

WebFieldsDataModel::~WebFieldsDataModel()
{
}

void WebFieldsDataModel::setForms(const WebEngineWallet::WebFormList &forms)
{
    m_forms = forms;
    removeRows(0, rowCount());
    for (int i = 0; i < m_forms.size(); ++i) {
        const WebForm &form = m_forms.at(i);
        for (int j = 0; j < form.fields.size(); ++j) {
            const WebField &field = form.fields.at(j);
            appendRow(createRowForField(field, i, j));
        }
    }
}

void WebFieldsDataModel::clearForms()
{
    m_forms.clear();
    removeRows(0, rowCount());
}

WebEngineWallet::WebFormList WebFieldsDataModel::checkedFields() const
{
    if (!m_checkableItems) {
        return {};
    }
    QMap<int, QVector<int>> fields;
    for (int i = 0; i < rowCount(); ++i) {
        QStandardItem *it  = item(i, ChosenCol);
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

QList<QStandardItem *> WebFieldsDataModel::createRowForField(const WebEngineWallet::WebForm::WebField& field, int formIndex, int fieldIndex)
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
    if (m_checkableItems) {
        row[ChosenCol]->setCheckable(true);
    }
    QString toolTip = toolTipForField(field);
    row[LabelCol]->setToolTip(toolTip);
    row[ValueCol]->setToolTip(toolTip);
    QStandardItem *chosen = row.at(ChosenCol);
    chosen->setData(formIndex, FormRole);
    chosen->setData(fieldIndex, FieldRole);
    return row;
}

QString WebFieldsDataModel::toolTipForField(const WebEngineWallet::WebForm::WebField& field)
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

WebFieldsDataView::WebFieldsDataView(QWidget* parent): QTableView(parent),
    m_passwordDelegate(new WebFieldsDataViewPasswordDelegate(this)), m_showPasswords(false), 
    m_showDetails(false), m_showToolTips(true)
{
    setItemDelegateForColumn(WebFieldsDataModel::ValueCol, m_passwordDelegate);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    verticalHeader()->hide();
}

WebFieldsDataView::~WebFieldsDataView()
{
}

void WebFieldsDataView::setModel(QAbstractItemModel* model)
{
    QTableView::setModel(model);
    setDetailsVisible(m_showDetails);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    WebFieldsDataModel *m = qobject_cast<WebFieldsDataModel*>(model);
    if (m) {
        setColumnHidden(WebFieldsDataModel::ChosenCol, !m->areItemsCheckable());
    }
}

void WebFieldsDataView::togglePasswords(bool show)
{
    if (m_showPasswords == show) {
        return;
    }
    m_showPasswords = show;
    setItemDelegateForColumn(WebFieldsDataModel::ValueCol, show ? itemDelegate() : m_passwordDelegate);
}

void WebFieldsDataView::toggleDetails(bool show)
{
    if (m_showDetails == show) {
        return;
    }
    setDetailsVisible(show);
}

void WebFieldsDataView::setDetailsVisible(bool visible)
{
    m_showDetails = visible;
    for (int i = WebFieldsDataModel::InternalNameCol; i <= WebFieldsDataModel::DetailsCol; ++i) {
        setColumnHidden(i, !visible);
    }
}

void WebFieldsDataView::toggleToolTips(bool show)
{
    m_showToolTips = show;
}

bool WebFieldsDataView::viewportEvent(QEvent* e)
{
    if (!m_showToolTips && (e->type() == QEvent::ToolTip || e->type() == QEvent::ToolTipChange)) {
        e->accept();
        return true;
    } else {
        return QTableView::viewportEvent(e);
    }
}

QSize WebFieldsDataView::sizeHint() const
{
    QSize hint = QTableView::sizeHint();
    int h = 2*frameWidth();
    if (horizontalHeader()->isVisible()) {
        h += horizontalHeader()->height();
    }
    if (model() && model()->rowCount() != 0) {
        h += rowHeight(0) * model()->rowCount();
    }
    hint.setHeight(h);
    return hint;
}
