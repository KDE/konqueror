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

#include <KLocalizedString>

#include <QAction>


FeaturePermissionBar::FeaturePermissionBar(QWidget *parent)
                     :KMessageWidget(parent)
{
    setCloseButtonVisible(false);
    setMessageType(KMessageWidget::Information);

    QAction* action = new QAction(i18nc("@action:deny access", "&Deny access"), this);
    connect(action, SIGNAL(triggered()), this, SLOT(onDeniedButtonClicked()));
    addAction(action);

    action = new QAction(i18nc("@action:grant access", "&Grant access"), this);
    connect(action, SIGNAL(triggered()), this, SLOT(onGrantedButtonClicked()));
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

void FeaturePermissionBar::setFeature (QWebEnginePage::Feature feature)
{
    m_feature = feature;
}

void FeaturePermissionBar::onDeniedButtonClicked()
{
    animatedHide();
    emit permissionDenied(m_feature);
    emit done();
}

void FeaturePermissionBar::onGrantedButtonClicked()
{
    animatedHide();
    emit permissionGranted(m_feature);
    emit done();
}
