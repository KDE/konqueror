/*
    kcookiesmanagement.h - Cookies manager

    SPDX-FileCopyrightText: 2000-2001 Marco Pinelli <pinmc@orion.it>
    SPDX-FileCopyrightText: 2000-2001 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KCOOKIESMANAGEMENT_H
#define KCOOKIESMANAGEMENT_H

#include <QHash>
#include <QList>
#include <QStringList>
#include <QTreeWidget>
#include <QNetworkCookie>

#include "ui_kcookiesmanagement.h"
#include <KCModule>

struct CookieProp;

class CookieListViewItem : public QTreeWidgetItem
{
public:
    CookieListViewItem(QTreeWidget *parent, const QString &dom);
    CookieListViewItem(QTreeWidgetItem *parent, CookieProp *cookie);
    ~CookieListViewItem() override;

    QString domain() const
    {
        return mDomain;
    }
    CookieProp *cookie() const
    {
        return mCookie;
    }
    CookieProp *leaveCookie();
    void setCookiesLoaded()
    {
        mCookiesLoaded = true;
    }
    bool cookiesLoaded() const
    {
        return mCookiesLoaded;
    }

private:
    void init(CookieProp *cookie, const QString &domain = QString(), bool cookieLoaded = false);
    CookieProp *mCookie;
    QString mDomain;
    bool mCookiesLoaded;
};

class KCookiesManagement : public KCModule
{
    Q_OBJECT

public:
    explicit KCookiesManagement(QWidget *parent, const QVariantList &args);
    ~KCookiesManagement() override;

    void load() override;
    void save() override;
    void defaults() override;
    QString quickHelp() const override;

private Q_SLOTS:
    void deleteCurrent();
    void deleteAll();
    void reload();
    void listCookiesForDomain(QTreeWidgetItem *);
    void updateForItem(QTreeWidgetItem *);
    void showConfigPolicyDialog();

private:
    void reset(bool deleteAll = false);
    bool cookieDetails(CookieProp *cookie);
    void clearCookieDetails();

    typedef QSet<QNetworkCookie> CookieSet;
    static CookieSet getCookies();

    bool mDeleteAllFlag;
    QWidget *mMainWidget;
    Ui::KCookiesManagementUI mUi;

    QStringList mDeletedDomains;
    typedef QList<CookieProp *> CookiePropList;
    QHash<QString, CookiePropList> mDeletedCookies;
};

#endif // KCOOKIESMANAGEMENT_H
