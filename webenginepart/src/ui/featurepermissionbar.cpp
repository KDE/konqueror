/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2009 Dawit Alemayehu <adawit @ kde.org>
 * Copyright (C) 2013 Allan Sandfeld Jensen <sandfeld@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "featurepermissionbar.h"


#include <QAction>


FeaturePermissionBar::FeaturePermissionBar(QWidget *parent)
                     :KMessageWidget(parent)
{
    setCloseButtonVisible(false);
    setMessageType(KMessageWidget::Information);

    QAction* action = new QAction(i18nc("@action:deny access", "&Deny access"), this);
    connect(action, &QAction::triggered, this, &FeaturePermissionBar::onDeniedButtonClicked);
    addAction(action);

    action = new QAction(i18nc("@action:grant access", "&Grant access"), this);
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

QString FeaturePermissionBar::labelForFeature(QWebEnginePage::Feature feature)
{
    switch (feature) {
        case QWebEnginePage::Notifications:
            return i18n("<html>Do you want to allow the site to send you notifications?");
        case QWebEnginePage::Geolocation:
            return i18n("<html>Do you want to grant the site access to information about your current physical location?");
        case QWebEnginePage::MediaAudioCapture:
            return i18n("<html>Do you want to allow the site to access your microphone and other audio capture devices?");
        case QWebEnginePage::MediaVideoCapture:
            return i18n("<html>Do you want to allow the site to access your camera and other video capture devices?");
        case QWebEnginePage::MediaAudioVideoCapture:
            return i18n("<html>Do you want to allow the site to access to your microphone, camera and other audio and video capture devices?");
        case QWebEnginePage::MouseLock:
            return i18n("<html>Do you want to allow the site to lock your mouse inside the web page?");
        case QWebEnginePage::DesktopVideoCapture:
            return i18n("<html>Do you want to allow the site to record your screen?");
        case QWebEnginePage::DesktopAudioVideoCapture:
            return i18n("<html>Do you want to allow the site to record your screen and your audio?");
        default:
            return QString();
    }
}

void FeaturePermissionBar::setFeature (QWebEnginePage::Feature feature)
{
    m_feature = feature;
    setText(labelForFeature(feature));
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
