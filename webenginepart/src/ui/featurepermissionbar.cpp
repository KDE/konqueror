/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit @ kde.org>
    SPDX-FileCopyrightText: 2013 Allan Sandfeld Jensen <sandfeld@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "featurepermissionbar.h"


#include <QAction>


FeaturePermissionBar::FeaturePermissionBar(QWebEnginePermission permission, QWidget *parent)
                     :KMessageWidget(parent), m_permission(permission)
{
    setCloseButtonVisible(false);
    setMessageType(KMessageWidget::Information);

    QAction *action = new QAction(i18nc("@action:deny permission", "&Deny permission"), this);
    connect(action, &QAction::triggered, this, &FeaturePermissionBar::onDeniedButtonClicked);
    addAction(action);

    action = new QAction(i18nc("@action:grant permission", "&Grant permission"), this);
    connect(action, &QAction::triggered, this, &FeaturePermissionBar::onGrantedButtonClicked);
    addAction(action);
    setText(labelText());
}

FeaturePermissionBar::~FeaturePermissionBar()
{
}

QWebEnginePermission::PermissionType FeaturePermissionBar::permissionType() const
{
    return m_permission.permissionType();
}

QUrl FeaturePermissionBar::origin() const
{
    return m_permission.origin();
}

QString FeaturePermissionBar::labelText() const
{
    QString origin = m_permission.origin().toDisplayString();
    switch (m_permission.permissionType()) {
        case QWebEnginePermission::PermissionType::MediaAudioCapture:
            return i18n("<html><b>%1</b> would like to access your microphone and other audio capture devices", origin);
        case QWebEnginePermission::PermissionType::MediaVideoCapture:
            return i18n("<html><b>%1</b> would like to access your camera and other video capture devices", origin);
        case QWebEnginePermission::PermissionType::MediaAudioVideoCapture:
            return i18n("<html><b>%1</b> would like to access to your microphone, camera and other audio and video capture devices", origin);
        case QWebEnginePermission::PermissionType::DesktopVideoCapture:
            return i18n("<html><b>%1</b> would like to record your screen", origin);
        case QWebEnginePermission::PermissionType::DesktopAudioVideoCapture:
            return i18n("<html><b>%1</b> would like to record your screen and your audio", origin);
        case QWebEnginePermission::PermissionType::MouseLock:
            return i18n("<html><b>%1</b> would like to lock your mouse inside the web page", origin);
        case QWebEnginePermission::PermissionType::Notifications:
            return i18n("<html><b>%1</b> would like to send you notifications", origin);
        case QWebEnginePermission::PermissionType::Geolocation:
            return i18n("<html><b>%1</b> would like to access information about your current physical location", origin);
        case QWebEnginePermission::PermissionType::ClipboardReadWrite:
            return i18n("<html><b>%1</b> would like to access your clipboard", origin);
        case QWebEnginePermission::PermissionType::LocalFontsAccess:
            return i18n("<html><b>%1</b> would like to access the fonts installed on your machine", origin);
        default:
            return i18n("<html><b>%1</b> would like to do something we don't know about", origin);
    }
}

void FeaturePermissionBar::onDeniedButtonClicked()
{
    animatedHide();
    m_permission.deny();
    emit done();
}

void FeaturePermissionBar::onGrantedButtonClicked()
{
    animatedHide();
    m_permission.grant();
    emit done();
}
