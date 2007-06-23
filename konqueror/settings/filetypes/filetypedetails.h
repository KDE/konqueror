#ifndef _FILETYPEDETAILS_H
#define _FILETYPEDETAILS_H

#include <QtGui/QTabWidget>
class TypesListItem;
class KIconButton;
class QLineEdit;
class QListWidget;
class QGroupBox;
class QButtonGroup;
class QCheckBox;
class QRadioButton;
class QPushButton;
class KServiceListWidget;

/**
 * This widget contains the right part of the file type configuration
 * dialog, that shows the details for a file type.
 * It is implemented as a separate class so that it can be used by
 * the keditfiletype program to show the details of a single mimetype.
 */
class FileTypeDetails : public QTabWidget
{
  Q_OBJECT
public:
  FileTypeDetails(QWidget *parent = 0);

  void setTypeItem( TypesListItem * item );

protected:
  void updateRemoveButton();
  void updateAskSave();

Q_SIGNALS:
  void embedMajor(const QString &major, bool &embed); // To adjust whether major type is being embedded
  void changed(bool);

protected Q_SLOTS:
  void updateIcon(const QString &icon);
  void updateDescription(const QString &desc);
  void addExtension();
  void removeExtension();
  void enableExtButtons();
  void slotAutoEmbedClicked(int button);
  void slotAskSaveToggled(bool);

private:
  TypesListItem * m_item;

  // First tab - General
  KIconButton *iconButton;
  QListWidget *extensionLB;
  QPushButton *addExtButton, *removeExtButton;
  QLineEdit *description;
  KServiceListWidget *serviceListWidget;

  // Second tab - Embedding
  QGroupBox *m_autoEmbedBox;
  QButtonGroup *m_autoEmbedGroup;
  KServiceListWidget *embedServiceListWidget;
  QRadioButton *m_rbOpenSeparate;
  QCheckBox *m_chkAskSave;
  QRadioButton *m_rbGroupSettings;
};

#endif
