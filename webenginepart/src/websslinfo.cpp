/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "websslinfo.h"

#include <QVariant>


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
  d = nullptr;
}

bool WebSslInfo::isValid() const
{
  return (d ? !d->peerAddress.isNull() : false);
}

QUrl WebSslInfo::url() const
{
  return (d ? d->url : QUrl());
}

QHostAddress WebSslInfo::parentAddress() const
{
  return (d ? d->parentAddress : QHostAddress());
}

QHostAddress WebSslInfo::peerAddress() const
{
  return (d ? d->peerAddress : QHostAddress());
}

QString WebSslInfo::protocol() const
{
  return (d ? d->protocol : QString());
}

QString WebSslInfo::ciphers() const
{
  return (d ?  d->ciphers : QString());
}

QString WebSslInfo::certificateErrors() const
{
  return (d ?  d->certErrors : QString());
}

int WebSslInfo::supportedChiperBits () const
{
  return (d ? d->supportedCipherBits : 0);
}

int WebSslInfo::usedChiperBits () const
{
  return (d ?  d->usedCipherBits : 0);
}

QList<QSslCertificate> WebSslInfo::certificateChain() const
{
  return (d ? d->certificateChain : QList<QSslCertificate>());
}

WebSslInfo& WebSslInfo::operator=(const WebSslInfo& other)
{
  if (d) {
    d->ciphers = other.d->ciphers;
    d->protocol = other.d->protocol;
    d->certErrors = other.d->certErrors;
    d->peerAddress = other.d->peerAddress;
    d->parentAddress = other.d->parentAddress;
    d->certificateChain = other.d->certificateChain;

    d->usedCipherBits = other.d->usedCipherBits;
    d->supportedCipherBits = other.d->supportedCipherBits;
    d->url = other.d->url;
  }

  return *this;
}

bool WebSslInfo::saveTo(QMap<QString, QVariant>& data) const
{
  const bool ok = isValid();
  if (ok) {
    data.insert(QStringLiteral("ssl_in_use"), true);
    data.insert(QStringLiteral("ssl_peer_ip"), d->peerAddress.toString());
    data.insert(QStringLiteral("ssl_parent_ip"), d->parentAddress.toString());
    data.insert(QStringLiteral("ssl_protocol_version"), d->protocol);
    data.insert(QStringLiteral("ssl_cipher"), d->ciphers);
    data.insert(QStringLiteral("ssl_cert_errors"), d->certErrors);
    data.insert(QStringLiteral("ssl_cipher_used_bits"), d->usedCipherBits);
    data.insert(QStringLiteral("ssl_cipher_bits"), d->supportedCipherBits);
    QByteArray certChain;
    for (const QSslCertificate& cert: d->certificateChain)
        certChain += cert.toPem();
    data.insert(QStringLiteral("ssl_peer_chain"), certChain);
  }

  return ok;
}

void WebSslInfo::restoreFrom(const QVariant& value, const QUrl& url, bool reset)
{
  if (reset) {
      *this = WebSslInfo();
  }

  if (value.isValid() && value.metaType().id() == QMetaType::QVariantMap) {
    QMap<QString,QVariant> metaData = value.toMap();
    if (metaData.value(QStringLiteral("ssl_in_use"), false).toBool()) {
        setCertificateChain(metaData.value(QStringLiteral("ssl_peer_chain")).toByteArray());
        setPeerAddress(metaData.value(QStringLiteral("ssl_peer_ip")).toString());
        setParentAddress(metaData.value(QStringLiteral("ssl_parent_ip")).toString());
        setProtocol(metaData.value(QStringLiteral("ssl_protocol_version")).toString());
        setCiphers(metaData.value(QStringLiteral("ssl_cipher")).toString());
        setCertificateErrors(metaData.value(QStringLiteral("ssl_cert_errors")).toString());
        setUsedCipherBits(metaData.value(QStringLiteral("ssl_cipher_used_bits")).toString());
        setSupportedCipherBits(metaData.value(QStringLiteral("ssl_cipher_bits")).toString());
        setUrl(url);
    }
  }
}

void WebSslInfo::setUrl (const QUrl &url)
{
  if (d)  
    d->url = url;
}

void WebSslInfo::setPeerAddress(const QString& address)
{
  if (d)
    d->peerAddress = QHostAddress(address);
}

void WebSslInfo::setParentAddress(const QString& address)
{
  if (d)  
    d->parentAddress = QHostAddress(address);
}

void WebSslInfo::setProtocol(const QString& protocol)
{
  if (d)
    d->protocol = protocol;
}

void WebSslInfo::setCertificateChain(const QByteArray& chain)
{
  if (d)
    d->certificateChain = QSslCertificate::fromData(chain);
}

void WebSslInfo::setCiphers(const QString& ciphers)
{
  if (d)
    d->ciphers = ciphers;
}

void WebSslInfo::setUsedCipherBits(const QString& bits)
{
  if (d)
    d->usedCipherBits = bits.toInt();
}

void WebSslInfo::setSupportedCipherBits(const QString& bits)
{
  if (d)  
    d->supportedCipherBits = bits.toInt();
}

void WebSslInfo::setCertificateErrors(const QString& certErrors)
{
  if (d)  
    d->certErrors = certErrors;
}
