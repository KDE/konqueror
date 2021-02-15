/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2009 Dawit Alemayehu <adawit @ kde.org>
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
    Q_EMIT saveFormDataRejected (m_requestKey);
    Q_EMIT done();
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
    Q_EMIT saveFormDataAccepted(m_requestKey);
    Q_EMIT done();
    clear();
}

void PasswordBar::clear()
{
    m_requestKey.clear();
    m_url.clear();
    if (m_detailsWidget) {
        m_detailsWidget->clear();
        m_detailsWidget->hide();
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
    m_detailsVisible = !m_detailsVisible;
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
    return mapTo(parentWidget(), {width() - m_detailsWidget->width(), height()});
}
