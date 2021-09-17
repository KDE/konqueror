/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
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
