/*
  Copyright (C) 2005 Ivor Hewitt <ivor@ivor.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef FILTEROPTS_H
#define FILTEROPTS_H

#include <QAbstractItemModel>

#include <kcmodule.h>
#include <ksharedconfig.h>

class KListWidget;
class KPushButton;
class QLineEdit;
class QCheckBox;
class KTabWidget;
class KListWidgetSearchLine;
class QTreeView;
class KIntSpinBox;

class AutomaticFilterModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    AutomaticFilterModel(QObject * parent = 0);

    void load(KConfigGroup &cg);
    void save(KConfigGroup &cg);
    void defaults();

    virtual QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex & index) const;
    virtual bool hasChildren(const QModelIndex & parent = QModelIndex()) const;
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual Qt::ItemFlags flags ( const QModelIndex & index ) const;

signals:
    void changed( bool );

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
    KCMFilter( QWidget* parent, const QVariantList& );
    ~KCMFilter();

    void load();
    void save();
    void defaults();
    QString quickHelp() const;

public Q_SLOTS:

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

    void spinBoxChanged( int );

private:
    KListWidget *mListBox;
    KListWidgetSearchLine *mSearchLine;
    QLineEdit *mString;
    QCheckBox *mEnableCheck;
    QCheckBox *mKillCheck;
    KPushButton *mInsertButton;
    KPushButton *mUpdateButton;
    KPushButton *mRemoveButton;
    KPushButton *mImportButton;
    KPushButton *mExportButton;
    KTabWidget *mFilterWidget;
    QTreeView *mAutomaticFilterList;
    KIntSpinBox *mRefreshFreqSpinBox;

    KSharedConfig::Ptr mConfig;
    QString mGroupname;
    int mSelCount;
    QString mOriginalString;

    AutomaticFilterModel mAutomaticFilterModel;
};

#endif
