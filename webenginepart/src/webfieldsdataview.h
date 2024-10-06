/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <posta@stefanocrocco.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef WEBFIELDSDATAVIEW_H
#define WEBFIELDSDATAVIEW_H

#include <QTableView>
#include <QStandardItemModel>
#include <QStyledItemDelegate>

#include "wallet/webenginewallet.h"

/**
 * @brief Helper class to display passwords in WebFieldsDataWidget
 */
class WebFieldsDataViewPasswordDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:

    /**
     * @brief Default constructor
     *
     * @param parent the parent object
     */
    explicit WebFieldsDataViewPasswordDelegate(QObject *parent = nullptr);

    ///@brief Destructor
    ~WebFieldsDataViewPasswordDelegate() override{}

    /**
     * @brief Override of <a href="https://doc.qt.io/qt-5/qstyleditemdelegate.html#paint">QStyledItemDelegate::paint()</a>
     *
     * It displays the text replacing each character with a special character according to QStyle::StyleHint::SH_LineEdit_PasswordCharacter,
     * but only if the data contained in the index represents a password, according to WebEngineCustomizeCacheableFieldsDlg::PasswordRole
     *
     * @param painter the painter
     * @param option the option
     * @param index the index to paint
     */
    void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const override;

    /**
     * @brief Override of <a href="https://doc.qt.io/qt-5/qstyleditemdelegate.html#sizeHint">QStyledItemDelegate::sizeHint()</a>
     *
     * @param option the option
     * @param index the index whose size hint should be returned
     * @return the size hint for @p index
     */
    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const override;

private:

    /**
     * @brief Whether the given index represents a password
     *
     * This function uses the value of the WebEngineCustomizeCacheableFieldsDlg::PasswordRole to determine if @p idx represents or not a password
     * @param idx the index
     * @return @b true if @p idx represents and index and @b false otherwise
     */
    static bool isPassword(const QModelIndex &idx);

    /**
     * @brief The string to display in place of a password
     *
     * It returns a string with the same length as the Qt::DisplayRole of @p index all made by the character returned by <tt>QStyle::styleHint</tt>
     * called with argument QStyle::StyleHint::SH_LineEdit_PasswordCharacter.
     *
     * @param option the option to pass to QStyle::styleHint
     * @param index the index containing the password
     * @return a string suitable to be displayed instead of the display role of @p index
     */
    static QString passwordReplacement(const QStyleOptionViewItem &option, const QModelIndex &index);
};

/**
 * @brief Model which contains informations about web forms
 *
 * This model has several columns, which correspond to the values of the #Columns enum.
 *
 * The items in this model can be checkable or not, depending on how it's created see (WebFieldsDataModel()). This behaviour can't be
 * changed after creation.
 *
 * To fill the model, call setForms() with the forms to display
 */
class WebFieldsDataModel : public QStandardItemModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     *
     * @param checkableItems whether the items in the model should be checkable or not
     * @param parent the parent object
     */
    WebFieldsDataModel(bool checkableItems, QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~WebFieldsDataModel() override;

    ///@brief enum to reference the various columns of the table
    enum Columns {
        ChosenCol = 0,  ///< The column where the user can select the fields
        LabelCol, ///< The field name column
        ValueCol, ///< The field value column
        InternalNameCol, ///< The internal name column
        TypeCol, ///< The field type column
        IdCol, ///< The ID field type column
        DetailsCol ///< The field details column
    };

    ///@brief Special roles used by the model
    enum Roles {
        PasswordRole = Qt::UserRole+1, ///< Role which tells whether an index corresponds to a password or not
        FormRole, ///< Role which contains the index of the form a field belongs to inside m_fields
        FieldRole ///< Role which contains the index of the field in the form it belongs to
    };

    /**
     * @brief Fills the model with form data
     *
     * @note This will remove all previous content
     *
     * @param forms the forms to display in the model
     */
    void setForms(const WebEngineWallet::WebFormList &forms);

    /**
     * @brief The list of fields checked by the user
     *
     * @return a list of the fields the user has checked
     */
    WebEngineWallet::WebFormList checkedFields() const;

    /**
     * @brief Whether the items in the model can be checked or not
     *
     * @return @b true if the items in the model can be checked and @b false otherwise
     */
    bool areItemsCheckable() const {return m_checkableItems;}

    /**
     * @brief Remove the contents from the model
     */
    void clearForms();

private:

    /**
     * @brief Creates a row for the given field
     *
     * @param field the field to create the row for
     * @param formIndex the index of the form the field belongs to in #m_forms
     * @param fieldIndex the index of the field in the parent form's list of fields
     * @return The row corresponding to the field
     */
    QList<QStandardItem*> createRowForField(const WebEngineWallet::WebForm::WebField &field, int formIndex, int fieldIndex);

    /**
     * @brief A string to use as tool tip for the given field
     *
     * The tool tip contains information about all the field characteristics which aren't shown by default (internal name, id, type, read-only, enabled, autocomplete enabled).
     * @param field the field
     * @return a string with the tool tip
     */
    static QString toolTipForField(const WebEngineWallet::WebForm::WebField &field);

private:
    ///@brief Whether the items in the model should have a check box
    bool m_checkableItems;

    ///@brief the forms displayed in the model
    WebEngineWallet::WebFormList m_forms;
};

/**
 * @brief View to display informations about web fields
 *
 * This view is meant to be used with a WebFieldsDataModel, but this isn't a requirement. See setModel() if you use another model.
 *
 * This widget can be configured to:
 * * display only the label and value column or all columns provided by WebFieldsDataModel
 * * display or not tool tips for items
 * * display the password in clear text or replace each character with a dot
 */
class WebFieldsDataView : public QTableView
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     *
     * @param parent the parent widget
     */
    WebFieldsDataView( QWidget *parent = nullptr);

    ///@brief Destructor
    ~WebFieldsDataView() override;

    /**
     * @brief Override of `QTableView::setModel()`
     *
     * In most cases, @p model should be an instance of WebFieldsDataModel. If you want to use another model, you should know that this class assumes that the model
     * provides a column for each value of the WebFieldsDataModel::Columns enum and in that order. Besides, if you don't want the items to be checkable, you'll need
     * to hide the first column by hand (if @p model is a WebFieldsDataModel, this is done automatically based on WebFieldsDataModel::areItemsCheckable()).
     *
     * @param model the model
     */
    void setModel(QAbstractItemModel *model) override;

    /**
     * @brief Override of `QTableView::sizeHint()`
     *
     * @return a size hint whose height is exactly that needed to display all the rows and the header view (unless that's hidden)
     */
    QSize sizeHint() const override;

public slots:
    /**
     * @brief Slot which toggles displaying of passwords on or off
     *
     * @param show whether passwords should be displayed (@b true) or obfuscated (@b false) using WebEnginePartPasswordDelegate
     *
     * @see WebEnginePartPasswordDelegate
     */
    void togglePasswords(bool show);

    /**
     * @brief Shows or hides the columns displaying details about fields
     *
     * When details are hidden, the only visible information about fields are name and value; when details are visible, instead, also the type and
     * characteristics such as being read-only, enabled and having autocomplete on or off are shown
     *
     * @note Unlike setDetailsVisible(), this function does nothing if @p show is @b true and details are already visible or @p show is @b false
     * and details are already hidden
     *
     * @param show whether to show or hide details about fields
     */
    void toggleDetails(bool show);

    /**
     * @brief Enables or disables tool tips for items
     *
     * @param show @b true if the tool tips should be shown and @b false otherwise
     */
    void toggleToolTips(bool show);

    /**
     * @brief Shows or hides the columns displaying details about fields
     *
     * When details are hidden, the only visible information about fields are name and value; when details are visible, instead, also the type and
     * characteristics such as being read-only, enabled and having autocomplete on or off are shown
     *
     * @note Unlike toggleDetails(), this function doesn't check the current details visibility state
     *
     * @param visible whether to show or hide details about fields
     */
    void setDetailsVisible(bool visible);
protected:

    /**
     * @brief Override of `QTableView::viewportEvent()`
     *
     * It prevents displaying the tool tips if #m_showToolTips is false
     *
     * @param e the event
     * @return As `QTableView::viewportEvent()`
     */
    bool viewportEvent(QEvent * e) override;

private:

    ///@brief the delegate to draw passwords
    WebFieldsDataViewPasswordDelegate *m_passwordDelegate;

    ///@brief Whether passwords should be displayed in clear text or not
    bool m_showPasswords;

    ///@brief Whether all columns should be shown or only the name and value columns (and the column with the check box if the items are checkable) should be shown
    bool m_showDetails;

    ///@brief Whether tool tips should be shown
    bool m_showToolTips;
};

#endif // WEBFIELDSDATAVIEW_H


