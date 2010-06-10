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

#include <QtCore/QVariant>


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

WebSslInfo::WebSslInfo(const WebSslInfo& other)
           :d(new WebSslInfo::WebSslInfoPrivate)
{
  *this = other;
}

WebSslInfo::~WebSslInfo()
{
  delete d;
  d = 0;
}

bool WebSslInfo::isValid() const
{
  return !d->peerAddress.isNull();
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

WebSslInfo& WebSslInfo::operator=(const WebSslInfo& other)
{
  d->ciphers = other.d->ciphers;
  d->protocol = other.d->protocol;
  d->certErrors = other.d->certErrors;
  d->peerAddress = other.d->peerAddress;
  d->parentAddress = other.d->parentAddress;
  d->certificateChain = other.d->certificateChain;

  d->usedCipherBits = other.d->usedCipherBits;
  d->supportedCipherBits = other.d->supportedCipherBits;
  d->url = other.d->url;

  return *this;
}

QVariant WebSslInfo::toMetaData() const
{
  if (isValid()) {
    QMap<QString, QVariant> data;
    data.insert("ssl_in_use", true);
    data.insert("ssl_peer_ip", d->peerAddress.toString());
    data.insert("ssl_parent_ip", d->parentAddress.toString());
    data.insert("ssl_protocol_version", d->protocol);
    data.insert("ssl_cipher", d->ciphers);
    data.insert("ssl_cert_errors", d->certErrors);
    data.insert("ssl_cipher_used_bits", d->usedCipherBits);
    data.insert("ssl_cipher_bits", d->supportedCipherBits);
    QByteArray certChain;
    Q_FOREACH(const QSslCertificate& cert, d->certificateChain)
        certChain += cert.toPem();
    data.insert("ssl_peer_chain", certChain);
    return data;
  }

  return QVariant();
}

void WebSslInfo::fromMetaData(const QVariant& value, const QUrl& url)
{
  if (value.isValid() && value.type() == QVariant::Map) {
    QMap<QString,QVariant> metaData = value.toMap();
    if (metaData.value("ssl_in_use", false).toBool()) {
        setCertificateChain(metaData.value("ssl_peer_chain").toByteArray());
        setPeerAddress(metaData.value("ssl_peer_ip").toString());
        setParentAddress(metaData.value("ssl_parent_ip").toString());
        setProtocol(metaData.value("ssl_protocol_version").toString());
        setCiphers(metaData.value("ssl_cipher").toString());
        setCertificateErrors(metaData.value("ssl_cert_errors").toString());
        setUsedCipherBits(metaData.value("ssl_cipher_used_bits").toString());
        setSupportedCipherBits(metaData.value("ssl_cipher_bits").toString());
        setUrl(url);
    }
  }
}

void WebSslInfo::setUrl (const QUrl &url)
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
