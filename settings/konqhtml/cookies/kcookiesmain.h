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
    KCookiesMain(QObject *parent, const KPluginMetaData &md={});
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

    /**
     * @brief The currently visible KCM
     * @return #policies if the policy module is the active one and #management otherwise
     */
    KCModule *currentModule() const;

private:
    QTabWidget *tab;
    KCookiesPolicies *policies;
    KCookiesManagement *management;
};

#endif // __KCOOKIESMAIN_H
