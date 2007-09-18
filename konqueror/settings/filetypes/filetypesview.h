#ifndef FILETYPESVIEW_H
#define FILETYPESVIEW_H

#include <Qt3Support/Q3PtrList>
#include <QtCore/QMap>
#include <QtGui/QLabel>
#include <QtGui/QStackedWidget>

#include <KConfig>
#include <KSharedConfig>
#include <KCModule>

#include "typeslistitem.h"

class QLabel;
class K3ListView;
class Q3ListViewItem;
class QPushButton;
class KLineEdit;
class FileTypeDetails;
class FileGroupDetails;
class QStackedWidget;

class FileTypesView : public KCModule
{
  Q_OBJECT
public:
  FileTypesView(QWidget *parent, const QVariantList &args);
  ~FileTypesView();

  void load();
  void save();
  void defaults();

protected Q_SLOTS:
  /** fill in the various graphical elements, set up other stuff. */
  void init();

  void addType();
  void removeType();
  void updateDisplay(Q3ListViewItem *);
  void slotDoubleClicked(Q3ListViewItem *);
  void slotFilter(const QString &patternFilter);
  void setDirty(bool state);

  void slotDatabaseChanged();
  void slotEmbedMajor(const QString &major, bool &embed);

protected:
  void readFileTypes();
  bool sync( QList<TypesListItem *>& itemsModified );

private:
  K3ListView *typesLV;
  QPushButton *m_removeTypeB;

  QStackedWidget * m_widgetStack;
  FileTypeDetails * m_details;
  FileGroupDetails * m_groupDetails;
  QLabel * m_emptyWidget;

  KLineEdit *patternFilterLE;
  QStringList removedList;
  bool m_dirty;
  QMap<QString,TypesListItem*> m_majorMap;
  Q3PtrList<TypesListItem> m_itemList;

  QList<TypesListItem *> m_itemsModified;

  KSharedConfig::Ptr m_konqConfig;
};

#endif
