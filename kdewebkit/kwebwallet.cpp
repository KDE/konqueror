/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2009 Dawit Alemayehu <adawit@kde.org>
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
 *
 */

#include "kwebwallet.h"

#include <kwallet.h>

#include <QtCore/QPointer>
#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebPage>
#include <qwindowdefs.h>

class KWebWallet::KWebWalletPrivate
{  
public:
    KWebWalletPrivate()
      : wid (0)
    {
    }

    QPointer<KWallet::Wallet> wallet;
    WId wid;
};

KWebWallet::KWebWallet(QObject *parent)
           :QObject(parent), d(new KWebWallet::KWebWalletPrivate)
{
    QWebPage *page = qobject_cast<QWebPage*>(parent);
    if (page && page->view())
        d->wid = page->view()->window()->winId();
}

KWebWallet::~KWebWallet()
{
    delete d;
}

bool KWebWallet::saveFormData(QWebFrame *frame, SaveType type)
{
    return false;
}

void KWebWallet::restoreFormData(QWebFrame *frame)
{
}

void KWebWallet::confirmSaveFormDataRequest(const QString &key)
{
}

void KWebWallet::doSaveFormData(QWebFrame *frame)
{
}

void KWebWallet::doRestoreFormData(QWebFrame *frame)
{
}
