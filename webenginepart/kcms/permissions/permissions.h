/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef PERMISSIONS_H
#define PERMISSIONS_H

#include <KCModule>

#include <QDialog>
#include <QScopedPointer>
#include <QWebEnginePermission>
#include <QStyledItemDelegate>

class QStandardItemModel;
class QStandardItem;
class QRadioButton;
class QButtonGroup;
class PermissionStateDelegate;

namespace Ui
{
class Permissions;
}

/**
 * @brief KCM which allows the user to check existing web permissions and to set the default
 * permission policy
 */
class Permissions: public KCModule
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     *
     * @param parent the parent widget
     * @param md as in `KCModule` constructor
     * @param args as in `KCModule` constructor
     */
    Permissions(QObject *parent, const KPluginMetaData &md={});

    /**
     * @brief Destructor
     */
    ~Permissions();

    /**
     * @brief Loads the settings from the configuration files
     */
    void load() override;

    /**
     * @brief Resets the KCM to its default values
     */
    void defaults() override;

    /**
     * @brief Saves the user settings
     *
     */
    void save() override;

    using MaybePermission = std::optional<QWebEnginePermission>; //An alias for an optional containing a `QWebEnginePermission` object

    /**
     * @brief The permission object associated with a row in the model
     *
     * @param idx one of the indexes of the row. Its column doesn't matter, as the
     * permission is always stored in the first column.
     *
     * @return the permission object contained in the index with the same row and
     * parent as @p idx but in the first columnn or `std::nullopt` if that index doesn't
     * contain a permission object (for example because it's a toplevel index)
     */
    MaybePermission permissionForRow(const QModelIndex idx) const;

    using MaybeState = std::optional<QWebEnginePermission::State>;

    /**
     * @brief The state of the permission associated with the given row
     *
     * @param idx one of the indexes in the row
     * @return The state of the permission associated with the row. If the user
     * changed it in the view, the returned state will be the one set by the user,
     * not the one returned by `QWebEngineProfile`. If the row containing @p idx
     * doesn't have an associated permission (usually because it's a toplevel row),
     * `std::nullopt` is returned
     */
    MaybeState stateForRow(const QModelIndex &idx) const;

    /**
     * @brief Changes the state of the permission associated with a given index
     *
     * This doesn't change the state of the permission object, but only the state
     * associated with it in #m_states. The permission object will only be updated
     * when save() is called.
     *
     * @param idx an index in the row corresponding to the permission to change
     * @param newState the new state for the permission
     */
    void setPermissionState(const QModelIndex &idx, QWebEnginePermission::State newState);

    /**
     * @brief The user-friendly name of a permission type
     *
     * @param type the type of the permission
     * @return the user-friendly name of @p type
     */
    static QString permissionTypeName(QWebEnginePermission::PermissionType type);

    /**
     * @brief The tooltip text describing a permission type
     *
     * @param type the type of the permission
     * @return The tooltip text describing a @p type
     */
    static QString permissionTypeToolTip(QWebEnginePermission::PermissionType type);

    /**
     * @brief The user-friendly name of a permission state
     *
     * @param state the permission state
     * @param type the type of permission the state refers to. It determines the returned string
     * in case @p state is `Ask`, since that state has different meaning for persistent and not
     * persistent types
     * @return the user-friendly name for @p state
     */
    static QString permissionStateName(QWebEnginePermission::State state, QWebEnginePermission::PermissionType type);

    /**
     * @brief A tooltip text describing a permission state
     *
     * @param state the permission state
     * @param type the type of permission the state refers to. It determines the returned string
     * in case @p state is `Ask`, since that state has different meaning for persistent and not
     * persistent types
     * @return A tooltip text describing @p state
     */
    static QString permissionStateToolTip(QWebEnginePermission::State state, QWebEnginePermission::PermissionType type);

private:

    /**
     * @brief Reads the list of permissions from the default profile
     *
     * This fills the #m_states hash
     */
    void readPermissions();

    /**
     * @brief Fills the permissions model grouping permissions according to their type
     *
     * The tree has one toplevel entry for each permission type. Each toplevel entry
     * has one entry for each URL which asked that type of permission.
     */
    void fillModelByType();

    /**
     * @brief Fills the permissions model grouping permissions according to their URL
     *
     * The tree has one toplevel entry for each URL (origin) . Each toplevel entry
     * has one entry for each type of permission asked by that URL.
     */
    void fillModelByOrigin();

    /**
     * @brief Fills the permissions model depending using the grouping chosen by the user
     *
     * This also resizes the view header according to its contents and expands all items
     */
    void fillModel();

    /**
     * @brief Inserts data in an item representing the state of a permission
     *
     * This sets the text, tooltip, alignment and PermissionStateRole.
     *
     * @param it the item to fill
     * @param state the state to display in the item
     * @param type the permission type the item refers to (required to generate the
     * state name and tooltip).
     *
     * @see permissionStateName()
     * @see permissionStateToolTip()
     */
    void fillPermissionStateItem(QStandardItem *it, QWebEnginePermission::State state, QWebEnginePermission::PermissionType type);

    /**
     * @brief Updates the state of a permission object
     *
     * This is the function which actually sets the state chosen by the user on the permission object, calling its `reset()`,
     * `grant()` or `denied()` methods.
     *
     * @param perm the permission to change
     * @param state the new state of the permission. It should not be `QWebEnginePermission::State::Invalid`
     */
    void updatePermission(QWebEnginePermission &perm, QWebEnginePermission::State state);

private Q_SLOTS:

    /**
     * @brief Sets the state of all permissions to `Ask`
     */
    void resetAllPermissions();

    /**
     * @brief Slot called when one of the grouping radio buttons is toggled
     *
     * It changes the grouping mode and refills the model if the button has been
     * checked and does nothing if it has been unchecked.
     *
     * @param btn the button
     * @param on whether the button has been checked or unchecked
     */
    void groupingButtonToggled(QRadioButton *btn, bool on);

private:

    static constexpr int s_permissionRole = Qt::UserRole; //!< The role used to store the permission associated with an index

    QScopedPointer<Ui::Permissions> m_ui; //!< The UI object

    QStandardItemModel *m_model; //!< The model used to display the list of permissions

    PermissionStateDelegate *m_delegate; //!< The delegate used to edit the permissions state

    /**
     * @brief A hash associating permissions and states
     *
     * When load() is called, it associates each permission with the state provided
     * by QWebEngineProfile. When the user changes the states in the view, the new
     * state is recorded here so that they may be changed when save() is called.
     */
    QHash<QWebEnginePermission, QWebEnginePermission::State> m_states;
};

/**
 * @brief Delegate to change permission states in the Permissions KCM
 *
 * It allows to choose the state of a permission using a combo box.
 */
class PermissionStateDelegate : public QStyledItemDelegate {
public:
    /**
     * @brief Constructor
     *
     * @param parent the parent object
     */
    PermissionStateDelegate(Permissions *parent = nullptr);

    ~PermissionStateDelegate() override; //!< Destructor

    /**
     * @brief Override of `QStyledItemDelegate::createEditor()`
     *
     * It creates a combo box with one entry for each of the three values `Ask`, `Grant` `Denied`
     */
    QWidget * createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const override;

    /**
     * @brief Override of `QStyledItemDelegate::setEditorData()`
     *
     * It reads the state associated with the index from permissions() and
     * sets the current item in the combo box accordingly.
     */
    void setEditorData(QWidget * editor, const QModelIndex & index) const override;

    /**
     * @brief Override of `QStyledItemDelegate::setModelData()`
     *
     * It uses Permissions::setPermissionState() to update the permission state according to the current item in the combo box
     */
    void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const override;

    /**
     * @brief Override of `QStyledItemDelegate::updateEditorGeometry()`
     */
    void updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option, const QModelIndex & index) const override;

private:

    /**
     * @brief The Permission KCM using the delegate
     *
     * @return the parent object cast to a #Permissions
     */
    Permissions* permissions() const;
};

/**
 * @brief `qHash()` implementation for `QWebEnginePermission`
 *
 * @param perm the permission object
 * @param seed the seed
 * @return a hash value determined by the origin and the permission type (but not the permission state)
 */
size_t qHash(const QWebEnginePermission &perm, size_t seed = 0);

#endif // PERMISSIONS_H
