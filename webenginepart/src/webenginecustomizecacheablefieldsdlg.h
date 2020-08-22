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

#ifndef WEBENGINECUSTOMIZECACHEABLEFIELDSDLG_H
#define WEBENGINECUSTOMIZECACHEABLEFIELDSDLG_H

#include <QDialog>
#include <QStyledItemDelegate>

#include "webenginewallet.h"

class QDialogButtonBox;
class QCheckBox;
class QTableView;
class QStandardItemModel;
class QStandardItem;

namespace Ui {
    class WebEngineCustomizeCacheableFieldsDlg;
};

/**
 * @brief Helper class to display passwords in WebEngineCustomizeCacheableFieldsDlg
 */

class WebEnginePartPasswordDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:

    /**
     * @brief Default constructor
     *
     * @param parent the parent object
     */
    explicit WebEnginePartPasswordDelegate(QObject *parent=nullptr);
    ~WebEnginePartPasswordDelegate(){}

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
 * @brief Dialog where the user can choose which fields in a web page should be saved in KWallet
 *
 * The most important part of the dialog is a table which displays information about the fields in the page. Each field has a check box which the user
 * can use to select which fields should be cached. The list of fields chosen by the user can be obtained using selectedFields().
 *
 * The dialog also contains three check box:
 * - a check box to toggle displaying details about each field
 * - a check box to toggle showing or hiding passwords
 * - a check box to request caching fields contents immediately after the dialog has been closed. This choice can be obtained using immediatelyCacheData().
 */
class WebEngineCustomizeCacheableFieldsDlg : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     *
     * @param forms a list of all the forms in the page
     * @param oldCustomization the previous custom configuration. Each key of the map must be the name of one form and the corresponding value must be a list of
     * the chosen fields in that form. This is used to decide which fields should be initially checked. If the map is empty, the initially selected fields will be decided automatically
     * @param parent the parent widget
     */
    WebEngineCustomizeCacheableFieldsDlg(const WebEngineWallet::WebFormList &forms, const QMap<QString, QStringList> &oldCustomization,  QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~WebEngineCustomizeCacheableFieldsDlg(){}

    /**
     * @brief Enum for special roles used by the table which displays the fields
     */
    enum Roles {
        PasswordRole = Qt::UserRole+1, ///< Role which tells whether an index corresponds to a password or not
        FormRole, ///< Role which contains the index of the form a field belongs to inside m_fields
        FieldRole ///< Role which contains the index of the field in the form it belongs to
    };

    /**
     * @brief The list of fields chosen by the user
     *
     * @return a list of the fields the user has checked
     */
    WebEngineWallet::WebFormList selectedFields() const;

    /**
     * @brief Whether the user has requested to immediately save the contents of the selected field
     *
     * If this is @b true, it means that the user has checked the check box asking to cache the fields contents as soon as the dialog is closed.
     * The code which created the dialog should take care to do so.
     * @return Whether fields contents should be cached immediately or not
     */
    bool immediatelyCacheData() const;

private slots:

    /**
     * @brief Slot which toggles displaying of passwords on or off
     *
     * @param show whether passwords should be displayed (@b true) or obfuscated (@b false) using WebEnginePartPasswordDelegate
     *
     * @see WebEnginePartPasswordDelegate
     */
    void toggleShowPasswords(bool show);

    /**
     * @brief Shows or hides the columns displaying details about fields
     *
     * When details are hidden, the only visible information about fields are name and value; when details are visible, instead, also the type and
     * characteristics such as being read-only, enabled and having autocomplete on or off are shown
     *
     * @param show whether to show or hide details about fields
     */
    void toggleDetails(bool show);

private:
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

    /**
     * @brief Fills the table with the fields information
     *
     * @param oldCustomization as in WebEngineCustomizeCacheableFieldsDlg()
     */
    void fillFieldTable(const QMap<QString, QStringList> &oldCustomization);

    /**
     * @brief Create a list of <a href="https://doc.qt.io/qt-5/qstandarditem.html">`QStandardItem`</a> representing the row with the information about a field
     *
     * @param field the field to describe in the row
     * @return a row containing information about @p field
     * @note This function doesn't set the FormRole and FieldRole for the newly created row and it doesn't check the first element. Callers of this function must do all of this themselves
     */
    QList<QStandardItem*> createRowForField(const WebEngineWallet::WebForm::WebField &field);

    /**
     * @brief A string to use as tool tip for the given field
     *
     * The tool tip contains information about all the field characteristics which aren't shown by default (internal name, id, type, read-only, enabled, autocomplete enabled).
     * @param field the field
     * @return a string with the tool tip
     */
    static QString toolTipForField(const WebEngineWallet::WebForm::WebField &field);

private:

    ///@brief A list of all the forms displayed in the dialog
    WebEngineWallet::WebFormList m_forms;

    ///@brief The model used by the table
    QStandardItemModel *m_model;

    ///@brief The delegate used to display obfuscated passwords
    WebEnginePartPasswordDelegate *m_passwordDelegate;

    ///@brief The ui object
    Ui::WebEngineCustomizeCacheableFieldsDlg *m_ui;
};

#endif // WEBENGINECUSTOMFORMFIELDSWALLETDLG_H
