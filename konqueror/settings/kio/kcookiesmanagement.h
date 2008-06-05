/**
 * kcookiesmanagement.h - Cookies manager
 *
 * Copyright 2000-2001 Marco Pinelli <pinmc@orion.it>
 *
 * Contributors:
 * Copyright (c) 2000-2001 Dawit Alemayehu <adawit@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef KCOOKIESMANAGEMENT_H
#define KCOOKIESMANAGEMENT_H

#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtGui/QTreeWidget>
#include <QtCore/QMultiHash>
#include <Qt3Support/Q3Dict>
#include <Qt3Support/Q3PtrList>

#include <kcmodule.h>
#include "ui_kcookiesmanagementdlg.h"


struct CookieProp;

class KCookiesManagementDlgUI : public QWidget, public Ui::KCookiesManagementDlgUI
{
public:
  KCookiesManagementDlgUI( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};


class CookieListViewItem : public QTreeWidgetItem
{
public:
    CookieListViewItem(QTreeWidget *parent, const QString &dom);
    CookieListViewItem(QTreeWidgetItem *parent, CookieProp *cookie);
    ~CookieListViewItem();

    QString domain() const { return mDomain; }
    CookieProp* cookie() const { return mCookie; }
    CookieProp* leaveCookie();
    void setCookiesLoaded() { mCookiesLoaded = true; }
    bool cookiesLoaded() const { return mCookiesLoaded; }

private:
    void init( CookieProp* cookie,
               const QString &domain = QString(),
               bool cookieLoaded=false );
    CookieProp *mCookie;
    QString mDomain;
    bool mCookiesLoaded;
};

class KCookiesManagement : public KCModule
{
    Q_OBJECT

public:
    KCookiesManagement(const KComponentData &componentData, QWidget *parent );
    ~KCookiesManagement();

    virtual void load();
    virtual void save();
    virtual void defaults();
    virtual QString quickHelp() const;

private Q_SLOTS:
    void deleteCookie();
    void deleteAllCookies();
    void getDomains();
    void getCookies(QTreeWidgetItem*);
    void showCookieDetails(QTreeWidgetItem*);
    void doPolicy();

private:
    void reset (bool deleteAll = false);
    bool cookieDetails(CookieProp *cookie);
    void clearCookieDetails();
    bool policyenabled();
    bool m_bDeleteAll;

    QWidget* mainWidget;
    KCookiesManagementDlgUI* dlg;

    QStringList deletedDomains;
    typedef Q3PtrList<CookieProp> CookiePropList;
    Q3Dict<CookiePropList> deletedCookies;
};

#endif // KCOOKIESMANAGEMENT_H
