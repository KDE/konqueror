/**
 * kcookiesmanagement.cpp - Cookies manager
 *
 * Copyright 2000-2001 Marco Pinelli <pinmc@orion.it>
 * Copyright 2000-2001 Dawit Alemayehu <adawit@kde.org>
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
#include "kcookiesmanagement.h"

// Qt
#include <QtCore/QTimer>
#include <QToolButton>
#include <QBoxLayout>
#include <QtCore/QList>
#include <QApplication>
#include <QLayout>
#include <QPushButton>
#include <QLabel>
#include <QtDBus/QtDBus>

// KDE
#include <kdebug.h>
#include <klocale.h>
#include <kdialog.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <kmessagebox.h>
#include <kurl.h>
#include <kdatetime.h>

// Local
#include "kcookiesmain.h"
#include "kcookiespolicies.h"

QString tolerantFromAce(const QByteArray& _domain);

struct CookieProp
{
    QString host;
    QString name;
    QString value;
    QString domain;
    QString path;
    QString expireDate;
    QString secure;
    bool allLoaded;
};

CookieListViewItem::CookieListViewItem(QTreeWidget *parent, const QString &dom)
                   :QTreeWidgetItem(parent)
{
    init( 0, dom );
}

CookieListViewItem::CookieListViewItem(QTreeWidgetItem *parent, CookieProp *cookie)
                   :QTreeWidgetItem(parent)
{
    init( cookie );
}

CookieListViewItem::~CookieListViewItem()
{
    delete mCookie;
}

void CookieListViewItem::init( CookieProp* cookie, const QString &domain,
                               bool cookieLoaded )
{
    mCookie = cookie;
    mDomain = domain;
    mCookiesLoaded = cookieLoaded;

    if (mCookie)
    {
        if (mDomain.isEmpty())
            setText(0, tolerantFromAce(mCookie->host.toLatin1()));
        else
            setText(0, tolerantFromAce(mDomain.toLatin1()));
        setText(1, mCookie->name);
    }
    else
    {
        QString siteName;
        if (mDomain.startsWith(QLatin1Char('.')))
            siteName = mDomain.mid(1);
        else
            siteName = mDomain;
        setText(0, tolerantFromAce(siteName.toLatin1()));
    }
}

CookieProp* CookieListViewItem::leaveCookie()
{
    CookieProp *ret = mCookie;
    mCookie = 0;
    return ret;
}

KCookiesManagement::KCookiesManagement(const KComponentData &componentData, QWidget *parent)
                   : KCModule(componentData, parent),
                     mDeleteAllFlag(false),
                     mMainWidget(parent)
{
  mUi.setupUi(this);
  mUi.searchLineEdit->setTreeWidget(mUi.cookiesTreeWidget);
  mUi.cookiesTreeWidget->setColumnWidth(0, 150);
  connect(mUi.cookiesTreeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), SLOT(on_configPolicyButton_clicked()));
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
  // If delete all cookies was requested!
  if(mDeleteAllFlag)
  {
    QDBusInterface kded("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer", QDBusConnection::sessionBus());
    QDBusReply<void> reply = kded.call( "deleteAllCookies" );
    if (!reply.isValid())
    {
      QString caption = i18n ("D-Bus Communication Error");
      QString message = i18n ("Unable to delete all the cookies as requested.");
      KMessageBox::sorry (this, message, caption);
      return;
    }
    mDeleteAllFlag = false; // deleted[Cookies|Domains] have been cleared yet
  }

  // Certain groups of cookies were deleted...
  QMutableStringListIterator it (mDeletedDomains);
  while (it.hasNext())
  {    
    QDBusInterface kded("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer", QDBusConnection::sessionBus());
    QDBusReply<void> reply = kded.call( "deleteCookiesFromDomain",( it.next() ) );
    if (!reply.isValid())
    {
      QString caption = i18n ("D-Bus Communication Error");
      QString message = i18n ("Unable to delete cookies as requested.");
      KMessageBox::sorry (this, message, caption);
      return;
    }
    it.remove();
  }

  // Individual cookies were deleted...
  bool success = true; // Maybe we can go on...
  QMutableHashIterator<QString, CookiePropList> cookiesDom(mDeletedCookies);
  while(cookiesDom.hasNext())
  {
    cookiesDom.next();
    CookiePropList list = cookiesDom.value();
    foreach(CookieProp *cookie, list)
    {
      QDBusInterface kded("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer", QDBusConnection::sessionBus());
      QDBusReply<void> reply = kded.call( "deleteCookie", cookie->domain,
                                          cookie->host, cookie->path,
                                          cookie->name );
      if (!reply.isValid())
      {
        success = false;
        break;
      }

      list.removeOne(cookie);
    }

    if (!success)
      break;

    mDeletedCookies.remove(cookiesDom.key());
  }

  emit changed( false );
}

void KCookiesManagement::defaults()
{
  reset();
  on_reloadButton_clicked();
}

void KCookiesManagement::reset(bool deleteAll)
{
  if (!deleteAll)
    mDeleteAllFlag = false;

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

QString KCookiesManagement::quickHelp() const
{
  return i18n("<h1>Cookie Management Quick Help</h1>" );
}

void KCookiesManagement::on_reloadButton_clicked()
{
  QDBusInterface kded("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer", QDBusConnection::sessionBus());
  QDBusReply<QStringList> reply = kded.call( "findDomains" );

  if (!reply.isValid())
  {
    QString caption = i18n ("Information Lookup Failure");
    QString message = i18n ("Unable to retrieve information about the "
                            "cookies stored on your computer.");
    KMessageBox::sorry (this, message, caption);
    return;
  }

  if (mUi.cookiesTreeWidget->topLevelItemCount() > 0)
      reset();

  CookieListViewItem *dom;
  const QStringList domains (reply.value());
  Q_FOREACH(const QString& domain, domains)
  {
    const QString siteName = (domain.startsWith(QLatin1Char('.')) ? domain.mid(1) : domain);
    if (mUi.cookiesTreeWidget->findItems(siteName, Qt::MatchFixedString).isEmpty()) {
        dom = new CookieListViewItem(mUi.cookiesTreeWidget, domain);
        dom->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }
  }

  // are there any cookies?
  mUi.deleteAllButton->setEnabled(mUi.cookiesTreeWidget->topLevelItemCount() > 0);
  mUi.cookiesTreeWidget->sortItems(0, Qt::AscendingOrder);
  emit changed(false);
}

Q_DECLARE_METATYPE( QList<int> )

void KCookiesManagement::on_cookiesTreeWidget_itemExpanded(QTreeWidgetItem *item)
{
  CookieListViewItem* cookieDom = static_cast<CookieListViewItem*>(item);
  if (!cookieDom || cookieDom->cookiesLoaded())
    return;

  QStringList cookies;
  QList<int> fields;
  fields << 0 << 1 << 2 << 3;  
  // Always check for cookies in both "foo.bar" and ".foo.bar" domains...
  const QString domain = cookieDom->domain() + QLatin1String(" .") + cookieDom->domain();
  QDBusInterface kded("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer", QDBusConnection::sessionBus());  
  QDBusReply<QStringList> reply = kded.call("findCookies", QVariant::fromValue(fields),
                                            domain, QString(), QString(), QString());
  if (reply.isValid())
      cookies.append(reply.value());

  QStringListIterator it(cookies);
  while (it.hasNext())
  {
    CookieProp *details = new CookieProp;
    details->domain = it.next();
    details->path = it.next();
    details->name = it.next();
    details->host = it.next();
    details->allLoaded = false;
    new CookieListViewItem(item, details);
  }

  if (!cookies.isEmpty())
  {
    static_cast<CookieListViewItem*>(item)->setCookiesLoaded();
    mUi.searchLineEdit->updateSearch();
  }
}

bool KCookiesManagement::cookieDetails(CookieProp *cookie)
{
  QList<int> fields;
  fields << 4 << 5 << 7;

  QDBusInterface kded("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer", QDBusConnection::sessionBus());
  QDBusReply<QStringList> reply = kded.call( "findCookies",
                                             QVariant::fromValue( fields ),
                                             cookie->domain,
                                             cookie->host,
                                             cookie->path,
                                             cookie->name);
  if (!reply.isValid())
    return false;

  const QStringList fieldVal = reply.value();

  QStringList::const_iterator c = fieldVal.begin();
  if (c == fieldVal.end()) // empty list, do not crash
    return false;

  bool ok;  
  cookie->value = *c++;
  qint64 tmp = (*c++).toLongLong(&ok);

  if (!ok || tmp == 0)
    cookie->expireDate = i18n("End of session");
  else
  {
    KDateTime expDate;
    expDate.setTime_t(tmp);
    cookie->expireDate = KGlobal::locale()->formatDateTime(expDate);
  }

  tmp = (*c).toUInt(&ok);
  cookie->secure = i18n((ok && tmp) ? "Yes" : "No");
  cookie->allLoaded = true;
  return true;
}

void KCookiesManagement::on_cookiesTreeWidget_currentItemChanged(QTreeWidgetItem* item)
{
  Q_ASSERT(item);
  CookieListViewItem* cookieItem = static_cast<CookieListViewItem*>(item);
  CookieProp *cookie = (cookieItem ? cookieItem->cookie() : 0);

  if (cookie)
  {
    if (cookie->allLoaded || cookieDetails(cookie))
    {
      mUi.nameLineEdit->setText(cookie->name);
      mUi.valueLineEdit->setText(cookie->value);
      mUi.domainLineEdit->setText(cookie->domain);
      mUi.pathLineEdit->setText(cookie->path);
      mUi.expiresLineEdit->setText(cookie->expireDate);
      mUi.secureLineEdit->setText(cookie->secure);
    }

    mUi.configPolicyButton->setEnabled(false);
  }
  else
  {
    clearCookieDetails();
    mUi.configPolicyButton->setEnabled(true);
  }

  mUi.deleteButton->setEnabled(true);
}

void KCookiesManagement::on_configPolicyButton_clicked()
{
  // Get current item
  CookieListViewItem *item = static_cast<CookieListViewItem*>(mUi.cookiesTreeWidget->currentItem());

  if (item)
  {
    KCookiesMain* mainDlg = qobject_cast<KCookiesMain*>(mMainWidget);
    // must be present or something is really wrong.
    Q_ASSERT(mainDlg);

    KCookiesPolicies* policyDlg = mainDlg->policyDlg();
    // must be present unless someone rewrote the widget in which case
    // this needs to be re-written as well.
    Q_ASSERT(policyDlg);
    
    policyDlg->setPolicy(item->domain());
  }
}

void KCookiesManagement::on_deleteButton_clicked()
{
  QTreeWidgetItem* currentItem = mUi.cookiesTreeWidget->currentItem();
  CookieListViewItem *item = static_cast<CookieListViewItem*>( currentItem );
  if (item && item->cookie())
  {
    CookieListViewItem *parent = static_cast<CookieListViewItem*>(item->parent());
    CookiePropList list = mDeletedCookies.value(parent->domain());
    list.append(item->leaveCookie());
    mDeletedCookies.insert(parent->domain(), list);
    delete item;
    if (parent->childCount() == 0)
      delete parent;
  }
  else
  {
    mDeletedDomains.append(item->domain());
    delete item;
  }

  currentItem = mUi.cookiesTreeWidget->currentItem();
  if (currentItem)
  {
    mUi.cookiesTreeWidget->setCurrentItem( currentItem );
    //on_cookiesTreeWidget_currentItemChanged( currentItem );
  }
  else
    clearCookieDetails();

  mUi.deleteButton->setEnabled(mUi.cookiesTreeWidget->currentItem());
  mUi.deleteAllButton->setEnabled(mUi.cookiesTreeWidget->topLevelItemCount() > 0);

  emit changed( true );
}

void KCookiesManagement::on_deleteAllButton_clicked()
{
  mDeleteAllFlag = true;
  reset(true);
  emit changed(true);
}

#include "kcookiesmanagement.moc"
