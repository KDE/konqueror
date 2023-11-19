/*
    kcookiespolicies.h - Cookies configuration

    Original Authors
    SPDX-FileCopyrightText: Waldo Bastian <bastian@kde.org>
    SPDX-FileCopyrightText: 1999 David Faure <faure@kde.org>

    Re-written by:
    SPDX-FileCopyrightText: 2000 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KCOOKIESPOLICIES_H
#define KCOOKIESPOLICIES_H

#include <KCModule>
#include <QMap>

#include "kcookiespolicyselectiondlg.h"
#include "ui_kcookiespolicies.h"

class QTreeWidgetItem;

class KCookiesPolicies : public KCModule
{
    Q_OBJECT

public:
    //TODO KF6: when dropping compatibility with KF5, remove QVariantList argument
    explicit KCookiesPolicies(QObject *parent, const KPluginMetaData &md={}, const QVariantList &args={});
    ~KCookiesPolicies() override;

    void load() override;
    void save() override;
    void defaults() override;

    void setPolicy(const QString &domain);

#if QT_VERSION_MAJOR < 6
    void setNeedsSave(bool needs) {emit changed(needs);}
#endif

protected Q_SLOTS:
    void cookiesEnabled(bool);
    void configChanged();

    void selectionChanged();
    void updateButtons();

    void deleteAllPressed();
    void deletePressed();
    void changePressed();
    void addPressed();
    void changePressed(QTreeWidgetItem *, bool state = true);
    void addPressed(const QString &, bool state = true);

private:
    void updateDomainList(const QStringList &list);
    bool handleDuplicate(const QString &domain, KonqInterfaces::CookieJar::CookieAdvice advice);
    void splitDomainAdvice(const QString &configStr, QString &domain, KonqInterfaces::CookieJar::CookieAdvice &advice);

private:
    quint64 mSelectedItemsCount;
    Ui::KCookiePoliciesUI mUi;
    QMap<QString, KonqInterfaces::CookieJar::CookieAdvice> mDomainPolicyMap;
};

#endif // KCOOKIESPOLICIES_H
