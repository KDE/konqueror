//  This file is part of the KDE project
//  SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
// 
//  SPDX-License-Identifier: LGPL-2.0-or-later

#include "konqsettingsbase.h"

#include <QJsonDocument>
#include <QJsonObject>

using namespace Konq;

SettingsBase::SettingsBase(const QString& configname, QObject* parent) : KConfigSkeleton(configname, parent)
{
}

SettingsBase::SettingsBase(KSharedConfig::Ptr config, QObject* parent) : KConfigSkeleton(config, parent)
{
}

SettingsBase::CookieAdvice SettingsBase::intToAdvice(int value, CookieAdvice defaultValue)
{
    if (value < 1 || value > static_cast<int>(CookieAdvice::Ask)) { //Ask is the last value
        return defaultValue;
    }
    return static_cast<CookieAdvice>(value);
}

//TODO Settings: remove once settings are stored using enums instead of strings
KConfigSkeleton::ItemInt* SettingsBase::cookieGlobalAdviceItem() const
{
    return dynamic_cast<KCoreConfigSkeleton::ItemInt*>(findItem("CookieGlobalAdviceRaw"));
}

//TODO Settings: remove once settings are stored using enums instead of strings
SettingsBase::CookieAdvice SettingsBase::cookieGlobalAdvice() const
{
    KConfigSkeleton::ItemInt *it = cookieGlobalAdviceItem();
    if (!it) {
        return defaultCookieAdvice();
    }
    return intToAdvice(it->value());
}

//TODO Settings: remove once settings are stored using enums instead of strings
void SettingsBase::setCookieGlobalAdvice(CookieAdvice advice)
{
    KConfigSkeleton::ItemInt *it = cookieGlobalAdviceItem();
    if (!it) {
        return;
    }
    it->setValue(static_cast<int>(advice));
}

KConfigSkeleton::ItemString * Konq::SettingsBase::cookieDomainAdviceItem() const
{
    return dynamic_cast<KCoreConfigSkeleton::ItemString*>(findItem(QStringLiteral("CookieDomainAdviceRaw")));
}

QHash<QString, SettingsBase::CookieAdvice> SettingsBase::cookieDomainAdvice() const
{
    KConfigSkeleton::ItemString *it = cookieDomainAdviceItem();
    if (!it) {
        return {};
    }
    QJsonObject obj = QJsonDocument::fromJson(it->value().toUtf8()).object();
    QHash<QString, CookieAdvice> res;
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        res.insert(it.key(), intToAdvice(it.value().toInt()));
    }
    return res;
}

void SettingsBase::setCookieDomainAdvice(const QHash<QString, CookieAdvice>& advice)
{
    KConfigSkeleton::ItemString *it = cookieDomainAdviceItem();
    if (!it) {
        return;
    }
    QJsonObject obj;
    for (auto it = advice.constBegin(); it != advice.constEnd(); ++it) {
        obj.insert(it.key(), static_cast<int>(it.value()));
    }
    it->setValue(QString(QJsonDocument(obj).toJson()));
}

KConfigGroup Konq::SettingsBase::certificateExceptionsGroup() const
{
    return sharedConfig()->group("CertificateExceptions");
}

void Konq::SettingsBase::usrRead()
{
    KConfigGroup grp = certificateExceptionsGroup();
    m_certificateExceptions.data.clear();
    for (QString url : grp.keyList()) {
        QList<int> exceptions = grp.readEntry(url, QList<int>{});
        if (!exceptions.isEmpty()) {
            m_certificateExceptions.data.insert(url, exceptions);
        }
    }
    m_certificateExceptions.dirty = false;
    KConfigSkeleton::usrRead();
}

bool Konq::SettingsBase::usrSave()
{
    if (!m_certificateExceptions.dirty) {
        return KConfigSkeleton::usrSave();
    }
    KConfigGroup grp = certificateExceptionsGroup();

    for (QString url : grp.keyList()) {
        grp.deleteEntry(url);
    }
    for (auto it = m_certificateExceptions.data.constBegin(); it != m_certificateExceptions.data.constEnd(); ++it) {
        grp.writeEntry(it.key(), it.value());
    }
    m_certificateExceptions.dirty = false;
    return KConfigSkeleton::usrSave();
}

void Konq::SettingsBase::usrSetDefaults()
{
    if (m_certificateExceptions.data.isEmpty()) {
        return;
    }
    m_certificateExceptions.data.clear();
    m_certificateExceptions.dirty = true;
}

bool Konq::SettingsBase::usrUseDefaults(bool b)
{
    m_isUsingDefaults = b;
    return KConfigSkeleton::usrUseDefaults(b);
}

QList<int> Konq::SettingsBase::certificateExceptions(const QString& url) const
{
    if (m_isUsingDefaults) {
        return {};
    }
    return m_certificateExceptions.data.value(url);
}

bool Konq::SettingsBase::addCertificateException(const QString& url, int exception)
{
    //exceptionList must be a reference because we want to modify the object inside
    //the hash, not a copy of it
    QList<int> &exceptionList = m_certificateExceptions.data[url];
    if (exceptionList.contains(exception)) {
        return false;
    }
    exceptionList.append(exception);
    m_certificateExceptions.dirty = true;
    return true;
}

QList<Konq::SettingsBase::SpeedDialEntry> Konq::SettingsBase::speedDialEntries() const
{
    KConfigGroup grp = config()->group("SpeedDial");
    int count = grp.readEntry("EntriesCount", 0);
    QList<SpeedDialEntry> entries;
    entries.reserve(count);
    for (int i = 0; i < count; ++i) {
        QString suffix = QString::number(i);
        SpeedDialEntry e {
            grp.readEntry(QStringLiteral("Name") + suffix),
            grp.readEntry(QStringLiteral("Url") + suffix, QUrl{}),
            grp.readEntry(QStringLiteral("Icon") + suffix, QUrl{})
        };
        entries.append(e);
    }
    return entries;
}

void Konq::SettingsBase::setSpeedDialEntries(const QList<SpeedDialEntry>& entries)
{
    KConfigGroup grp = config()->group("SpeedDial");
    grp.writeEntry("EntriesCount", entries.length());
    for (int i = 0; i < entries.length(); ++i) {
        QString suffix = QString::number(i);
        SpeedDialEntry e = entries.at(i);
        grp.writeEntry(QStringLiteral("Name") + suffix, e.name);
        grp.writeEntry(QStringLiteral("Url") + suffix, e.url);
        grp.writeEntry(QStringLiteral("Icon") + suffix, e.iconUrl);
    }
}

size_t Konq::qHash(const Konq::SettingsBase::SpeedDialEntry& e, size_t seed)
{
    return qHashMulti(seed, e.name, e.url, e.iconUrl);
}


QDebug Konq::operator<<(QDebug dbg, SettingsBase::CookieAdvice advice)
{
    QDebugStateSaver saver(dbg);
    switch (advice) {
        case SettingsBase::CookieAdvice::Accept:
            dbg.nospace() << "Accept";
            break;
        case SettingsBase::CookieAdvice::AcceptForSession:
            dbg.nospace() << "Accept for session";
            break;
        case SettingsBase::CookieAdvice::Reject:
            dbg.nospace() << "Reject";
            break;
        case SettingsBase::CookieAdvice::Ask:
            dbg.nospace() << "Ask";
            break;
    }

    return dbg;
}
