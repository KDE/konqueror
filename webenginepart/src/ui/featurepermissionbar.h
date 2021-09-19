/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2013 Allan Sandfeld Jensen <sandfeld @ kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef FEATUREPERMISSIONBAR_H
#define FEATUREPERMISSIONBAR_H

#include <KMessageWidget>
#include <KLocalizedString>

#include <QWebEnginePage>
#include <QUrl>

class FeaturePermissionBar : public KMessageWidget
{
    Q_OBJECT
public:
    explicit FeaturePermissionBar(QWidget *parent = nullptr);
    ~FeaturePermissionBar() override;

    QWebEnginePage::Feature feature() const;
    QUrl url() const;

    void setFeature(QWebEnginePage::Feature);
    void setUrl(const QUrl &url);

Q_SIGNALS:
    void permissionPolicyChosen(QWebEnginePage::Feature feature, QWebEnginePage::PermissionPolicy policy);
    void done();

private Q_SLOTS:
    void onDeniedButtonClicked();
    void onGrantedButtonClicked();

private:
    QString labelText(QWebEnginePage::Feature feature) const;

private:
    QWebEnginePage::Feature m_feature;
    QUrl m_url;
};

#endif // FEATUREPERMISSIONBAR_H
