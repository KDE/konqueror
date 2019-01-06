/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2013 Allan Sandfeld Jensen <sandfeld @ kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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

#ifndef FEATUREPERMISSIONBAR_H
#define FEATUREPERMISSIONBAR_H

#include <KMessageWidget>

#include <QWebEnginePage>


class FeaturePermissionBar : public KMessageWidget
{
    Q_OBJECT
public:
    explicit FeaturePermissionBar(QWidget *parent = nullptr);
    ~FeaturePermissionBar() override;

    QWebEnginePage::Feature feature() const;

    void setFeature(QWebEnginePage::Feature);

Q_SIGNALS:
    void permissionGranted(QWebEnginePage::Feature);
    void permissionDenied(QWebEnginePage::Feature);
    void done();

private Q_SLOTS:
    void onDeniedButtonClicked();
    void onGrantedButtonClicked();

private:
    QWebEnginePage::Feature m_feature;
};

#endif // FEATUREPERMISSIONBAR_H
