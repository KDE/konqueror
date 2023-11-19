/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit @ kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "passwordbar.h"

#include "settings/webenginesettings.h"

#include <KColorScheme>
#include <KLocalizedString>

#include <QCoreApplication>
#include <QAction>
#include <QPalette>
#include <QToolButton>
#include <QTimer>

#include <algorithm>

PasswordBar::PasswordBar(QWidget *parent) :KMessageWidget(parent), m_detailsVisible(false), m_detailsWidget(new CredentialsDetailsWidget(parent))
{
    setCloseButtonVisible(false);
    setMessageType(KMessageWidget::Information);

    QAction* action = new QAction(i18nc("@action:remember password", "&Remember"), this);
    connect(action, &QAction::triggered, this, &PasswordBar::onRememberButtonClicked);
    addAction(action);

    action = new QAction(i18nc("@action:never for this site", "Ne&ver for this site"), this);
    connect(action, &QAction::triggered, this, &PasswordBar::onNeverButtonClicked);
    addAction(action);

    action = new QAction(i18nc("@action:not now", "N&ot now"), this);
    connect(action, &QAction::triggered, this, &PasswordBar::onNotNowButtonClicked);
    addAction(action);

    m_detailsAction = new QAction(i18nc("@action:display details about credentials to store", "&Show details"), this);
    m_detailsAction->setObjectName("detailsAction");
    connect(m_detailsAction, &QAction::triggered, this, &PasswordBar::onDetailsButtonClicked);
    addAction(m_detailsAction);
}

PasswordBar::~PasswordBar()
{
    if (m_detailsWidget) {
        m_detailsWidget->deleteLater();
    }
}

QUrl PasswordBar::url() const
{
    return m_url;
}

QString PasswordBar::requestKey() const
{
    return m_requestKey;
}

void PasswordBar::setUrl (const QUrl& url)
{
    m_url = url;
}

void PasswordBar::setRequestKey (const QString& key)
{
    m_requestKey = key;
}

void PasswordBar::onNotNowButtonClicked()
{
    animatedHide();
    emit saveFormDataRejected (m_requestKey);
    emit done();
    clear();
}

void PasswordBar::onNeverButtonClicked()
{
    WebEngineSettings::self()->addNonPasswordStorableSite(m_url.host());
    onNotNowButtonClicked();
}

void PasswordBar::onRememberButtonClicked()
{
    animatedHide();
    emit saveFormDataAccepted(m_requestKey);
    emit done();
    clear();
}

void PasswordBar::clear()
{
    m_requestKey.clear();
    m_url.clear();
    if (m_detailsWidget) {
        m_detailsWidget->clear();
        setDetailsWidgetVisibility(false);
    }
}

void PasswordBar::resizeEvent(QResizeEvent* event)
{
    KMessageWidget::resizeEvent(event);
    if (m_detailsVisible && m_detailsWidget) {
        m_detailsWidget->move(computeDetailsWidgetPosition());
    }
}

void PasswordBar::onDetailsButtonClicked()
{
    setDetailsWidgetVisibility(!m_detailsVisible);
}

void PasswordBar::setDetailsWidgetVisibility(bool visible)
{
    m_detailsVisible = visible;
    if (m_detailsVisible) {
        m_detailsAction->setText(i18nc("@action:hide details about credentials to store", "&Hide details"));
    } else {
        m_detailsAction->setText(i18nc("@action:display details about credentials to store", "&Show details"));
    }
    if (m_detailsWidget) {
        m_detailsWidget->setVisible(m_detailsVisible);
        if (m_detailsVisible) {
            m_detailsWidget->resize(m_detailsWidget->sizeHint());
            m_detailsWidget->move(computeDetailsWidgetPosition());
        }
    }
}

void PasswordBar::setForms(const WebEngineWallet::WebFormList& forms)
{
    if (m_detailsWidget) {
        m_detailsWidget->setForms(forms);
    }
}

QPoint PasswordBar::computeDetailsWidgetPosition() const
{
    if (!m_detailsWidget) {
        return QPoint();
    }
    return mapTo(parentWidget(), QPoint{width() - m_detailsWidget->width(), height()});
}
