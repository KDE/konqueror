
#ifndef __plugin_domtreeviewer_h
#define __plugin_domtreeviewer_h

#include <kparts/plugin.h>

class DOMTreeWindow;

class PluginDomtreeviewer : public KParts::Plugin
{
  Q_OBJECT
public:
  PluginDomtreeviewer( QObject* parent, 
	               const QVariantList & );
  virtual ~PluginDomtreeviewer();

public slots:
  void slotShowDOMTree();
  void slotDestroyed();
private:
  DOMTreeWindow* m_dialog;
};

#endif
