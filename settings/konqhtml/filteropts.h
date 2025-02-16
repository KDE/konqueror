/*
    SPDX-FileCopyrightText: 2005 Ivor Hewitt <ivor@ivor.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef FILTEROPTS_H
#define FILTEROPTS_H

#include <QAbstractItemModel>
#include <QTabWidget>

#include <kcmodule.h>
#include <ksharedconfig.h>

class QListWidget;
class QPushButton;
class QLineEdit;
class QCheckBox;
class QTreeView;
class KListWidgetSearchLine;
class KPluralHandlingSpinBox;

/**
 * @brief Model which handles loading and saving the list of automatic filters
 *
 * The name of the configuration file where the list is stored is returned by autoFilterFileName()
 * and, currently, is `konqautofiltersrc`. This file should only be used to store the automatic filters
 * and have a group for each filter:
 * - the name of the group is the filter name
 * - it has entries telling whether it's enabled, its URL, the name of the cache file to use for it and
 *  the position where display it in the list of filters.
 *
 * @note This configuration file isn't used in the usual way. See save() for more information.
 */
class AutomaticFilterModel : public QAbstractItemModel
{
    Q_OBJECT

    /**
     * @brief The name of the config file where the list of filters is kept
     * @return The name of the config file where the list of filters is kept. Currently, this is `konqautofiltersrc`
     */
    static QString autoFilterFileName();

public:
    AutomaticFilterModel(QObject *parent = nullptr);

    void load();

    /**
     * @brief Saves the information to file
     *
     * Saving the information must take into account the situation where an entry has been removed
     * by the default configuration (for example because the list isn't maintained anymore):
     * - if the entry had been enabled by the user, we want to keep it not to surprise the user
     * - if the entry hasn't been enabled by the user, we don't want to keep it because it would clutter the UI.
     * To do so, we store all the information (not only those changed from the default) for enabled entries.
     *
     * @warning This function assumes that all filters are disabled by default. if this changes, the algorithm
     * which decides what entries should be saved must be changed
     */
    void save();
    void defaults();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

signals:
    void changed(bool);

private:
    struct FilterConfig {
        bool enableFilter;
        QString filterName;
        QString filterURL;
        QString filterLocalFilename;
        int position;
    };
    QList<struct FilterConfig> mFilters;

    KSharedConfig::Ptr mConfig;
    QString mGroupname;
};

class KCMFilter : public KCModule
{
    Q_OBJECT
public:
    //TODO KF6: when dropping compatibility with KF5, remove QVariantList argument
    KCMFilter(QObject *parent, const KPluginMetaData &md={}, const QVariantList &args={});
    ~KCMFilter() override;

    void load() override;
    void save() override;
    void defaults() override;

protected Q_SLOTS:
    void insertFilter();
    void updateFilter();
    void removeFilter();
    void slotItemSelected();
    void slotEnableChecked();
    void slotKillChecked();
    void slotInfoLinkActivated(const QString &url);

    void exportFilters();
    void importFilters();
    void updateButton();

    void spinBoxChanged(int);

private:
    QListWidget *mListBox;
    KListWidgetSearchLine *mSearchLine;
    QLineEdit *mString;
    QCheckBox *mEnableCheck;
    QCheckBox *mKillCheck;
    QPushButton *mInsertButton;
    QPushButton *mUpdateButton;
    QPushButton *mRemoveButton;
    QPushButton *mImportButton;
    QPushButton *mExportButton;
    QTabWidget *mFilterWidget;
    QTreeView *mAutomaticFilterList;
    KPluralHandlingSpinBox *mRefreshFreqSpinBox;

    KSharedConfig::Ptr mConfig;
    QString mGroupname;
    int mSelCount;
    QString mOriginalString;

    AutomaticFilterModel mAutomaticFilterModel;
};

#endif
