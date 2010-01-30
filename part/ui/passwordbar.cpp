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
#include "ui_passwordbar.h"

#include "settings/webkitsettings.h"

#include <KDE/KIcon>
#include <KDE/KDebug>
#include <KDE/KColorScheme>
#include <KDE/KLocalizedString>

#include <QtCore/QUrl>
#include <QtCore/QCoreApplication>
#include <QtGui/QPalette>

namespace KDEPrivate {

class PasswordBar::PasswordBarPrivate
{
public:
    PasswordBarPrivate() {}

    void init (PasswordBar* passwordBar)
    {
        ui.setupUi(passwordBar);
        ui.closeButton->setIcon(KIcon("dialog-close"));

        QPalette pal = passwordBar->palette();
        KColorScheme::adjustBackground(pal, KColorScheme::ActiveBackground);
        passwordBar->setPalette(pal);
        passwordBar->setBackgroundRole(QPalette::Base);
        passwordBar->setAutoFillBackground(true);

        connect(ui.notNowButton, SIGNAL(clicked()),
                passwordBar, SLOT(onNotNowButtonClicked()));
        connect(ui.closeButton, SIGNAL (clicked()),
                passwordBar, SLOT(onNotNowButtonClicked()));
        connect(ui.neverButton, SIGNAL(clicked()),
                passwordBar, SLOT(onNeverButtonClicked()));
        connect(ui.rememberButton, SIGNAL(clicked()),
                passwordBar, SLOT(onRememberButtonClicked()));
    }

    Ui::PasswordBar ui;
    QString requestKey;
    QUrl url;
};

PasswordBar::PasswordBar(QWidget *parent)
            :QWidget(parent), d(new PasswordBarPrivate)
{
    d->init(this);

    // Hide the widget by default
    setVisible(false);
}

PasswordBar::~PasswordBar()
{
    delete d;
}

void PasswordBar::onSaveFormData(const QString &key, const QUrl &url)
{
    d->url = url;
    d->requestKey = key;
    d->ui.infoLabel->setText(i18n("<html>Do you want %1 to remember the login "
                                  "information for <b>%2</b>?</html>",
                                  QCoreApplication::applicationName(),
                                  url.host()));

    if (WebKitSettings::self()->isNonPasswordStorableSite(url.host()))
      onNotNowButtonClicked();
    else
       show();
}

void PasswordBar::onNotNowButtonClicked()
{
    hide();
    emit saveFormDataRejected (d->requestKey);

}

void PasswordBar::onNeverButtonClicked()
{
    WebKitSettings::self()->addNonPasswordStorableSite(d->url.host());
    onNotNowButtonClicked();
}

void PasswordBar::onRememberButtonClicked()
{
    hide();
    emit saveFormDataAccepted(d->requestKey);
}

}

#include "passwordbar.moc"
