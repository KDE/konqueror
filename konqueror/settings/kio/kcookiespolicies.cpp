/**
 * kcookiespolicies.cpp - Cookies configuration
 *
 * Original Authors
 * Copyright (c) Waldo Bastian <bastian@kde.org>
 * Copyright (c) 1999 David Faure <faure@kde.org>
 * Copyright (c) 2008 Urs Wolfer <uwolfer @ kde.org>
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

// Own
#include "kcookiespolicies.h"

// Local
#include "ksaveioconfig.h"

// Qt
#include <QLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QRadioButton>
#include <QToolButton>
#include <QBoxLayout>
#include <QtDBus/QtDBus>

// KDE
#include <kiconloader.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kconfig.h>
#include <kurl.h>
#include <kdebug.h>


// QUrl::fromAce/toAce don't accept a domain that starts with a '.', like we do here.
// So we use these wrappers.
QString tolerantFromAce (const QByteArray& _domain)
{
    QByteArray domain (_domain);
    const bool hasDot = domain.startsWith ('.');
    if (hasDot)
        domain.remove (0, 1);
    QString ret = QUrl::fromAce (domain);
    if (hasDot) {
        ret.prepend ('.');
    }
    return ret;
}

static QByteArray tolerantToAce (const QString& _domain)
{
    QString domain (_domain);
    const bool hasDot = domain.startsWith ('.');
    if (hasDot)
        domain.remove (0, 1);
    QByteArray ret = QUrl::toAce (domain);
    if (hasDot) {
        ret.prepend ('.');
    }
    return ret;
}


KCookiesPolicies::KCookiesPolicies (const KComponentData& componentData, QWidget* parent)
    : KCModule (componentData, parent)
{
    mUi.setupUi (this);
    mUi.kListViewSearchLine->setTreeWidget (mUi.policyTreeWidget);
    QList<int> columns;
    columns.append (0);
    mUi.kListViewSearchLine->setSearchColumns (columns);

    mUi.pbNew->setIcon (KIcon ("list-add"));
    mUi.pbChange->setIcon (KIcon ("edit-rename"));
    mUi.pbDelete->setIcon (KIcon ("list-remove"));
    mUi.pbDeleteAll->setIcon (KIcon ("edit-delete"));

    // Connect the main swicth :) Enable/disable cookie support
    connect (mUi.cbEnableCookies, SIGNAL (toggled (bool)),
             SLOT (cookiesEnabled (bool)));
    connect (mUi.cbEnableCookies, SIGNAL (toggled (bool)),
             SLOT (configChanged()));

    // Connect the preference check boxes...
    connect (mUi.cbRejectCrossDomainCookies, SIGNAL (toggled (bool)),
             SLOT (configChanged()));
    connect (mUi.cbAutoAcceptSessionCookies, SIGNAL (toggled (bool)),
             SLOT (configChanged()));

    connect (mUi.rbPolicyAsk, SIGNAL (toggled (bool)),
             SLOT (configChanged()));
    connect (mUi.rbPolicyAccept, SIGNAL (toggled (bool)),
             SLOT (configChanged()));
    connect (mUi.rbPolicyAcceptForSession, SIGNAL(toggled(bool)),
              SLOT(configChanged()));
    connect (mUi.rbPolicyReject, SIGNAL (toggled (bool)),
             SLOT (configChanged()));
    // Connect signals from the domain specific policy listview.
    connect (mUi.policyTreeWidget, SIGNAL (itemSelectionChanged()),
             SLOT (selectionChanged()));
    connect (mUi.policyTreeWidget, SIGNAL (itemDoubleClicked (QTreeWidgetItem*, int)),
             SLOT (changePressed()));

    // Connect the buttons...
    connect (mUi.pbNew, SIGNAL (clicked()), SLOT (addPressed()));
    connect (mUi.pbChange, SIGNAL (clicked()), SLOT (changePressed()));
    connect (mUi.pbDelete, SIGNAL (clicked()), SLOT (deletePressed()));
    connect (mUi.pbDeleteAll, SIGNAL (clicked()), SLOT (deleteAllPressed()));
}

KCookiesPolicies::~KCookiesPolicies()
{
}

void KCookiesPolicies::configChanged ()
{
    //kDebug() << "KCookiesPolicies::configChanged...";
    emit changed (true);
}

void KCookiesPolicies::cookiesEnabled (bool enable)
{
    mUi.bgDefault->setEnabled (enable);
    mUi.bgPreferences->setEnabled (enable);
    mUi.gbDomainSpecific->setEnabled (enable);
}

void KCookiesPolicies::setPolicy (const QString& domain)
{
    QTreeWidgetItemIterator it (mUi.policyTreeWidget);
    bool hasExistingPolicy = false;
    while (*it) {
        if ((*it)->text(0) == domain) {
            hasExistingPolicy = true;
            break;
        }
        ++it;
    }

    if (hasExistingPolicy) {
        changePressed((*it), false);
    } else {
        addPressed(domain);
    }
}

void KCookiesPolicies::changePressed()
{
    changePressed(mUi.policyTreeWidget->currentItem());
}

void KCookiesPolicies::addPressed()
{
    addPressed(QString());
}

void KCookiesPolicies::changePressed(QTreeWidgetItem* item, bool state)
{
    Q_ASSERT(item);
    QString oldDomain = item->text (0);

    KCookiesPolicySelectionDlg pdlg (this);
    pdlg.setWindowTitle (i18nc ("@title:window", "Change Cookie Policy"));
    pdlg.setPolicy (KCookieAdvice::strToAdvice (mDomainPolicyMap.value(item)));
    pdlg.setEnableHostEdit (state, oldDomain);

    if (pdlg.exec() && !pdlg.domain().isEmpty()) {
        QString newDomain = tolerantFromAce (pdlg.domain().toLatin1());
        int advice = pdlg.advice();
        if (newDomain == oldDomain || !handleDuplicate (newDomain, advice)) {
            mDomainPolicyMap[item] = KCookieAdvice::adviceToStr(advice);
            item->setText (0, newDomain);
            item->setText (1, i18n (mDomainPolicyMap.value(item)));
            configChanged();
        }
    }
}

void KCookiesPolicies::addPressed(const QString& domain, bool state)
{
    KCookiesPolicySelectionDlg pdlg (this);
    pdlg.setWindowTitle (i18nc ("@title:window", "New Cookie Policy"));
    pdlg.setEnableHostEdit(state, domain);

    if (mUi.rbPolicyAccept->isChecked())
        pdlg.setPolicy (KCookieAdvice::Reject);
    else
        pdlg.setPolicy (KCookieAdvice::Accept);

    if (pdlg.exec() && !pdlg.domain().isEmpty()) {
        const QString domain = tolerantFromAce (pdlg.domain().toLatin1());
        int advice = pdlg.advice();

        if (!handleDuplicate (domain, advice)) {
            const char* strAdvice = KCookieAdvice::adviceToStr (advice);
            QTreeWidgetItem* item = new QTreeWidgetItem (mUi.policyTreeWidget,
                    QStringList() << domain << i18n (strAdvice));
            mDomainPolicyMap.insert (item, strAdvice);
            configChanged();
            updateButtons();
        }
    }
}

bool KCookiesPolicies::handleDuplicate (const QString& domain, int advice)
{
    QTreeWidgetItem* item = mUi.policyTreeWidget->topLevelItem (0);
    while (item != 0) {
        if (item->text (0) == domain) {
            QString msg = i18n ("<qt>A policy already exists for"
                                "<center><b>%1</b></center>"
                                "Do you want to replace it?</qt>", domain);
            int res = KMessageBox::warningContinueCancel (this, msg,
                      i18nc ("@title:window", "Duplicate Policy"),
                      KGuiItem (i18n ("Replace")));
            if (res == KMessageBox::Continue) {
                mDomainPolicyMap[item] = KCookieAdvice::adviceToStr(advice);
                item->setText (0, domain);
                item->setText (1, i18n (mDomainPolicyMap.value(item)));
                configChanged();
                return true;
            } else
                return true;  // User Cancelled!!
        }
        item = mUi.policyTreeWidget->itemBelow (item);
    }
    return false;
}

void KCookiesPolicies::deletePressed()
{
    QTreeWidgetItem* nextItem = 0L;

    Q_FOREACH (QTreeWidgetItem * item, mUi.policyTreeWidget->selectedItems()) {
        nextItem = mUi.policyTreeWidget->itemBelow (item);
        if (!nextItem)
            nextItem = mUi.policyTreeWidget->itemAbove (item);

        mDomainPolicyMap.remove (item);
        delete item;
    }

    if (nextItem)
        nextItem->setSelected (true);

    updateButtons();
    configChanged();
}

void KCookiesPolicies::deleteAllPressed()
{
    mDomainPolicyMap.clear();
    mUi.policyTreeWidget->clear();
    updateButtons();
    configChanged();
}

void KCookiesPolicies::updateButtons()
{
    bool hasItems = mUi.policyTreeWidget->topLevelItemCount() > 0;

    mUi.pbChange->setEnabled ( (hasItems && mSelectedItemsCount == 1));
    mUi.pbDelete->setEnabled ( (hasItems && mSelectedItemsCount > 0));
    mUi.pbDeleteAll->setEnabled (hasItems);
}

void KCookiesPolicies::updateDomainList (const QStringList& domainConfig)
{
    mUi.policyTreeWidget->clear();

    QStringList::ConstIterator it = domainConfig.begin();
    for (; it != domainConfig.end(); ++it) {
        QString domain;
        KCookieAdvice::Value advice = KCookieAdvice::Dunno;

        splitDomainAdvice (*it, domain, advice);

        if (!domain.isEmpty()) {
            QStringList items;
            items << tolerantFromAce(domain.toLatin1()) << i18n(KCookieAdvice::adviceToStr(advice));
            QTreeWidgetItem* item = new QTreeWidgetItem (mUi.policyTreeWidget, items);
            mDomainPolicyMap[item] = KCookieAdvice::adviceToStr(advice);
        }
    }    

    mUi.policyTreeWidget->sortItems(0, Qt::AscendingOrder);
}

void KCookiesPolicies::selectionChanged ()
{
    mSelectedItemsCount = mUi.policyTreeWidget->selectedItems().count();

    updateButtons ();
}

void KCookiesPolicies::load()
{
    mSelectedItemsCount = 0;

    KConfig cfg ("kcookiejarrc");
    KConfigGroup group = cfg.group ("Cookie Policy");

    bool enableCookies = group.readEntry ("Cookies", true);
    mUi.cbEnableCookies->setChecked (enableCookies);
    cookiesEnabled (enableCookies);

    // Warning: the default values are duplicated in kcookiejar.cpp
    KCookieAdvice::Value advice = KCookieAdvice::strToAdvice (group.readEntry (
                                      "CookieGlobalAdvice", "Accept"));
    switch (advice) {
    case KCookieAdvice::Accept:
        mUi.rbPolicyAccept->setChecked (true);
        break;
    case KCookieAdvice::AcceptForSession:
        mUi.rbPolicyAcceptForSession->setChecked (true);
        break;
    case KCookieAdvice::Reject:
        mUi.rbPolicyReject->setChecked (true);
        break;
    case KCookieAdvice::Ask:
    case KCookieAdvice::Dunno:
    default:
        mUi.rbPolicyAsk->setChecked (true);
    }

    bool enable = group.readEntry ("RejectCrossDomainCookies", true);
    mUi.cbRejectCrossDomainCookies->setChecked (enable);

    bool sessionCookies = group.readEntry ("AcceptSessionCookies", true);
    mUi.cbAutoAcceptSessionCookies->setChecked (sessionCookies);
    updateDomainList (group.readEntry ("CookieDomainAdvice", QStringList()));

    if (enableCookies) {
        updateButtons();
    }
}

void KCookiesPolicies::save()
{
    KConfig cfg ("kcookiejarrc");
    KConfigGroup group = cfg.group ("Cookie Policy");

    bool state = mUi.cbEnableCookies->isChecked();
    group.writeEntry ("Cookies", state);
    state = mUi.cbRejectCrossDomainCookies->isChecked();
    group.writeEntry ("RejectCrossDomainCookies", state);
    state = mUi.cbAutoAcceptSessionCookies->isChecked();
    group.writeEntry ("AcceptSessionCookies", state);

    QString advice;
    if (mUi.rbPolicyAccept->isChecked())
        advice = KCookieAdvice::adviceToStr (KCookieAdvice::Accept);
    else if (mUi.rbPolicyAcceptForSession->isChecked())
        advice = KCookieAdvice::adviceToStr (KCookieAdvice::AcceptForSession);
    else if (mUi.rbPolicyReject->isChecked())
        advice = KCookieAdvice::adviceToStr (KCookieAdvice::Reject);
    else
        advice = KCookieAdvice::adviceToStr (KCookieAdvice::Ask);

    group.writeEntry ("CookieGlobalAdvice", advice);

    QStringList domainConfig;
    QMapIterator<QTreeWidgetItem*, const char*> it (mDomainPolicyMap);
    while (it.hasNext()) {
        it.next();
        QTreeWidgetItem* item = it.key();
        QString policy = tolerantToAce (item->text (0));
        policy += QLatin1Char (':');
        policy += QLatin1String (it.value());
        domainConfig << policy;
    }

    group.writeEntry ("CookieDomainAdvice", domainConfig);
    group.sync();

    // Update the cookiejar...
    if (!mUi.cbEnableCookies->isChecked()) {
        QDBusInterface kded ("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer", QDBusConnection::sessionBus());
        kded.call ("shutdown");
    } else {
        QDBusInterface kded ("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer", QDBusConnection::sessionBus());
        QDBusReply<void> reply = kded.call ("reloadPolicy");
        if (!reply.isValid())
            KMessageBox::sorry (0, i18n ("Unable to communicate with the cookie handler service.\n"
                                         "Any changes you made will not take effect until the service "
                                         "is restarted."));
    }

    // Force running io-slave to reload configurations...
    KSaveIOConfig::updateRunningIOSlaves (this);
    emit changed (false);
}


void KCookiesPolicies::defaults()
{
    mUi.cbEnableCookies->setChecked (true);
    mUi.rbPolicyAsk->setChecked (true);
    mUi.rbPolicyAccept->setChecked (false);
    mUi.rbPolicyAcceptForSession->setChecked (false);
    mUi.rbPolicyReject->setChecked (false);
    mUi.cbRejectCrossDomainCookies->setChecked (true);
    mUi.cbAutoAcceptSessionCookies->setChecked (false);
    mUi.policyTreeWidget->clear();
    mDomainPolicyMap.clear();

    cookiesEnabled (mUi.cbEnableCookies->isChecked());
    updateButtons();
}

void KCookiesPolicies::splitDomainAdvice (const QString& cfg, QString& domain,
        KCookieAdvice::Value& advice)
{
    int sepPos = cfg.lastIndexOf (':');

    // Ignore any policy that does not contain a domain...
    if (sepPos <= 0)
        return;

    domain = cfg.left (sepPos);
    advice = KCookieAdvice::strToAdvice (cfg.mid (sepPos + 1));
}

QString KCookiesPolicies::quickHelp() const
{
    return i18n ("<p><h1>Cookies</h1> Cookies contain information that Konqueror"
                 " (or any other KDE application using the HTTP protocol) stores"
                 " on your computer from a remote Internet server. This means"
                 " that a web server can store information about you and your"
                 " browsing activities on your machine for later use. You might"
                 " consider this an invasion of privacy.</p><p>However, cookies are"
                 " useful in certain situations. For example, they are often used"
                 " by Internet shops, so you can 'put things into a shopping"
                 " basket'. Some sites require you have a browser that supports"
                 " cookies.</p><p>Because most people want a compromise between privacy"
                 " and the benefits cookies offer, KDE offers you the ability to"
                 " customize the way it handles cookies. You might, for example"
                 " want to set KDE's default policy to ask you whenever a server"
                 " wants to set a cookie or simply reject or accept everything."
                 " For example, you might choose to accept all cookies from your"
                 " favorite shopping web site. For this all you have to do is"
                 " either browse to that particular site and when you are presented"
                 " with the cookie dialog box, click on <i> This domain </i> under"
                 " the 'apply to' tab and choose accept or simply specify the name"
                 " of the site in the <i> Domain Specific Policy </i> tab and set"
                 " it to accept. This enables you to receive cookies from trusted"
                 " web sites without being asked every time KDE receives a cookie.</p>"
                );
}

#include "kcookiespolicies.moc"
