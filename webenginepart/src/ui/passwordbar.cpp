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


PasswordBar::PasswordBar(QWidget *parent)
            :KMessageWidget(parent)
{
    setCloseButtonVisible(false);
    setMessageType(KMessageWidget::Information);

    QAction* action = new QAction(i18nc("@action:remember password", "&Remember"), this);
    connect(action, SIGNAL(triggered()), this, SLOT(onRememberButtonClicked()));
    addAction(action);

    action = new QAction(i18nc("@action:never for this site", "Ne&ver for this site"), this);
    connect(action, SIGNAL(triggered()), this, SLOT(onNeverButtonClicked()));
    addAction(action);

    action = new QAction(i18nc("@action:not now", "N&ot now"), this);
    connect(action, SIGNAL(triggered()), this, SLOT(onNotNowButtonClicked()));
    addAction(action);
}

PasswordBar::~PasswordBar()
{
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
}
