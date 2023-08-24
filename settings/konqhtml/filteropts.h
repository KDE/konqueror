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

class AutomaticFilterModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    AutomaticFilterModel(QObject *parent = nullptr);

    void load(KConfigGroup &cg);
    void save(KConfigGroup &cg);
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

#if QT_VERSION_MAJOR < 6
    void setNeedsSave(bool needs) {emit changed(needs);}
#endif

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
