/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2001 Carsten Pfeiffer <pfeiffer@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SETTINGS_PLUGIN
#define SETTINGS_PLUGIN

#include <kparts_version.h>
#include <konq_kpart_plugin.h>

class KConfig;

namespace KonqInterfaces {
    class CookieJar;
}

class SettingsPlugin : public KonqParts::Plugin
{
    Q_OBJECT
public:
    SettingsPlugin(QObject *parent,
                   const KPluginMetaData& metaData,
                   const QVariantList &);
    ~SettingsPlugin() override;

private:
    bool cookiesEnabled(const QString &url);
    void updateIOSlaves();
    int proxyType();
    KonqInterfaces::CookieJar *cookieJar() const;

private slots:
    void toggleJavascript(bool checked);
    void toggleJava(bool checked);
    void toggleCookies(bool checked);
    void togglePlugins(bool checked);
    void toggleImageLoading(bool checked);
    void toggleProxy(bool checked);
    void toggleCache(bool checked);
    void cachePolicyChanged(int p);

    void showPopup();

private:
    KConfig *mConfig;
};

#endif // SETTINGS_PLUGIN
