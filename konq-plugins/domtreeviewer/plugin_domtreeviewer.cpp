#include "plugin_domtreeviewer.h"
#include "domtreewindow.h"
#include "domtreeview.h"

#include <kaction.h>
#include <kcomponentdata.h>

#include <kdebug.h>
#include <klocale.h>
#include <kgenericfactory.h>
#include <kactioncollection.h>
#include <khtml_part.h>

K_PLUGIN_FACTORY(DomtreeviewerFactory, registerPlugin<PluginDomtreeviewer>();)
K_EXPORT_PLUGIN(DomtreeviewerFactory( "domtreeviewer" ))


PluginDomtreeviewer::PluginDomtreeviewer( QObject* parent, 
	                                  const QVariantList & )
  : Plugin( parent ), m_dialog( 0 )
{
  QAction *a = actionCollection()->addAction("viewdomtree");

  a->setText(i18n("Show &DOM Tree"));
  a->setIcon(KIcon("view-web-browser-dom-tree"));
  connect(a, SIGNAL(triggered()), this, SLOT(slotShowDOMTree()));
}

PluginDomtreeviewer::~PluginDomtreeviewer()
{
  kDebug(90180) ;
  delete m_dialog;
}

void PluginDomtreeviewer::slotShowDOMTree()
{
  if ( m_dialog )
  {
    delete m_dialog;
    Q_ASSERT((DOMTreeWindow *)m_dialog == (DOMTreeWindow *)0);
  }
  if (KHTMLPart *part = qobject_cast<KHTMLPart *>(parent()))
  {
    m_dialog = new DOMTreeWindow(this);
    connect( m_dialog, SIGNAL( destroyed() ), this, SLOT( slotDestroyed() ) );
    m_dialog->view()->setHtmlPart(part);
    m_dialog->show();
  }
}

void PluginDomtreeviewer::slotDestroyed()
{
  kDebug(90180) ;
  m_dialog = 0;
}

#include <plugin_domtreeviewer.moc>
