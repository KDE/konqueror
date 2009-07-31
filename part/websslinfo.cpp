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

#include "websslinfo.h"

class WebSslInfo::WebSslInfoPrivate
{
public:
  WebSslInfoPrivate()
      : usedCipherBits(0), supportedCipherBits(0) {}

  QUrl url;
  QString ciphers;
  QString protocol;
  QString certErrors;
  QHostAddress peerAddress;
  QHostAddress parentAddress;
  QList<QSslCertificate> certificateChain;

  int usedCipherBits;
  int supportedCipherBits;
};

WebSslInfo::WebSslInfo()
                :d(new WebSslInfo::WebSslInfoPrivate)
{
}

WebSslInfo::~WebSslInfo()
{
  delete d;
  d = 0;
}

QUrl WebSslInfo::url() const
{
  return d->url;
}

QHostAddress WebSslInfo::parentAddress() const
{
  return d->parentAddress;
}

QHostAddress WebSslInfo::peerAddress() const
{
  return d->peerAddress;
}

QString WebSslInfo::protocol() const
{
  return d->protocol;
}

QString WebSslInfo::ciphers() const
{
  return d->ciphers;
}

QString WebSslInfo::certificateErrors() const
{
  return d->certErrors;
}

int WebSslInfo::supportedChiperBits () const
{
  return d->supportedCipherBits;
}

int WebSslInfo::usedChiperBits () const
{
  return d->usedCipherBits;
}

QList<QSslCertificate> WebSslInfo::certificateChain() const
{
  return d->certificateChain;
}

bool WebSslInfo::isValid() const
{
  return d->url.isValid();
}

void WebSslInfo::reset()
{
  d->url = QUrl();
  d->ciphers.clear();
  d->protocol.clear();
  d->certErrors.clear();
  d->peerAddress.clear();
  d->parentAddress.clear();

  d->usedCipherBits = 0;
  d->supportedCipherBits = 0;
}

void WebSslInfo::setUrl (const QString& url)
{
  d->url = url;
}

void WebSslInfo::setUrl (const QUrl& url)
{
  d->url = url;
}

void WebSslInfo::setPeerAddress(const QString& address)
{
  d->peerAddress = address;
}

void WebSslInfo::setParentAddress(const QString& address)
{
  d->parentAddress = address;
}

void WebSslInfo::setProtocol(const QString& protocol)
{
  d->protocol = protocol;
}

void WebSslInfo::setCertificateChain(const QByteArray& chain)
{
  d->certificateChain = QSslCertificate::fromData(chain);
}

void WebSslInfo::setCiphers(const QString& ciphers)
{
  d->ciphers = ciphers;
}

void WebSslInfo::setUsedCipherBits(const QString& bits)
{
  d->usedCipherBits = bits.toInt();
}

void WebSslInfo::setSupportedCipherBits(const QString& bits)
{
  d->supportedCipherBits = bits.toInt();
}

void WebSslInfo::setCertificateErrors(const QString& certErrors)
{
  d->certErrors = certErrors;
}
