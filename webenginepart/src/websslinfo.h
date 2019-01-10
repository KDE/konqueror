/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2009 Dawit Alemayehu <adawit@kde.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef WEBSSLINFO_H
#define WEBSSLINFO_H

#include <QUrl>
#include <QList>
#include <QString>
#include <QHostAddress>
#include <QSslCertificate>

class WebSslInfo
{
public:
  WebSslInfo();
  WebSslInfo(const WebSslInfo&);
  virtual ~WebSslInfo();

  bool isValid() const;
  QUrl url() const;
  QHostAddress peerAddress() const;
  QHostAddress parentAddress() const;
  QString ciphers() const;
  QString protocol() const;
  QString certificateErrors() const;
  int supportedChiperBits () const;
  int usedChiperBits () const;
  QList<QSslCertificate> certificateChain() const;

  bool saveTo(QMap<QString, QVariant>&) const;
  void restoreFrom(const QVariant &, const QUrl& = QUrl(), bool reset = false);

  void setUrl (const QUrl &url);
  WebSslInfo& operator = (const WebSslInfo&);

protected:
  void setCiphers(const QString& ciphers);
  void setProtocol(const QString& protocol);
  void setPeerAddress(const QString& address);
  void setParentAddress(const QString& address);
  void setCertificateChain(const QByteArray& chain);
  void setCertificateErrors(const QString& certErrors);
  void setUsedCipherBits(const QString& bits);
  void setSupportedCipherBits(const QString& bits);

private:
  class WebSslInfoPrivate;
  WebSslInfoPrivate* d;
};

#endif // WEBSSLINFO_H
