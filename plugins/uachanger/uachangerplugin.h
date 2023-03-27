/*
    SPDX-FileCopyrightText: 2001 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef __UACHANGER_PLUGIN_H
#define __UACHANGER_PLUGIN_H

#include <QMap>
#include <QStringList>
#include <QUrl>

#include <konq_kpart_plugin.h>
#include <kparts/readonlypart.h>

class KActionMenu;
class QAction;
class QActionGroup;
class KConfig;

namespace KIO
{
}

class UAChangerPlugin : public KonqParts::Plugin
{
    Q_OBJECT

public:
    explicit UAChangerPlugin(QObject *parent, const QVariantList &args);
    ~UAChangerPlugin() override;

protected slots:
    void slotDefault();
    void parseDescFiles();

    void slotConfigure();
    void slotAboutToShow();
    void slotApplyToDomain();
    void slotEnableMenu();
    void slotItemSelected(QAction *);
    void slotReloadDescriptions();

protected:
    QString findTLD(const QString &hostname);
    QString filterHost(const QString &hostname);

private:
    void reloadPage();
    void loadSettings();
    void saveSettings();

    int m_selectedItem;
    bool m_bApplyToDomain;
    bool m_bSettingsLoaded;

    KParts::ReadOnlyPart *m_part;
    KActionMenu *m_pUAMenu;
    KConfig *m_config;
    QAction *m_applyEntireSiteAction;
    QAction *m_defaultAction;
    QActionGroup *m_actionGroup;

    QUrl m_currentURL;
    QString m_currentUserAgent;

    QStringList m_lstAlias;    // menu entry names
    QStringList m_lstIdentity; // UA strings

    // A little wrapper around tag names so that other always goes to the end.
    struct MenuGroupSortKey {
        QString tag;
        bool    isOther;
        MenuGroupSortKey(): isOther(false) {}
        MenuGroupSortKey(const QString &t, bool oth): tag(t), isOther(oth) {}

        bool operator==(const MenuGroupSortKey &o) const
        {
            return tag == o.tag && isOther == o.isOther;
        }

        bool operator<(const MenuGroupSortKey &o) const
        {
            return (!isOther && o.isOther) || (tag < o.tag);
        }
    };

    typedef QList<int> BrowserGroup;
    typedef QMap<MenuGroupSortKey, BrowserGroup> AliasMap;
    typedef QMap<MenuGroupSortKey, QString> BrowserMap;

    typedef AliasMap::Iterator AliasIterator;
    typedef AliasMap::ConstIterator AliasConstIterator;

    BrowserMap m_mapBrowser; // tag -> menu name
    AliasMap m_mapAlias;     // tag -> UA string/menu entry name indices.
};

#endif
