/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "permissions.h"
#include "ui_permissions.h"

#include "konqsettings.h"

#include "../src/profile.h" //webenginepart/src/profile

#include <KPluginFactory>

#include <QDBusMessage>
#include <QDBusConnection>

#include <QStandardItemModel>
#include <QStandardItem>
#include <QInputDialog>
#include <QWebEnginePermission>
#include <QComboBox>

K_PLUGIN_CLASS_WITH_JSON(Permissions, "kcm_permissions.json")

PermissionStateDelegate::PermissionStateDelegate(Permissions* parent) : QStyledItemDelegate(parent)
{
}

PermissionStateDelegate::~PermissionStateDelegate() noexcept
{
}

Permissions* PermissionStateDelegate::permissions() const
{
    return qobject_cast<Permissions*>(parent());
}

QWidget* PermissionStateDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& idx) const
{
    QComboBox *combo = new QComboBox(parent);
    Permissions::MaybePermission mbPerm = permissions()->permissionForRow(idx);

    if (!mbPerm) {
        return QStyledItemDelegate::createEditor(parent, option, idx);
    }

    combo->addItems({
        Permissions::permissionStateName(QWebEnginePermission::State::Ask, mbPerm->permissionType()),
        Permissions::permissionStateName(QWebEnginePermission::State::Granted, mbPerm->permissionType()),
        Permissions::permissionStateName(QWebEnginePermission::State::Denied, mbPerm->permissionType())
    });
    return combo;
}

void PermissionStateDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    QComboBox* combo = qobject_cast<QComboBox*>(editor);
    if (!combo) {
        return;
    }

    Permissions::MaybeState mbState = permissions()->stateForRow(index);
    if (!mbState) {
        return;
    }

    //State 0 is invalid, while we want index 0 for Ask
    combo->setCurrentIndex(static_cast<int>(mbState.value()) - 1);
}

void PermissionStateDelegate::setModelData(QWidget* editor, QAbstractItemModel*, const QModelIndex& index) const
{
    QComboBox* combo = qobject_cast<QComboBox*>(editor);
    if (!combo) {
        return;
    }
    //Index 0 means Ask, while Ask is State 1
    int newState = combo->currentIndex() + 1;
    permissions()->setPermissionState(index, static_cast<QWebEnginePermission::State>(newState));
}

void PermissionStateDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const
{
    editor->setGeometry(option.rect);
}

Permissions::Permissions(QObject *parent, const KPluginMetaData &md): KCModule(parent, md),
    m_ui(new Ui::Permissions),
    m_model(new QStandardItemModel(this)),
    m_delegate(new PermissionStateDelegate(this))
{
    m_ui->setupUi(widget());
    m_ui->permissions->setModel(m_model);
    m_ui->permissions->setItemDelegateForColumn(1, m_delegate);
    connect(m_ui->resetAllPermissionsBtn, &QPushButton::clicked, this, &Permissions::resetAllPermissions);
    connect(m_ui->groupByOrigin, &QPushButton::toggled, this, [this](bool on) {groupingButtonToggled(m_ui->groupByOrigin, on);});
    connect(m_ui->groupByType, &QPushButton::toggled, this, [this](bool on) {groupingButtonToggled(m_ui->groupByType, on);});

    connect(m_ui->defaultPermissionPolicy, &QComboBox::currentIndexChanged, this, [this]{setNeedsSave(true);});

    setSupportsInstantApply(false);
}

Permissions::~Permissions()
{
}

QString Permissions::permissionTypeName(QWebEnginePermission::PermissionType type)
{
    switch (type) {
        case QWebEnginePermission::PermissionType::MediaAudioCapture:
            return i18nc("Permission to access the microphone", "Microphone access");
        case QWebEnginePermission::PermissionType::MediaVideoCapture:
            return i18nc("Permission to access the webcam", "Webcam access");
        case QWebEnginePermission::PermissionType::MediaAudioVideoCapture:
            return i18nc("Permission to access the microphone and the webcam", "Microphone and webcam access");
        case QWebEnginePermission::PermissionType::DesktopVideoCapture:
            return i18nc("Permission to access the user's screen", "Screen access");
        case QWebEnginePermission::PermissionType::DesktopAudioVideoCapture:
            return i18nc("Permission to access the user's screen and application audio", "Screen and application audio access");
        case QWebEnginePermission::PermissionType::MouseLock:
            return i18nc("Permission to lock the mouse pointer inside the page", "Mouse pointer locking");
        case QWebEnginePermission::PermissionType::Notifications:
            return i18nc("Permission to send notifications", "Notifications");
        case QWebEnginePermission::PermissionType::Geolocation:
            return i18nc("Permission to access the user's physical location", "Physical location access");
        case QWebEnginePermission::PermissionType::ClipboardReadWrite:
            return i18nc("Permission to read and write the user's clipboard", "Clipboard access");
        case QWebEnginePermission::PermissionType::LocalFontsAccess:
            return i18nc("Permission to access the user's local fonts", "Local fonts access");
        case QWebEnginePermission::PermissionType::Unsupported:
            return {};
    }
    Q_UNREACHABLE_RETURN({}); //This is only to make the compiler happy, since all possible values are handled in the switch
}

QString Permissions::permissionTypeToolTip(QWebEnginePermission::PermissionType type)
{
    switch (type) {
        case QWebEnginePermission::PermissionType::MediaAudioCapture:
            return i18nc("@info:tooltip", "Permission to access audio sources such as a microphone");
        case QWebEnginePermission::PermissionType::MediaVideoCapture:
            return i18nc("@info:tooltip", "Permission to access video sources such as a webcam");
        case QWebEnginePermission::PermissionType::MediaAudioVideoCapture:
            return i18nc("@info:tooltip", "Permission to access audio sources such as a microphone and video sources such a webcam");
        case QWebEnginePermission::PermissionType::DesktopVideoCapture:
            return i18nc("@info:tooltip", "Permission to access screen contents");
        case QWebEnginePermission::PermissionType::DesktopAudioVideoCapture:
            return i18nc("@info:tooltip", "Permission to access screen contents and application audio");
        case QWebEnginePermission::PermissionType::MouseLock:
            return i18nc("@info:tooltip", "Permission to lock the mouse pointer inside a specific part of the page");
        case QWebEnginePermission::PermissionType::Notifications:
            return i18nc("@info:tooltip", "Permission to send notifications");
        case QWebEnginePermission::PermissionType::Geolocation:
            return i18nc("@info:tooltip", "Permission to access your physical location");
        case QWebEnginePermission::PermissionType::ClipboardReadWrite:
            return i18nc("@info:tooltip", "Permission to read and modify the contents of the clipboard");
        case QWebEnginePermission::PermissionType::LocalFontsAccess:
            return i18nc("@info:tooltip", "Permission to access the fonts installed on your machine");
        case QWebEnginePermission::PermissionType::Unsupported:
            return {};
    }
    Q_UNREACHABLE_RETURN({}); //This is only to make the compiler happy, since all possible values are handled in the switch
}

QString Permissions::permissionStateName(QWebEnginePermission::State state, QWebEnginePermission::PermissionType type)
{
    switch (state) {
        case QWebEnginePermission::State::Ask:
            if (QWebEnginePermission::isPersistent(type)) {
                return i18nc("@item:inlistbox Ask for permission next time", "Ask next time");
            } else {
                return i18nc("@item:inlistbox Ask for permission every time", "Ask every time");
            }
        case QWebEnginePermission::State::Granted:
            return i18nc("@item:inlistbox The permission has been granted", "Granted");
        case QWebEnginePermission::State::Denied:
            return i18nc("@item:inlistbox The permission has been denied", "Denied");
        case QWebEnginePermission::State::Invalid:
        default:
            return {};
    }
}

QString Permissions::permissionStateToolTip(QWebEnginePermission::State state, QWebEnginePermission::PermissionType type)
{
    switch (state) {
        case QWebEnginePermission::State::Ask:
            if (QWebEnginePermission::isPersistent(type)) {
                return i18nc("@info:tooltip", "The permission has never been asked before. You'll be asked the next time the web page requires it");
            } else {
                return i18nc("@info:tooltip", "You'll be asked for the permission every time a page requires it");
            }
        case QWebEnginePermission::State::Granted:
            return i18nc("@info:tooltip", "You have already granted this permission");
        case QWebEnginePermission::State::Denied:
            return i18nc("@info:tooltip", "You have already denied this permission");
        case QWebEnginePermission::State::Invalid:
        default:
            return {};
    }
}

void Permissions::groupingButtonToggled(QRadioButton* btn, bool on)
{
    Q_UNUSED(btn);
    if (on) {
        fillModel();
    }
}

void Permissions::readPermissions()
{
    QList<QWebEnginePermission> permissions = KonqWebEnginePart::Profile::defaultProfile()->listAllPermissions();
    for (const QWebEnginePermission &perm : permissions) {
        m_states.insert(perm, perm.state());
    }
}

void Permissions::resetAllPermissions()
{
    for (int i = 0; i < m_model->rowCount(); ++i) {
        QStandardItem *p = m_model->item(i, 0);
        for (int j = 0; j < p->rowCount(); ++j) {
            setPermissionState(m_model->index(j, 1, p->index()), QWebEnginePermission::State::Ask);
        }
    }
}

void Permissions::setPermissionState(const QModelIndex &idx, QWebEnginePermission::State newState)
{
    QStandardItem *it = m_model->itemFromIndex(idx);
    MaybePermission mbPerm = permissionForRow(idx);
    if (!mbPerm) {
        return;
    }

    QWebEnginePermission::State oldState = m_states.value(*mbPerm, QWebEnginePermission::State::Invalid);

    m_states[*mbPerm] = newState;

    it = it->parent()->child(it->row(), 1);
    it->setText(permissionStateName(newState, mbPerm->permissionType()));
    it->setToolTip(permissionStateToolTip(newState, mbPerm->permissionType()));

    if (newState != oldState) {
        setNeedsSave(true);
    }
}

void Permissions::fillModel()
{
    if (m_ui->groupByOrigin->isChecked()) {
        fillModelByOrigin();
    } else {
        fillModelByType();
    }
    m_ui->permissions->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_ui->permissions->expandAll();
}

void Permissions::fillPermissionStateItem(QStandardItem *it, QWebEnginePermission::State state, QWebEnginePermission::PermissionType type)
{
    it->setText(permissionStateName(state, type));
    it->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
    it->setToolTip(permissionStateToolTip(state, type));
}

void Permissions::fillModelByOrigin()
{
    m_model->clear();

    m_model->setHorizontalHeaderLabels({
        i18nc("@title:column The type of permission", "Permission type"),
        i18nc("@title:column The status of a permission permission", "Status")
    });

    QHash<QUrl, QStandardItem*> items;
    for (auto [perm, state] : m_states.asKeyValueRange()) {
        QUrl url = perm.origin();
        QStandardItem *originIt = items.value(url);
        if (!originIt) {
            originIt = new QStandardItem(url.toDisplayString());
            m_model->appendRow(originIt);
        }
        QStandardItem *typeIt = new QStandardItem(permissionTypeName(perm.permissionType()));
        typeIt->setToolTip(permissionTypeToolTip(perm.permissionType()));
        typeIt->setData(QVariant::fromValue(perm), s_permissionRole);
        typeIt->setEditable(false);
        QStandardItem *stateIt = new QStandardItem();
        fillPermissionStateItem(stateIt, state, perm.permissionType());
        originIt->appendRow({typeIt, stateIt});
    }
}

void Permissions::fillModelByType()
{
    m_model->clear();

    m_model->setHorizontalHeaderLabels({
        i18nc("@title:column the origin the permission was granted to", "Origin"),
        i18nc("@title:column The status of a permission permission", "Status")
    });

    QHash<QWebEnginePermission::PermissionType, QStandardItem*> items;

    for (auto [perm, state] : m_states.asKeyValueRange()) {
        QWebEnginePermission::PermissionType type = perm.permissionType();
        QStandardItem *typeIt = items.value(type);
        if (!typeIt) {
            typeIt = new QStandardItem(permissionTypeName(type));
            typeIt->setToolTip(permissionTypeToolTip(type));
            typeIt->setEditable(false);
            m_model->appendRow(typeIt);
        }
        QString urlString = perm.origin().toDisplayString();
        QStandardItem *urlIt = new QStandardItem(urlString);
        //The tooltip can be useful in case the item text is truncated
        urlIt->setToolTip(urlString);
        urlIt->setData(QVariant::fromValue(perm), s_permissionRole);
        urlIt->setEditable(false);
        QStandardItem *stateIt = new QStandardItem();
        fillPermissionStateItem(stateIt, state, perm.permissionType());
        typeIt->appendRow({urlIt, stateIt});
    }
}

Permissions::MaybePermission Permissions::permissionForRow(const QModelIndex idx) const
{
    QVariant data = m_model->index(idx.row(), 0, idx.parent()).data(s_permissionRole);
    return data.isValid() ? std::make_optional(data.value<QWebEnginePermission>()) : std::nullopt;
}

Permissions::MaybeState Permissions::stateForRow(const QModelIndex& idx) const
{
    MaybePermission mbPerm = permissionForRow(idx);
    if (!mbPerm) {
        return std::nullopt;
    }

    QWebEnginePermission::State state = m_states.value(mbPerm.value(), QWebEnginePermission::State::Invalid);
    return state != QWebEnginePermission::State::Invalid ? std::make_optional(state) : std::nullopt;
}

void Permissions::defaults()
{
    Konq::Settings::self()->withDefaults([this]{load();});
    resetAllPermissions();
    setNeedsSave(true);
    setRepresentsDefaults(true);
    KCModule::defaults();
}

void Permissions::load()
{
    readPermissions();
    fillModel();
    m_ui->defaultPermissionPolicy->setCurrentIndex(Konq::Settings::persistentPermissionPolicy());
    KCModule::load();
}

void Permissions::updatePermission(QWebEnginePermission& perm, QWebEnginePermission::State state)
{
    switch (state) {
        case QWebEnginePermission::State::Ask:
            perm.reset();
            break;
        case QWebEnginePermission::State::Granted:
            perm.grant();
            break;
        case QWebEnginePermission::State::Denied:
            perm.deny();
            break;
        default:
            break;
    }
}

void Permissions::save()
{
    for (QWebEnginePermission& perm : m_states.keys()) {
        updatePermission(perm, m_states.value(perm));
    }

    Konq::Settings::setPersistentPermissionPolicy(m_ui->defaultPermissionPolicy->currentIndex());
    Konq::Settings::self()->save();

    QDBusMessage message =
        QDBusMessage::createSignal(QStringLiteral("/KonqMain"), QStringLiteral("org.kde.Konqueror.Main"), QStringLiteral("reparseConfiguration"));
    QDBusConnection::sessionBus().send(message);
    KCModule::save();
}

size_t qHash(const QWebEnginePermission& perm, size_t seed)
{
    return qHashMulti(seed, perm.origin(), perm.permissionType());
}

#include "permissions.moc"
