/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "cache.h"
#include "ui_cache.h"

#include "konqsettings.h"

#include <QDBusMessage>
#include <QDBusConnection>

int constexpr conversionFactor = 1000000;

Cache::Cache(QObject *parent, const KPluginMetaData &md, const QVariantList &): KCModule(parent, md), m_ui(new Ui::Cache)
{
    m_ui->setupUi(widget());
    connect(m_ui->memoryCache, &QCheckBox::toggled, this, &Cache::toggleMemoryCache);
    connect(m_ui->cacheSize, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int){markAsChanged();});
    auto changedBoolArg = [this](bool){markAsChanged();};
    connect(m_ui->memoryCache, &QCheckBox::clicked, this, changedBoolArg);
    connect(m_ui->cacheEnabled, &QGroupBox::clicked, this, changedBoolArg);
    connect(m_ui->useCustomCacheDir, &QGroupBox::clicked, this, changedBoolArg);
    connect(m_ui->customCacheDir, &KUrlRequester::textChanged, this, [this](const QString &){markAsChanged();});
}

Cache::~Cache()
{
}

void Cache::defaults()
{
    Konq::Settings::self()->withDefaults([this]{load();});
    setNeedsSave(true);
    setRepresentsDefaults(true);
    KCModule::defaults();
}

void Cache::load()
{
    m_ui->cacheEnabled->setChecked(Konq::Settings::cacheEnabled());
    m_ui->memoryCache->setChecked(Konq::Settings::keepCacheInMemory());
    int maxSizeInBytes = Konq::Settings::maximumCacheSize();
    //Ensure that maxSizeInMB is greater than 0 if maxSizeInBytes is not 0
    int maxSizeInMB = maxSizeInBytes == 0 ? 0 : std::max(1, maxSizeInBytes / conversionFactor);
    m_ui->cacheSize->setValue(maxSizeInMB);
    QString path = Konq::Settings::customCacheDir();
    m_ui->useCustomCacheDir->setChecked(!path.isEmpty());
    m_ui->customCacheDir->setUrl(QUrl::fromLocalFile(path));
    KCModule::load();
}

void Cache::save()
{
    bool cacheEnabled = m_ui->cacheEnabled->isChecked();
    Konq::Settings::setCacheEnabled(cacheEnabled);
    Konq::Settings::setKeepCacheInMemory(m_ui->memoryCache->isChecked());
    //We store the size in bytes, not in MB
    Konq::Settings::setMaximumCacheSize(m_ui->cacheSize->value()*conversionFactor);
    QString path = m_ui->customCacheDir->isEnabled() ? m_ui->customCacheDir->url().path() : QString();
    Konq::Settings::setCustomCacheDir(path);
    Konq::Settings::self()->save();

    QDBusMessage message =
        QDBusMessage::createSignal(QStringLiteral("/KonqMain"), QStringLiteral("org.kde.Konqueror.Main"), QStringLiteral("reparseConfiguration"));
    QDBusConnection::sessionBus().send(message);
    KCModule::save();
}

void Cache::toggleMemoryCache(bool on)
{
    m_ui->useCustomCacheDir->setEnabled(!on);
}
