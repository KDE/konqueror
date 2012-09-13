/*
   Copyright (C) 2000-2011 Dawit Alemayehu <adawit@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "dirfilterplugin.h"

#include <QLabel>
#include <QSpacerItem>
#include <QToolButton>
#include <QPushButton>
#include <QHBoxLayout>
#include <QBoxLayout>

#include <kicon.h>
#include <kmenu.h>
#include <kaction.h>
#include <klocale.h>
#include <kfileitem.h>
#include <kmimetype.h>
#include <klineedit.h>
#include <kactionmenu.h>
#include <kconfiggroup.h>
#include <kcomponentdata.h>
#include <kpluginfactory.h>
#include <kactioncollection.h>

#include <kparts/browserextension.h>


K_GLOBAL_STATIC(SessionManager, globalSessionManager)

static void generateKey(const KUrl& url, QString* key)
{
    if (url.isValid()) {
        *key = url.protocol();
        *key += QLatin1Char(':');

        if (url.hasHost()) {
            *key += url.host();
            *key += QLatin1Char(':');
        }

        if (url.hasPath()) {
            *key += url.path();
        }
    }
}

static void saveNameFilter(const KUrl& url, const QString& filter)
{
    SessionManager::Filters f = globalSessionManager->restore(url);
    f.nameFilter = filter;
    globalSessionManager->save(url, f);
}

static void saveTypeFilters(const KUrl& url, const QStringList& filters)
{
    SessionManager::Filters f = globalSessionManager->restore(url);
    f.typeFilters = filters;
    globalSessionManager->save(url, f);
}

SessionManager::SessionManager()
{
    m_bSettingsLoaded = false;
    loadSettings();
}

SessionManager::~SessionManager()
{
    saveSettings();
}

SessionManager::Filters SessionManager::restore(const KUrl& url)
{
    QString key;
    generateKey(url, &key);
    return m_filters.value(key);
}

void SessionManager::save(const KUrl& url, const Filters& filters)
{
    QString key;
    generateKey(url, &key);
    m_filters[key] = filters;
}

void SessionManager::saveSettings()
{
    KConfig cfg("dirfilterrc", KConfig::NoGlobals);
    KConfigGroup group = cfg.group("General");

    group.writeEntry("ShowCount", showCount);
    group.writeEntry("UseMultipleFilters", useMultipleFilters);
    cfg.sync();
}

void SessionManager::loadSettings()
{
    if (m_bSettingsLoaded)
        return;

    KConfig cfg("dirfilterrc", KConfig::NoGlobals);
    KConfigGroup group = cfg.group("General");

    showCount = group.readEntry("ShowCount", false);
    useMultipleFilters = group.readEntry("UseMultipleFilters", true);
    m_bSettingsLoaded = true;
}


FilterBar::FilterBar(QWidget* parent)
    : QWidget(parent)
{
    // Create close button
    QToolButton* closeButton = new QToolButton(this);
    closeButton->setAutoRaise(true);
    closeButton->setIcon(KIcon("dialog-close"));
    closeButton->setToolTip(i18nc("@info:tooltip", "Hide Filter Bar"));
    connect(closeButton, SIGNAL(clicked()), this, SIGNAL(closeRequest()));

    // Create label
    QLabel* filterLabel = new QLabel(i18nc("@label:textbox", "F&ilter:"), this);

    // Create filter editor
    m_filterInput = new KLineEdit(this);
    m_filterInput->setLayoutDirection(Qt::LeftToRight);
    m_filterInput->setClearButtonShown(true);
    connect(m_filterInput, SIGNAL(textChanged(QString)),
            this, SIGNAL(filterChanged(QString)));
    setFocusProxy(m_filterInput);

    m_typeFilterButton = new QPushButton(this);
    m_typeFilterButton->setIcon(KIcon("view-filter"));
    m_typeFilterButton->setText(i18nc("@label:button", "Filter by t&ype"));
    m_typeFilterButton->setToolTip(i18nc("@info:tooltip", "Filter items by file type."));

    // Apply layout
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(closeButton);
    layout->addWidget(filterLabel);
    layout->addWidget(m_filterInput);
    layout->addWidget(m_typeFilterButton);
    layout->addItem(new QSpacerItem(20, 20, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum));

    filterLabel->setBuddy(m_filterInput);
}

FilterBar::~FilterBar()
{
}

void FilterBar::selectAll()
{
    m_filterInput->selectAll();
}

bool FilterBar::typeFilterMenuEnabled() const
{
    return m_typeFilterButton->isEnabled();
}

void FilterBar::setEnableTypeFilterMenu(bool state)
{
    m_typeFilterButton->setEnabled(state);
}

void FilterBar::setTypeFilterMenu(QMenu* menu)
{
    m_typeFilterButton->setMenu(menu);
}

QMenu* FilterBar::typeFilterMenu()
{
    return m_typeFilterButton->menu();
}

void FilterBar::setNameFilter(const QString& text)
{
    m_filterInput->setText(text);
}

void FilterBar::clear()
{
    m_filterInput->clear();
}

void FilterBar::showEvent(QShowEvent* event)
{
    if (!event->spontaneous()) {
        m_filterInput->setFocus();
    }
}

void FilterBar::keyReleaseEvent(QKeyEvent* event)
{
    QWidget::keyReleaseEvent(event);
    if (event->key() == Qt::Key_Escape) {
        if (m_filterInput->text().isEmpty()) {
            emit closeRequest();
        } else {
            m_filterInput->clear();
        }
    }
}


DirFilterPlugin::DirFilterPlugin(QObject* parent, const QVariantList&)
    : KParts::Plugin(parent)
    , m_filterBar(0)
    , m_focusWidget(0)
{
    m_part = qobject_cast<KParts::ReadOnlyPart*>(parent);
    if (m_part) {
        connect(m_part, SIGNAL(aboutToOpenURL()), this, SLOT(slotOpenURL()));
        //connect(m_part, SIGNAL(completed()), this, SLOT(slotOpenURLCompleted()));
        connect(m_part, SIGNAL(completed(bool)), this, SLOT(slotOpenURLCompleted()));
    }

    KParts::ListingNotificationExtension* notifyExt = KParts::ListingNotificationExtension::childObject(m_part);
    if (notifyExt && notifyExt->supportedNotificationEventTypes() != KParts::ListingNotificationExtension::None) {
        m_listingExt = KParts::ListingFilterExtension::childObject(m_part);
        connect(notifyExt, SIGNAL(listingEvent(KParts::ListingNotificationExtension::NotificationEventType, KFileItemList)),
                this, SLOT(slotListingEvent(KParts::ListingNotificationExtension::NotificationEventType, KFileItemList)));

        KAction* action = actionCollection()->addAction(QLatin1String("filterdir"), this, SLOT(slotShowFilterBar()));
        action->setText(i18nc("@action:inmenu Tools", "Show Filter Bar"));
        action->setIcon(KIcon("view-filter"));
        action->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_I);
    }
}

DirFilterPlugin::~DirFilterPlugin()
{
}

void DirFilterPlugin::slotOpenURL()
{
    if (m_part && !m_part->arguments().reload()) {
        m_pMimeInfo.clear();
        if (m_filterBar && m_filterBar->isVisible()) {
            m_filterBar->clear();
            m_filterBar->setEnableTypeFilterMenu(false);  // Will be enabled once loading has completed
        }
    }
}

void DirFilterPlugin::slotOpenURLCompleted()
{
    if (m_listingExt && m_part && m_filterBar && m_filterBar->isVisible()) {
        setFilterBar();
    }
}

void DirFilterPlugin::slotShowPopup()
{
    QMenu* filterMenu = (m_filterBar ? m_filterBar->typeFilterMenu() : 0);
    if (!filterMenu)
        return;

    filterMenu->clear();

    QString label;
    QStringList inodes;
    quint64 enableReset = 0;
    QMapIterator<QString, MimeInfo> it(m_pMimeInfo);

    while (it.hasNext()) {
        it.next();

        if (it.key().startsWith("inode")) {
            inodes << it.key();
            continue;
        }

        if (!globalSessionManager->showCount) {
            label = it.value().mimeComment;
        } else {
            label = it.value().mimeComment;
            label += "  (";
            label += QString::number(it.value().filenames.size());
            label += ')';
        }

        QAction* action = filterMenu->addAction(KIcon(it.value().iconName), label);
        action->setCheckable(true);
        if (it.value().useAsFilter) {
            action->setChecked(true);
            enableReset++;
        }
        action->setData(it.key());
        m_pMimeInfo[it.key()].action = action;
    }

    // Add all the items that have mime-type of "inode/*" here...
    if (!inodes.isEmpty()) {
        filterMenu->addSeparator();

        Q_FOREACH(const QString & inode, inodes) {
            if (!globalSessionManager->showCount)
                label = m_pMimeInfo[inode].mimeComment;
            else {
                label = m_pMimeInfo[inode].mimeComment;
                label += "  (";
                label += QString::number(m_pMimeInfo[inode].filenames.size());
                label += ')';
            }

            QAction* action = filterMenu->addAction(KIcon(m_pMimeInfo[inode].iconName), label);
            action->setCheckable(true);
            if (m_pMimeInfo[inode].useAsFilter) {
                action->setChecked(true);
                enableReset ++;
            }
            action->setData(it.key());
            m_pMimeInfo[inode].action = action;
        }
    }
    filterMenu->addSeparator();
    QAction* action = filterMenu->addAction(i18n("Use Multiple Filters"),
                                            this, SLOT(slotMultipleFilters()));
    action->setEnabled(enableReset <= 1);
    action->setCheckable(true);
    action->setChecked(globalSessionManager->useMultipleFilters);

    action = filterMenu->addAction(i18n("Show Count"), this,
                                   SLOT(slotShowCount()));
    action->setCheckable(true);
    action->setChecked(globalSessionManager->showCount);

    action = filterMenu->addAction(i18n("Reset"), this,
                                   SLOT(slotReset()));
    action->setEnabled(enableReset);
}

void DirFilterPlugin::slotItemSelected(QAction* action)
{
    if (!m_listingExt || !action || !m_part)
        return;

    MimeInfoMap::iterator it = m_pMimeInfo.find(action->data().toString());
    if (it != m_pMimeInfo.constEnd()) {
        MimeInfo& mimeInfo = it.value();
        QStringList filters;

        if (mimeInfo.useAsFilter) {
            mimeInfo.useAsFilter = false;
            filters = m_listingExt->filter(KParts::ListingFilterExtension::MimeType).toStringList();
            if (filters.removeAll(it.key()))
                m_listingExt->setFilter(KParts::ListingFilterExtension::MimeType, filters);
        } else {
            m_pMimeInfo[it.key()].useAsFilter = true;

            if (globalSessionManager->useMultipleFilters) {
                filters = m_listingExt->filter(KParts::ListingFilterExtension::MimeType).toStringList();
                filters << it.key();
            } else {
                filters << it.key();

                MimeInfoMap::iterator item = m_pMimeInfo.begin();
                MimeInfoMap::const_iterator itemEnd = m_pMimeInfo.end();
                while (item != itemEnd) {
                    if (item != it)
                        item.value().useAsFilter = false;
                    item++;
                }
            }
            m_listingExt->setFilter(KParts::ListingFilterExtension::MimeType, filters);
        }

        saveTypeFilters(m_part->url(), filters);
    }
}

void DirFilterPlugin::slotNameFilterChanged(const QString& filter)
{
    if (!m_listingExt || !m_part)
        return;

    m_listingExt->setFilter(KParts::ListingFilterExtension::SubString, filter);
    saveNameFilter(m_part->url(), filter);
}

void DirFilterPlugin::slotCloseRequest()
{
    if (m_filterBar) {
        m_filterBar->clear();
        m_filterBar->hide();
        if (m_focusWidget) {
            m_focusWidget->setFocus();
            m_focusWidget = 0;
        }
    }
}

void DirFilterPlugin::slotListingEvent(KParts::ListingNotificationExtension::NotificationEventType type, const KFileItemList& items)
{
    if (!m_listingExt)
        return;

    switch (type) {
    case KParts::ListingNotificationExtension::ItemsAdded: {
        const QStringList filters = m_listingExt->filter(KParts::ListingFilterExtension::MimeType).toStringList();
        Q_FOREACH(const KFileItem & item, items) {
            const QString mimeType(item.mimetype());
            if (m_pMimeInfo.contains(mimeType)) {
                m_pMimeInfo[mimeType].filenames.insert(item.name());
            } else {
                MimeInfo& mimeInfo = m_pMimeInfo[mimeType];
                mimeInfo.useAsFilter = filters.contains(mimeType);
                mimeInfo.mimeComment = item.mimeComment();
                mimeInfo.iconName = item.iconName();
                mimeInfo.filenames.insert(item.name());
            }
        }
        break;
    }
    case KParts::ListingNotificationExtension::ItemsDeleted:
        Q_FOREACH(const KFileItem & item, items) {
            const QString mimeType(item.mimetype());
            MimeInfoMap::iterator it = m_pMimeInfo.find(mimeType);
            if (it != m_pMimeInfo.end()) {
                MimeInfo& info = it.value();

                if (info.filenames.size() > 1) {
                    info.filenames.remove(item.name());
                } else {
                    if (info.useAsFilter) {
                        QStringList filters = m_listingExt->filter(KParts::ListingFilterExtension::MimeType).toStringList();
                        filters.removeAll(mimeType);
                        m_listingExt->setFilter(KParts::ListingFilterExtension::MimeType, filters);
                        saveTypeFilters(m_part->url(), filters);
                    }
                    m_pMimeInfo.erase(it);
                }
            }
        }
        break;
    default:
        return;
    }

    // Enable/disable mime based filtering depending on whether the number
    // document types in the current directory.
    if (m_filterBar) {
        m_filterBar->setEnableTypeFilterMenu(m_pMimeInfo.count() > 1);
    }
}

void DirFilterPlugin::slotReset()
{
    if (!m_part || !m_listingExt)
        return;

    MimeInfoMap::const_iterator itEnd = m_pMimeInfo.end();
    for (MimeInfoMap::iterator it = m_pMimeInfo.begin(); it != itEnd; ++it)
        it.value().useAsFilter = false;

    const QStringList filters;
    m_listingExt->setFilter(KParts::ListingFilterExtension::MimeType, filters);
    saveTypeFilters(m_part->url(), filters);
}

void DirFilterPlugin::slotShowCount()
{
    globalSessionManager->showCount = !globalSessionManager->showCount;
}

void DirFilterPlugin::slotMultipleFilters()
{
    globalSessionManager->useMultipleFilters = !globalSessionManager->useMultipleFilters;
}

void DirFilterPlugin::slotShowFilterBar()
{
    QWidget* partWidget = (m_part ? m_part->widget() : 0);

    if (!m_filterBar && partWidget) {
        m_filterBar = new FilterBar(partWidget);
        m_filterBar->setTypeFilterMenu(new QMenu(m_filterBar));
        connect(m_filterBar->typeFilterMenu(), SIGNAL(aboutToShow()),
                this, SLOT(slotShowPopup()));
        connect(m_filterBar->typeFilterMenu(), SIGNAL(triggered(QAction*)),
                this, SLOT(slotItemSelected(QAction*)));
        connect(m_filterBar, SIGNAL(filterChanged(QString)),
                this, SLOT(slotNameFilterChanged(QString)));
        connect(m_filterBar, SIGNAL(closeRequest()),
                this, SLOT(slotCloseRequest()));
        QBoxLayout* layout = qobject_cast<QBoxLayout*>(partWidget->layout());
        if (layout)
            layout->addWidget(m_filterBar);
    }

    // Get the widget that currently has the focus so we can properly
    // restore it when the filter bar is closed.
    QWidget* window = (partWidget ? partWidget->window() : 0);
    m_focusWidget = (window ? window->focusWidget() : 0);

    if (m_filterBar) {
        setFilterBar();
        m_filterBar->show();
    }
}

void DirFilterPlugin::setFilterBar()
{
    SessionManager::Filters savedFilters = globalSessionManager->restore(m_part->url());

    if (m_listingExt)
        m_listingExt->setFilter(KParts::ListingFilterExtension::MimeType, savedFilters.typeFilters);

    if (m_filterBar) {
        m_filterBar->setNameFilter(savedFilters.nameFilter);
        m_filterBar->setEnableTypeFilterMenu(m_pMimeInfo.count() > 1);
    }

    Q_FOREACH(const QString & mimeType, savedFilters.typeFilters) {
        if (m_pMimeInfo.contains(mimeType)) {
            m_pMimeInfo[mimeType].useAsFilter = true;
        }
    }
}

K_PLUGIN_FACTORY(DirFilterFactory, registerPlugin<DirFilterPlugin>();)
K_EXPORT_PLUGIN(DirFilterFactory("dirfilterplugin"))

#include "dirfilterplugin.moc"
