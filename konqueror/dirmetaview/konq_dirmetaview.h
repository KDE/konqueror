#ifndef __konq_dirmetaview__
#define __konq_dirmetaview__

#include <qframe.h>
#include <qmultilineedit.h>

#include <kparts/browserextension.h>
#include <kparts/part.h>
#include <kparts/factory.h>

#include <kmimetype.h>
#include <kio/job.h>

class QLabel;
class QHBoxLayout;
class KSeparator;
class KonqMetaDataProvider;

namespace Konqueror
{

class DirDetailViewFactory : public KParts::Factory
{
  Q_OBJECT
public:
  DirDetailViewFactory( QObject *parent = 0, const char *name = 0 );
  virtual ~DirDetailViewFactory();

  virtual KParts::Part *createPart( QWidget *parentWidget = 0, const char *widgetName = 0, QObject *parent = 0, const char *name = 0, const char *classname = "KParts::Part", const QStringList &args = QStringList() );

  KMimeType::Ptr dirMimeType() { return m_dirMimeType; }

private:
  KMimeType::Ptr m_dirMimeType;
};

class DetailWidget;

class AnnotationEdit : public QMultiLineEdit
{
  Q_OBJECT
public:
  AnnotationEdit( DetailWidget *parent, const char *name );
  virtual ~AnnotationEdit();

protected:
  virtual void focusOutEvent( QFocusEvent *e );
  virtual void keyPressEvent( QKeyEvent *e );

private:
  DetailWidget *m_parent;
};

class DirDetailView; 

class DetailWidget : public QWidget
{
  Q_OBJECT
public:

  DetailWidget( DirDetailView *parent, QWidget *parentWidget, const char *name );
  virtual ~DetailWidget();

  void setPixmap( const QPixmap &pix ) { m_pix = pix; }
  void setText( const QString &text ) { m_text = text; }

  void editDone();
protected:
  virtual void paintEvent( QPaintEvent * );
  virtual void mouseReleaseEvent( QMouseEvent *e );

  QString m_text;
  QPixmap m_pix;
  QPixmap m_bg;

  QRect m_editRect;

  QGuardedPtr<AnnotationEdit> m_edit;

  DirDetailView *m_parent;
};

class DirDetailView : public KParts::ReadOnlyPart
{
  Q_OBJECT
public:
  DirDetailView( DirDetailViewFactory *factory, QWidget *parentWidget, const char *widgetName, QObject *parent, const char *name );
  virtual ~DirDetailView();

  virtual bool openURL( const KURL &url );

  bool openURL( const KURL &url, KonqFileItemList selection );

  virtual bool openFile() { return true; }

  virtual bool eventFilter( QObject *obj, QEvent *event );

  void saveAnnotation( const QString &text );

protected slots:
  void slotUpdate();

protected:
  DetailWidget *m_widget;

  KURL m_url;

  KURL m_currentURL;
  QString m_currentServiceType;

  DirDetailViewFactory *m_factory;

  KURL::List m_currentSelection;

  KonqMetaDataProvider *m_metaDataProvider;

  KonqFileItemList m_currentFileItemSelection;

  bool forceUpdate;
};

};

#endif
