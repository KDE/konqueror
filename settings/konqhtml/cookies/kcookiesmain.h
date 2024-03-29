/*
    kcookiesmain.h - Cookies configuration

    First version of cookies configuration:
        SPDX-FileCopyrightText: Waldo Bastian <bastian@kde.org>
    This dialog box:
        SPDX-FileCopyrightText: David Faure <faure@kde.org>
*/

#ifndef __KCOOKIESMAIN_H
#define __KCOOKIESMAIN_H

#include <KCModule>

class QTabWidget;
class KCookiesPolicies;
class KCookiesManagement;

class KCookiesMain : public KCModule
{
    Q_OBJECT
public:
    //TODO KF6: when dropping compatibility with KF5, remove QVariantList argument
    KCookiesMain(QObject *parent, const KPluginMetaData &md={}, const QVariantList &args={});
    ~KCookiesMain() override;

    KCookiesPolicies *policyDlg()
    {
        return policies;
    }

    void save() override;
    void load() override;
    void defaults() override;

private:
    /**
     * @brief Calls setNeedsSave according to the status of the two submodules
     */
    void updateNeedsSave();

private:
    QTabWidget *tab;
    KCookiesPolicies *policies;
    KCookiesManagement *management;
};

#endif // __KCOOKIESMAIN_H
