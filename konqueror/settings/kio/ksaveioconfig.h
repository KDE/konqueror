/*
   Copyright (C) 2001 Dawit Alemayehu <adawit@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KSAVEIO_CONFIG_H_
#define KSAVEIO_CONFIG_H_

#include <kprotocolmanager.h>

class QWidget;

namespace KSaveIOConfig
{
int proxyDisplayUrlFlags();
void setProxyDisplayUrlFlags (int);

/* Reload config file (kioslaverc) */
void reparseConfiguration();

/** Timeout Settings */
void setReadTimeout (int);

void setConnectTimeout (int);

void setProxyConnectTimeout (int);

void setResponseTimeout (int);


/** Cache Settings */
void setMaxCacheAge (int);

void setUseCache (bool);

void setMaxCacheSize (int);

void setCacheControl (KIO::CacheControl);


/** Proxy Settings */
void setUseReverseProxy (bool);

void setProxyType (KProtocolManager::ProxyType);

void setProxyConfigScript (const QString&);

void setProxyFor (const QString&, const QString&);

QString noProxyFor();
void setNoProxyFor (const QString&);


/** Miscellaneous Settings */
void setMarkPartial (bool);

void setMinimumKeepSize (int);

void setAutoResume (bool);

/** Update all running io-slaves */
void updateRunningIOSlaves (QWidget* parent = 0L);

/** Update proxy scout */
void updateProxyScout (QWidget* parent = 0L);
}

#endif
