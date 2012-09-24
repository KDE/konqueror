/**
 * kcookiespolicies.h - Cookies configuration
 *
 * Original Authors
 * Copyright (c) Waldo Bastian <bastian@kde.org>
 * Copyright (c) 1999 David Faure <faure@kde.org>
 *
 * Re-written by:
 * Copyright (c) 2000- Dawit Alemayehu <adawit@kde.org>
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

#ifndef KCOOKIESPOLICIES_H
#define KCOOKIESPOLICIES_H

#include <QtCore/QMap>
#include <kcmodule.h>

#include "kcookiespolicyselectiondlg.h"
#include "ui_kcookiespolicies.h"


class QTreeWidgetItem;

class KCookiesPolicies : public KCModule
{
    Q_OBJECT

public:
    KCookiesPolicies(const KComponentData &componentData, QWidget *parent);
    ~KCookiesPolicies();

    virtual void load();
    virtual void save();
    virtual void defaults();
    virtual QString quickHelp() const;

    void addNewPolicy(const QString& domain);

protected Q_SLOTS:
    void cookiesEnabled( bool );
    void configChanged();

    void selectionChanged();
    void updateButtons();

    void deleteAllPressed();
    void deletePressed();
    void changePressed();
    void addPressed();

private:
    void updateDomainList(const QStringList& list);
    bool handleDuplicate( const QString& domain, int );
    void splitDomainAdvice (const QString& configStr, QString &domain,
                            KCookieAdvice::Value &advice);

private:
    quint64 mSelectedItemsCount;
    Ui::KCookiePoliciesUI mUi;
    QMap<QTreeWidgetItem*, const char*> mDomainPolicyMap;
};

#endif // KCOOKIESPOLICIES_H
