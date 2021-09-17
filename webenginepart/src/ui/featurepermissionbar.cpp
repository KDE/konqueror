/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit @ kde.org>
    SPDX-FileCopyrightText: 2013 Allan Sandfeld Jensen <sandfeld@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "featurepermissionbar.h"


#include <QAction>


FeaturePermissionBar::FeaturePermissionBar(QWidget *parent)
                     :KMessageWidget(parent)
{
    setCloseButtonVisible(false);
    setMessageType(KMessageWidget::Information);

    QAction *action = new QAction(i18nc("@action:deny permission", "&Deny permission"), this);
    connect(action, &QAction::triggered, this, &FeaturePermissionBar::onDeniedButtonClicked);
    addAction(action);

    action = new QAction(i18nc("@action:grant permission", "&Grant permission"), this);
    connect(action, &QAction::triggered, this, &FeaturePermissionBar::onGrantedButtonClicked);
    addAction(action);

    // FIXME: Add option to allow and remember for this site.
}

FeaturePermissionBar::~FeaturePermissionBar()
{
}

QWebEnginePage::Feature FeaturePermissionBar::feature() const
{
    return m_feature;
}

QUrl FeaturePermissionBar::url() const
{
    return m_url;
}

void FeaturePermissionBar::setUrl(const QUrl& url)
{
    m_url = url;
}

QString FeaturePermissionBar::labelText(QWebEnginePage::Feature feature) const
{
    QString origin = m_url.toDisplayString();
    switch (feature) {
        case QWebEnginePage::Notifications:
            return i18n("<html><b>%1</b> would like to send you notifications", origin);
        case QWebEnginePage::Geolocation:
            return i18n("<html><b>%1</b> would like to access information about your current physical location", origin);
        case QWebEnginePage::MediaAudioCapture:
            return i18n("<html><b>%1</b> would like to access your microphone and other audio capture devices", origin);
        case QWebEnginePage::MediaVideoCapture:
            return i18n("<html><b>%1</b> would like to access your camera and other video capture devices", origin);
        case QWebEnginePage::MediaAudioVideoCapture:
            return i18n("<html><b>%1</b> would like to access to your microphone, camera and other audio and video capture devices", origin);
        case QWebEnginePage::MouseLock:
            return i18n("<html><b>%1</b> would like to lock your mouse inside the web page", origin);
        case QWebEnginePage::DesktopVideoCapture:
            return i18n("<html><b>%1</b> would like to record your screen", origin);
        case QWebEnginePage::DesktopAudioVideoCapture:
            return i18n("<html><b>%1</b> would like to record your screen and your audio", origin);
        default:
            return QString();
    }
}

void FeaturePermissionBar::setFeature (QWebEnginePage::Feature feature)
{
    m_feature = feature;
    setText(labelText(feature));
}

void FeaturePermissionBar::onDeniedButtonClicked()
{
    animatedHide();
    emit permissionPolicyChosen(m_feature, QWebEnginePage::PermissionDeniedByUser);
    emit done();
}

void FeaturePermissionBar::onGrantedButtonClicked()
{
    animatedHide();
    emit permissionPolicyChosen(m_feature, QWebEnginePage::PermissionGrantedByUser);
    emit done();
}
