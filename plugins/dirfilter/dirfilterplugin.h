/*
    SPDX-FileCopyrightText: 2000-2011 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef DIR_FILTER_PLUGIN_H
#define DIR_FILTER_PLUGIN_H

#include <QSet>
#include <QPointer>
#include <QStringList>
#include <QWidget>
#include <QMenu>
#include <QUrl>

#include <konq_kpart_plugin.h>
#include <KParts/ListingFilterExtension>
#include <KParts/ListingNotificationExtension>

class QPushButton;
class KFileItemList;
class KLineEdit;
namespace KParts
{
class ReadOnlyPart;
}

class FilterBar : public QWidget
{
    Q_OBJECT

public:
    explicit FilterBar(QWidget *parent = nullptr);
    ~FilterBar() override;
    void selectAll();

    QMenu *typeFilterMenu();
    void setTypeFilterMenu(QMenu *);

    bool typeFilterMenuEnabled() const;
    void setEnableTypeFilterMenu(bool);

    void setNameFilter(const QString &);

public Q_SLOTS:
    void clear();

Q_SIGNALS:
    void filterChanged(const QString &nameFilter);
    void closeRequest();

protected:
    void showEvent(QShowEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    KLineEdit *m_filterInput;
    QPushButton *m_typeFilterButton;
};

class SessionManager
{
public:
    struct Filters {
        QStringList typeFilters;
        QString nameFilter;
    };

    SessionManager();
    ~SessionManager();
    Filters restore(const QUrl &url);
    void save(const QUrl &url, const Filters &filters);

    bool showCount;
    bool useMultipleFilters;

protected:
    void loadSettings();
    void saveSettings();

private:
    bool m_bSettingsLoaded;
    QMap<QString, Filters> m_filters;
};

class DirFilterPlugin : public KonqParts::Plugin
{
    Q_OBJECT

public:

    DirFilterPlugin(QObject *parent, const QVariantList &);
    ~DirFilterPlugin() override;

private Q_SLOTS:
    void slotReset();
    void slotOpenURL();
    void slotOpenURLCompleted();
    void slotShowPopup();
    void slotShowCount();
    void slotShowFilterBar();
    void slotMultipleFilters();
    void slotItemSelected(QAction *);
    void slotNameFilterChanged(const QString &);
    void slotCloseRequest();
    void slotListingEvent(KParts::ListingNotificationExtension::NotificationEventType, const KFileItemList &);

private:
    void setFilterBar();

    struct MimeInfo {
        MimeInfo() : action(nullptr), useAsFilter(false) {}

        QAction *action;
        bool useAsFilter;

        QString iconName;
        QString mimeComment;

        QSet<QString> filenames;
    };
    typedef QMap<QString, MimeInfo> MimeInfoMap;

    FilterBar *m_filterBar;
    QWidget *m_focusWidget;
    QPointer<KParts::ReadOnlyPart> m_part;
    QPointer<KParts::ListingFilterExtension> m_listingExt;
    MimeInfoMap m_pMimeInfo;
};

#endif

