/*

  Support for scripting of plugins using the npruntime interface

  Copyright (c) 2006, 2010 Maksim Orlovich <maksim@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include "nsplugin.h"
#include "sdk/npruntime.h"
#include <kparts/browserextension.h>

#include <QHash>

namespace kdeNsPluginViewer {

class ScriptExportEngine
{
public:
  /* Tries to create a scripting manager if the plugin supports it.
      If not, returns 0
  */
  static ScriptExportEngine* create(NSPluginInstance* instance);

  /* Fills in the functions table with plugins API support */
  static void fillInScriptingFunctions(NPNetscapeFuncs* nsFuncs);

  ~ScriptExportEngine();

  /* LiveConnectExtension API... */
  bool get(const unsigned long objid, const QString& field, KParts::LiveConnectExtension::Type&  type, unsigned long& retobjid, QString& value);
  bool put(const unsigned long objid, const QString& field, const QString& value);
  bool call(const unsigned long objid, const QString& func, const QStringList& args,
                    KParts::LiveConnectExtension::Type& retType, unsigned long& retobjid, QString& value);
  void unregister(const unsigned long objid);
private:
  ScriptExportEngine(NSPluginInstance* inst, NPObject* root);

  void setupReturn(const NPVariant& result, KParts::LiveConnectExtension::Type& type,
                   unsigned long& retobjid, QString& value);

  QHash<unsigned long, NPObject*> _objectsForId;
  QHash<NPObject*, unsigned long> _objectIds;

  // Returns the object corresponding to ID, or null... 0 is the root object..
  NPObject* getScriptObject(unsigned long objid);

#if 0
  struct FunctionInfo {
    NPObject* baseObject; //Object we're a function of.. we retain it..
    QString   name;       //The function we are..

    unsigned long id; //Our ID
    unsigned long rc; //Our refcount..
  };

  QMap<unsigned long,             FunctionInfo*> _functionForId;
  QMap<QPair<NPObject*, QString>, FunctionInfo*> _functionForBaseAndName;

  //Returns the function corresponding to ID, or null..
  FunctionInfo* getFunctionInfo(const unsigned long objid);
#endif  

  unsigned long _nextId;
  unsigned long allocObjId (NPObject* object); 
  unsigned long allocFuncId(NPObject* object, QString field);

  NSPluginInstance* _pluginInstance;
  NPObject*         _liveConnectRoot;

  //Nocopyable...
  Q_DISABLE_COPY(ScriptExportEngine);
};

} // namespace kdeNsPluginViewer

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;

