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
    KCookiesMain(QWidget *parent, const QVariantList &args);
    ~KCookiesMain() override;

    KCookiesPolicies *policyDlg()
    {
        return policies;
    }

    void save() override;
    void load() override;
    void defaults() override;
    QString quickHelp() const override;

private:
    QTabWidget *tab;
    KCookiesPolicies *policies;
    KCookiesManagement *management;
};

#endif // __KCOOKIESMAIN_H
