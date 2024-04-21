/**
    kcookiespolicies.cpp - Cookies configuration

    Original Authors
    SPDX-FileCopyrightText: Waldo Bastian <bastian@kde.org>
    SPDX-FileCopyrightText: 1999 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2008 Urs Wolfer <uwolfer @ kde.org>

    Re-written by:
    SPDX-FileCopyrightText: 2000 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

// Own
#include "kcookiespolicies.h"

// Qt
#include <QCheckBox>
#include <QDBusInterface>
#include <QDBusReply>
#include <QPushButton>
#include <QJsonDocument>
#include <QJsonObject>

// KDE
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <QUrl>
#include <KSharedConfig>

using namespace KonqInterfaces;


// QUrl::fromAce/toAce don't accept a domain that starts with a '.', like we do here.
// So we use these wrappers.
QString tolerantFromAce(const QByteArray &_domain)
{
    QByteArray domain(_domain);
    const bool hasDot = domain.startsWith('.');
    if (hasDot) {
        domain.remove(0, 1);
    }
    QString ret = QUrl::fromAce(domain);
    if (hasDot) {
        ret.prepend(QLatin1Char('.'));
    }
    return ret;
}

static QByteArray tolerantToAce(const QString &_domain)
{
    QString domain(_domain);
    const bool hasDot = domain.startsWith(QLatin1Char('.'));
    if (hasDot) {
        domain.remove(0, 1);
    }
    QByteArray ret = QUrl::toAce(domain);
    if (hasDot) {
        ret.prepend('.');
    }
    return ret;
}

KCookiesPolicies::KCookiesPolicies(QObject *parent, const KPluginMetaData &md, const QVariantList &)
    : KCModule(parent, md)
    , mSelectedItemsCount(0)
{
    mUi.setupUi(widget());
    mUi.kListViewSearchLine->setTreeWidget(mUi.policyTreeWidget);
    QList<int> columns;
    columns.append(0);
    mUi.kListViewSearchLine->setSearchColumns(columns);

    mUi.pbNew->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    mUi.pbChange->setIcon(QIcon::fromTheme(QStringLiteral("edit-rename")));
    mUi.pbDelete->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));
    mUi.pbDeleteAll->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));

    // Connect the main switch :) Enable/disable cookie support
    connect(mUi.cbEnableCookies, &QAbstractButton::toggled, this, &KCookiesPolicies::cookiesEnabled);
    connect(mUi.cbEnableCookies, &QAbstractButton::toggled, this, &KCookiesPolicies::configChanged);

    // Connect the preference check boxes...
    connect(mUi.cbRejectCrossDomainCookies, &QAbstractButton::toggled, this, &KCookiesPolicies::configChanged);
    connect(mUi.cbAutoAcceptSessionCookies, &QAbstractButton::toggled, this, &KCookiesPolicies::configChanged);

    connect(mUi.rbPolicyAsk, &QAbstractButton::toggled, this, &KCookiesPolicies::configChanged);
    connect(mUi.rbPolicyAccept, &QAbstractButton::toggled, this, &KCookiesPolicies::configChanged);
    connect(mUi.rbPolicyAcceptForSession, &QAbstractButton::toggled, this, &KCookiesPolicies::configChanged);
    connect(mUi.rbPolicyReject, &QAbstractButton::toggled, this, &KCookiesPolicies::configChanged);
    // Connect signals from the domain specific policy listview.
    connect(mUi.policyTreeWidget, &QTreeWidget::itemSelectionChanged, this, &KCookiesPolicies::selectionChanged);
    connect(mUi.policyTreeWidget, &QTreeWidget::itemDoubleClicked, this, QOverload<>::of(&KCookiesPolicies::changePressed));

    // Connect the buttons...
    connect(mUi.pbNew, &QAbstractButton::clicked, this, QOverload<>::of(&KCookiesPolicies::addPressed));
    connect(mUi.pbChange, &QAbstractButton::clicked, this, QOverload<>::of(&KCookiesPolicies::changePressed));
    connect(mUi.pbDelete, &QAbstractButton::clicked, this, &KCookiesPolicies::deletePressed);
    connect(mUi.pbDeleteAll, &QAbstractButton::clicked, this, &KCookiesPolicies::deleteAllPressed);
}

KCookiesPolicies::~KCookiesPolicies()
{
}

void KCookiesPolicies::configChanged()
{
    // kDebug() << "KCookiesPolicies::configChanged...";
    setNeedsSave(true);
}

void KCookiesPolicies::cookiesEnabled(bool enable)
{
    mUi.bgDefault->setEnabled(enable);
    mUi.bgPreferences->setEnabled(enable);
    mUi.gbDomainSpecific->setEnabled(enable);
}

void KCookiesPolicies::setPolicy(const QString &domain)
{
    QTreeWidgetItemIterator it(mUi.policyTreeWidget);
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

void KCookiesPolicies::changePressed(QTreeWidgetItem *item, bool state)
{
    Q_ASSERT(item);
    const QString oldDomain(item->text(0));

    KCookiesPolicySelectionDlg pdlg(widget());
    pdlg.setWindowTitle(i18nc("@title:window", "Change Cookie Policy"));
    pdlg.setPolicy(mDomainPolicyMap.value(oldDomain));
    pdlg.setEnableHostEdit(state, oldDomain);

    if (pdlg.exec() && !pdlg.domain().isEmpty()) {
        const QString newDomain = tolerantFromAce(pdlg.domain().toLatin1());
        CookieJar::CookieAdvice advice = pdlg.advice();
        if (newDomain == oldDomain || !handleDuplicate(newDomain, advice)) {
            mDomainPolicyMap[newDomain] = advice;
            item->setText(0, newDomain);
            item->setText(1, i18n(KCookieAdvice::adviceToStr(mDomainPolicyMap.value(newDomain))));
            configChanged();
        }
    }
}

void KCookiesPolicies::addPressed(const QString &domain, bool state)
{
    KCookiesPolicySelectionDlg pdlg(widget());
    pdlg.setWindowTitle(i18nc("@title:window", "New Cookie Policy"));
    pdlg.setEnableHostEdit(state, domain);

    if (mUi.rbPolicyAccept->isChecked()) {
        pdlg.setPolicy(CookieJar::CookieAdvice::Reject);
    } else {
        pdlg.setPolicy(CookieJar::CookieAdvice::Accept);
    }

    if (pdlg.exec() && !pdlg.domain().isEmpty()) {
        const QString domain = tolerantFromAce(pdlg.domain().toLatin1());
        CookieJar::CookieAdvice advice = pdlg.advice();

        if (!handleDuplicate(domain, advice)) {
            const char *strAdvice = KCookieAdvice::adviceToStr(advice);
            const QStringList items{
                domain,
                i18n(strAdvice),
            };
            QTreeWidgetItem *item = new QTreeWidgetItem(mUi.policyTreeWidget, items);
            mDomainPolicyMap.insert(item->text(0), advice);
            configChanged();
            updateButtons();
        }
    }
}

bool KCookiesPolicies::handleDuplicate(const QString &domain, CookieJar::CookieAdvice advice)
{
    QTreeWidgetItem *item = mUi.policyTreeWidget->topLevelItem(0);
    while (item != nullptr) {
        if (item->text(0) == domain) {
            const int res = KMessageBox::warningContinueCancel(widget(),
                                                               i18n("<qt>A policy already exists for"
                                                                    "<center><b>%1</b></center>"
                                                                    "Do you want to replace it?</qt>",
                                                                    domain),
                                                               i18nc("@title:window", "Duplicate Policy"),
                                                               KGuiItem(i18n("Replace")));
            if (res == KMessageBox::Continue) {
                mDomainPolicyMap[domain] = advice;
                item->setText(0, domain);
                item->setText(1, i18n(KCookieAdvice::adviceToStr(mDomainPolicyMap.value(domain))));
                configChanged();
                return true;
            } else {
                return true; // User Cancelled!!
            }
        }
        item = mUi.policyTreeWidget->itemBelow(item);
    }
    return false;
}

void KCookiesPolicies::deletePressed()
{
    QTreeWidgetItem *nextItem = nullptr;

    const QList<QTreeWidgetItem *> selectedItems = mUi.policyTreeWidget->selectedItems();
    for (const QTreeWidgetItem *item : selectedItems) {
        nextItem = mUi.policyTreeWidget->itemBelow(item);
        if (!nextItem) {
            nextItem = mUi.policyTreeWidget->itemAbove(item);
        }

        mDomainPolicyMap.remove(item->text(0));
        delete item;
    }

    if (nextItem) {
        nextItem->setSelected(true);
    }

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

    mUi.pbChange->setEnabled((hasItems && mSelectedItemsCount == 1));
    mUi.pbDelete->setEnabled((hasItems && mSelectedItemsCount > 0));
    mUi.pbDeleteAll->setEnabled(hasItems);
}

void KCookiesPolicies::updateDomainList(const QStringList &domainConfig)
{
    mUi.policyTreeWidget->clear();

    QStringList::ConstIterator it = domainConfig.begin();
    for (; it != domainConfig.end(); ++it) {
        QString domain;
        CookieJar::CookieAdvice advice = CookieJar::CookieAdvice::Unknown;
        splitDomainAdvice(*it, domain, advice);
        if (!domain.isEmpty()) {
            const QStringList items{
                tolerantFromAce(domain.toLatin1()),
                i18n(KCookieAdvice::adviceToStr(advice)),
            };
            QTreeWidgetItem *item = new QTreeWidgetItem(mUi.policyTreeWidget, items);
            mDomainPolicyMap[item->text(0)] = advice;
        }
    }

    mUi.policyTreeWidget->sortItems(0, Qt::AscendingOrder);
}

void KCookiesPolicies::selectionChanged()
{
    mSelectedItemsCount = mUi.policyTreeWidget->selectedItems().count();
    updateButtons();
}

void KCookiesPolicies::load()
{
    mSelectedItemsCount = 0;

    KSharedConfig::Ptr cfg = KSharedConfig::openConfig();
    KConfigGroup group = cfg->group("Cookie Policy");

    bool enableCookies = group.readEntry("Cookies", true);
    mUi.cbEnableCookies->setChecked(enableCookies);
    cookiesEnabled(enableCookies);

    CookieJar::CookieAdvice advice = CookieJar::readAdviceConfigEntry(group, "CookieGlobalAdvice");
    switch (advice) {
    case CookieJar::CookieAdvice::Accept:
        mUi.rbPolicyAccept->setChecked(true);
        break;
    case CookieJar::CookieAdvice::AcceptForSession:
        mUi.rbPolicyAcceptForSession->setChecked(true);
        break;
    case CookieJar::CookieAdvice::Reject:
        mUi.rbPolicyReject->setChecked(true);
        break;
    case CookieJar::CookieAdvice::Ask:
    case CookieJar::CookieAdvice::Unknown:
    default:
        mUi.rbPolicyAsk->setChecked(true);
    }

    bool enable = group.readEntry("RejectCrossDomainCookies", true);
    mUi.cbRejectCrossDomainCookies->setChecked(enable);

    bool sessionCookies = group.readEntry("AcceptSessionCookies", true);
    mUi.cbAutoAcceptSessionCookies->setChecked(sessionCookies);
    updateDomainList(group.readEntry("CookieDomainAdvice", QStringList()));

    if (enableCookies) {
        updateButtons();
    }
    KCModule::load();
}

void KCookiesPolicies::save()
{
    KSharedConfig::Ptr cfg = KSharedConfig::openConfig();
    KConfigGroup group = cfg->group("Cookie Policy");

    bool state = mUi.cbEnableCookies->isChecked();
    group.writeEntry("Cookies", state);
    state = mUi.cbRejectCrossDomainCookies->isChecked();
    group.writeEntry("RejectCrossDomainCookies", state);
    state = mUi.cbAutoAcceptSessionCookies->isChecked();
    group.writeEntry("AcceptSessionCookies", state);

    CookieJar::CookieAdvice advice;
    if (mUi.rbPolicyAccept->isChecked()) {
        advice = CookieJar::CookieAdvice::Accept;
    } else if (mUi.rbPolicyAcceptForSession->isChecked()) {
        advice = CookieJar::CookieAdvice::AcceptForSession;
    } else if (mUi.rbPolicyReject->isChecked()) {
        advice = CookieJar::CookieAdvice::Reject;
    } else {
        advice = CookieJar::CookieAdvice::Ask;
    }

    CookieJar::writeAdviceConfigEntry(group, "CookieGlobalAdvice", advice);

    QStringList domainConfig;
    QJsonObject obj;
    for (auto it = mDomainPolicyMap.constBegin(); it != mDomainPolicyMap.constEnd(); ++it) {
        obj.insert(it.key(), CookieJar::adviceToInt(it.value()));
    }
    group.writeEntry("CookieDomainAdvice", QJsonDocument(obj).toJson());

    group.sync();

    QDBusMessage message =
        QDBusMessage::createSignal(QStringLiteral("/KonqMain"), QStringLiteral("org.kde.Konqueror.Main"), QStringLiteral("reparseConfiguration"));
    QDBusConnection::sessionBus().send(message);
    setNeedsSave(false);
}

void KCookiesPolicies::defaults()
{
    mUi.cbEnableCookies->setChecked(true);
    mUi.rbPolicyAsk->setChecked(true);
    mUi.rbPolicyAccept->setChecked(false);
    mUi.rbPolicyAcceptForSession->setChecked(false);
    mUi.rbPolicyReject->setChecked(false);
    mUi.cbRejectCrossDomainCookies->setChecked(true);
    mUi.cbAutoAcceptSessionCookies->setChecked(false);
    mUi.policyTreeWidget->clear();
    mDomainPolicyMap.clear();

    cookiesEnabled(mUi.cbEnableCookies->isChecked());
    updateButtons();
}

void KCookiesPolicies::splitDomainAdvice(const QString &cfg, QString &domain, CookieJar::CookieAdvice &advice)
{
    int sepPos = cfg.lastIndexOf(QLatin1Char(':'));

    // Ignore any policy that does not contain a domain...
    if (sepPos <= 0) {
        return;
    }

    domain = cfg.left(sepPos);
    advice = KCookieAdvice::strToAdvice(cfg.mid(sepPos + 1));
}
