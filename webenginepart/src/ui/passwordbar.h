/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit @ kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PASSWORDBAR_H
#define PASSWORDBAR_H

#include <KMessageWidget>

#include <QUrl>
#include <QPointer>

#include "wallet/webenginewallet.h"
#include "credentialsdetailswidget.h"

class PasswordBar : public KMessageWidget
{
    Q_OBJECT
public:
    explicit PasswordBar(QWidget *parent = nullptr);
    ~PasswordBar() override;

    QUrl url() const;
    QString requestKey() const;

    void setUrl(const QUrl&);
    void setRequestKey(const QString&);

    void setForms(const WebEngineWallet::WebFormList &forms);

Q_SIGNALS:
    void saveFormDataRejected(const QString &key);
    void saveFormDataAccepted(const QString &key);
    void done();
    void toggleDetailsRequested(const QUrl &url, bool visible);
    void moved();

private Q_SLOTS:
    void onNotNowButtonClicked();
    void onNeverButtonClicked();
    void onRememberButtonClicked();
    void onDetailsButtonClicked();

protected:
    void resizeEvent(QResizeEvent * event) override;

private:
    void clear();
    void setDetailsWidgetVisibility(bool visible);
    QPoint computeDetailsWidgetPosition() const;

    QUrl m_url;
    QString m_requestKey;
    bool m_detailsVisible;
    QAction *m_detailsAction;
    QPointer<CredentialsDetailsWidget> m_detailsWidget;
};

#endif // PASSWORDBAR_H
