/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2013 Allan Sandfeld Jensen <sandfeld @ kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef FEATUREPERMISSIONBAR_H
#define FEATUREPERMISSIONBAR_H

#include <KMessageWidget>
#include <KLocalizedString>

#include <QWebEnginePermission>
#include <QUrl>

class FeaturePermissionBar : public KMessageWidget
{
    Q_OBJECT
public:
    explicit FeaturePermissionBar(QWebEnginePermission permission, QWidget *parent = nullptr);
    ~FeaturePermissionBar() override;

    QWebEnginePermission::PermissionType permissionType() const;
    QUrl origin() const;

Q_SIGNALS:
    void done();

private Q_SLOTS:
    void onDeniedButtonClicked();
    void onGrantedButtonClicked();

private:
    QString labelText() const;

private:
    QWebEnginePermission m_permission;
};

#endif // FEATUREPERMISSIONBAR_H
