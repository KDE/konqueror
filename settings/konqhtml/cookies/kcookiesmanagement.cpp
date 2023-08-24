/*
    kcookiesmanagement.cpp - Cookies manager

    SPDX-FileCopyrightText: 2000-2001 Marco Pinelli <pinmc@orion.it>
    SPDX-FileCopyrightText: 2000-2001 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

// Own
#include "kcookiesmanagement.h"

// Qt
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QPushButton>
#include <QDateTime>
#include <QLocale>
#include <QApplication>

// KDE
#include <KLocalizedString>
#include <KMessageBox>

// Local
#include "kcookiesmain.h"
#include "kcookiespolicies.h"
#include "interfaces/browser.h"
#include "interfaces/cookiejar.h"

using namespace KonqInterfaces;

QString tolerantFromAce(const QByteArray &_domain);

struct CookieProp {
    QNetworkCookie cookie;
    QString host;
    bool allLoaded;

    QString expireDate() const {
        QDateTime expDate = cookie.expirationDate();
        if (expDate.isValid()) {
            return QLocale().toString(expDate, QLocale::ShortFormat);
        } else {
            return i18n("End of session");
        }
    }

    QString secure() const {
        return i18n(cookie.isSecure() ? "Yes" : "No");
    }
};

CookieListViewItem::CookieListViewItem(QTreeWidget *parent, const QString &dom)
    : QTreeWidgetItem(parent)
{
    init(nullptr, dom);
}

CookieListViewItem::CookieListViewItem(QTreeWidgetItem *parent, CookieProp *cookie)
    : QTreeWidgetItem(parent)
{
    init(cookie);
}

CookieListViewItem::~CookieListViewItem()
{
    delete mCookie;
}

void CookieListViewItem::init(CookieProp *cookie, const QString &domain, bool cookieLoaded)
{
    mCookie = cookie;
    mDomain = domain;
    mCookiesLoaded = cookieLoaded;

    if (mCookie) {
        if (mDomain.isEmpty()) {
            setText(0, tolerantFromAce(mCookie->host.toLatin1()));
        } else {
            setText(0, tolerantFromAce(mDomain.toLatin1()));
        }
        setText(1, mCookie->cookie.name());
    } else {
        QString siteName;
        if (mDomain.startsWith(QLatin1Char('.'))) {
            siteName = mDomain.mid(1);
        } else {
            siteName = mDomain;
        }
        setText(0, tolerantFromAce(siteName.toLatin1()));
    }
}

CookieProp *CookieListViewItem::leaveCookie()
{
    CookieProp *ret = mCookie;
    mCookie = nullptr;
    return ret;
}

KCookiesManagement::KCookiesManagement(QObject *parent, const KPluginMetaData &md, const QVariantList &)
    : KCModule(parent, md)
    , mDeleteAllFlag(false)
    , mMainWidget(qobject_cast<QWidget*>(parent))
{
    mUi.setupUi(widget());
    mUi.searchLineEdit->setTreeWidget(mUi.cookiesTreeWidget);
    mUi.cookiesTreeWidget->setColumnWidth(0, 150);

    connect(mUi.deleteButton, &QAbstractButton::clicked, this, &KCookiesManagement::deleteCurrent);
    connect(mUi.deleteAllButton, &QAbstractButton::clicked, this, &KCookiesManagement::deleteAll);
    connect(mUi.reloadButton, &QAbstractButton::clicked, this, &KCookiesManagement::reload);
    connect(mUi.cookiesTreeWidget, &QTreeWidget::itemExpanded, this, &KCookiesManagement::listCookiesForDomain);
    connect(mUi.cookiesTreeWidget, &QTreeWidget::currentItemChanged, this, &KCookiesManagement::updateForItem);
    connect(mUi.cookiesTreeWidget, &QTreeWidget::itemDoubleClicked, this, &KCookiesManagement::showConfigPolicyDialog);
    connect(mUi.configPolicyButton, &QAbstractButton::clicked, this, &KCookiesManagement::showConfigPolicyDialog);
}

KCookiesManagement::~KCookiesManagement()
{
}

void KCookiesManagement::load()
{
    defaults();
}

void KCookiesManagement::save()
{
    Browser *browser = Browser::browser(qApp);
    CookieJar *jar = browser ? browser->cookieJar() : nullptr;

    // If delete all cookies was requested!
    if (mDeleteAllFlag) {
        if (jar) {
            jar->removeAllCookies();
        }
        mDeleteAllFlag = false; // deleted[Cookies|Domains] have been cleared yet
    }

    if (!mDeletedDomains.isEmpty()) {
        if (jar) {
            for (const QString &d :mDeletedDomains) {
                jar->removeCookiesWithDomain(d);
            }
        }
        mDeletedDomains.clear();
    }

    // Individual cookies were deleted...
    if (jar) {
        for (auto it = mDeletedCookies.constBegin(); it != mDeletedCookies.constEnd(); ++it) {
            CookiePropList list = it.value();
            QVector<QNetworkCookie> toDelete;
            std::transform(list.constBegin(), list.constEnd(), std::back_inserter(toDelete), [](CookieProp* p){return p->cookie;});
            jar->removeCookies(toDelete);
        }
    }
    mDeletedCookies.clear();
    setNeedsSave(false);
}

void KCookiesManagement::defaults()
{
    reset();
    reload();
}

void KCookiesManagement::reset(bool deleteAll)
{
    if (!deleteAll) {
        mDeleteAllFlag = false;
    }

    clearCookieDetails();
    mDeletedDomains.clear();
    mDeletedCookies.clear();

    mUi.cookiesTreeWidget->clear();
    mUi.deleteButton->setEnabled(false);
    mUi.deleteAllButton->setEnabled(false);
    mUi.configPolicyButton->setEnabled(false);
}

void KCookiesManagement::clearCookieDetails()
{
    mUi.nameLineEdit->clear();
    mUi.valueLineEdit->clear();
    mUi.domainLineEdit->clear();
    mUi.pathLineEdit->clear();
    mUi.expiresLineEdit->clear();
    mUi.secureLineEdit->clear();
}

QSet<QNetworkCookie> KCookiesManagement::getCookies()
{
    Browser *browser = Browser::browser(qApp);
    CookieJar *jar = browser ? browser->cookieJar() : nullptr;
    if (!jar) {
        return {};
    }
    return jar->cookies();
}

void KCookiesManagement::reload()
{

    if (mUi.cookiesTreeWidget->topLevelItemCount() > 0) {
        reset();
    }

    CookieSet cookies = getCookies();
    QStringList domains;
    std::transform(cookies.constBegin(), cookies.constEnd(), std::back_inserter(domains), [](const QNetworkCookie &c){return c.domain();});

    CookieListViewItem *dom;
    for (const QString &domain : domains) {
        const QString siteName = (domain.startsWith(QLatin1Char('.')) ? domain.mid(1) : domain);
        if (mUi.cookiesTreeWidget->findItems(siteName, Qt::MatchFixedString).isEmpty()) {
            dom = new CookieListViewItem(mUi.cookiesTreeWidget, domain);
            dom->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        }
    }

    // are there any cookies?
    mUi.deleteAllButton->setEnabled(mUi.cookiesTreeWidget->topLevelItemCount() > 0);
    mUi.cookiesTreeWidget->sortItems(0, Qt::AscendingOrder);
    setNeedsSave(false);
}

Q_DECLARE_METATYPE(QList<int>)

void KCookiesManagement::listCookiesForDomain(QTreeWidgetItem *item)
{
    CookieListViewItem *cookieDom = static_cast<CookieListViewItem *>(item);
    if (!cookieDom || cookieDom->cookiesLoaded()) {
        return;
    }

    QStringList cookiesString;
    // Always check for cookies in both "foo.bar" and ".foo.bar" domains...
    const QStringList domains{cookieDom->domain(), QLatin1String(".") + cookieDom->domain()};
    CookieSet cookies = getCookies();
    QList<QNetworkCookie> filteredCookies;
    auto filter = [domains](const QNetworkCookie &c){return domains.contains(c.domain());};
    std::copy_if(cookies.constBegin(), cookies.constEnd(), std::back_inserter(filteredCookies), filter);
    for (const QNetworkCookie &c : filteredCookies) {
        CookieProp *details = new CookieProp;
        details->cookie.setDomain(c.domain());
        details->cookie.setPath(c.path());
        details->cookie.setName(c.name());
        details->host = c.domain();
        if (details->host.startsWith('.')) {
            details->host.remove(0, 1);
        }
        details->allLoaded = false;
        new CookieListViewItem(item, details);
    }

    if (!filteredCookies.isEmpty()) {
        static_cast<CookieListViewItem *>(item)->setCookiesLoaded();
        mUi.searchLineEdit->updateSearch();
    }
}

bool KCookiesManagement::cookieDetails(CookieProp *cookie)
{
    CookieSet cookies = getCookies();
    const QStringList domains{cookie->cookie.domain(), QLatin1String(".") + cookie->cookie.domain()};
    auto filter = [cookie, domains](const QNetworkCookie &c){
        return domains.contains(c.domain()) && c.path() == cookie->cookie.path() && c.name() == cookie->cookie.name();
    };
    auto it = std::find_if(cookies.constBegin(), cookies.constEnd(), filter);
    if (it == cookies.constEnd()) {
        return false;
    }
    cookie->cookie = *it;
    cookie->allLoaded = true;
    return true;
}

void KCookiesManagement::updateForItem(QTreeWidgetItem *item)
{
    if (item) {
        CookieListViewItem *cookieItem = static_cast<CookieListViewItem *>(item);
        CookieProp *cookie = cookieItem->cookie();

        if (cookie) {
            if (cookie->allLoaded || cookieDetails(cookie)) {
                mUi.nameLineEdit->setText(cookie->cookie.name());
                mUi.valueLineEdit->setText(cookie->cookie.value());
                mUi.domainLineEdit->setText(cookie->cookie.domain());
                mUi.pathLineEdit->setText(cookie->cookie.path());
                mUi.expiresLineEdit->setText(cookie->expireDate());
                mUi.secureLineEdit->setText(cookie->secure());
            }

            mUi.configPolicyButton->setEnabled(false);
        } else {
            clearCookieDetails();
            mUi.configPolicyButton->setEnabled(true);
        }
    } else {
        mUi.configPolicyButton->setEnabled(false);
    }
    mUi.deleteButton->setEnabled(item != nullptr);
}

void KCookiesManagement::showConfigPolicyDialog()
{
    // Get current item
    CookieListViewItem *item = static_cast<CookieListViewItem *>(mUi.cookiesTreeWidget->currentItem());
    Q_ASSERT(item); // the button is disabled otherwise

    if (item) {
        KCookiesMain *mainDlg = qobject_cast<KCookiesMain *>(mMainWidget);
        // must be present or something is really wrong.
        Q_ASSERT(mainDlg);

        KCookiesPolicies *policyDlg = mainDlg->policyDlg();
        // must be present unless someone rewrote the widget in which case
        // this needs to be re-written as well.
        Q_ASSERT(policyDlg);

        policyDlg->setPolicy(item->domain());
    }
}

void KCookiesManagement::deleteCurrent()
{
    QTreeWidgetItem *currentItem = mUi.cookiesTreeWidget->currentItem();
    Q_ASSERT(currentItem); // the button is disabled otherwise
    CookieListViewItem *item = static_cast<CookieListViewItem *>(currentItem);
    if (item->cookie()) {
        CookieListViewItem *parent = static_cast<CookieListViewItem *>(item->parent());
        CookiePropList list = mDeletedCookies.value(parent->domain());
        list.append(item->leaveCookie());
        mDeletedCookies.insert(parent->domain(), list);
        delete item;
        if (parent->childCount() == 0) {
            delete parent;
        }
    } else {
        mDeletedDomains.append(item->domain());
        delete item;
    }

    currentItem = mUi.cookiesTreeWidget->currentItem();
    if (currentItem) {
        mUi.cookiesTreeWidget->setCurrentItem(currentItem);
    } else {
        clearCookieDetails();
    }

    mUi.deleteAllButton->setEnabled(mUi.cookiesTreeWidget->topLevelItemCount() > 0);

    setNeedsSave(true);
}

void KCookiesManagement::deleteAll()
{
    mDeleteAllFlag = true;
    reset(true);
    setNeedsSave(true);
}
