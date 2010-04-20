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

#include <QtCore/QtGlobal>
#include <QtCore/QHash>

#include <cstring>

#include "scripting.h"

namespace kdeNsPluginViewer {

// npruntime API --- variants & identifiers
//-----------------------------------------------------------------------------

static void g_NPN_ReleaseObject(NPObject* object);

static void g_NPN_ReleaseVariantValue(NPVariant* variant)
{
    if (!variant) return;

    if (variant->type == NPVariantType_Object)
        g_NPN_ReleaseObject(variant->value.objectValue);
    else if (variant->type == NPVariantType_String)
        g_NPN_MemFree(const_cast<NPUTF8*>(variant->value.stringValue.UTF8Characters));
}

struct NSPluginIdentifier
{
    QString str;
    qint32  num;
    bool isString;
};

static QHash<QString, NSPluginIdentifier*> stringIdents;
static QHash<int32,   NSPluginIdentifier*> intIdents;
 /* unfortunately, since this not per-session we can't clean up int idents.
    ### on 64-bit, can use small-value optimization-like encoding for them
  */

//Internal -- not part of standard API..
static NPIdentifier g_NPN_GetQStringIdentifier(const QString& str)
{
    QHash<QString, NSPluginIdentifier*>::const_iterator i = stringIdents.constFind(str);
    if (i != stringIdents.constEnd())
        return i.value();

    NSPluginIdentifier* ident = new NSPluginIdentifier();
    ident->isString = true;
    ident->str      = str;
    stringIdents[str] = ident;
    return ident;
}

static NPIdentifier g_NPN_GetStringIdentifier(const NPUTF8* name)
{
    QString str = QString::fromUtf8(name);
    return g_NPN_GetQStringIdentifier(str);
}

static void g_NPN_GetStringIdentifiers(const NPUTF8 **names, qint32 nameCount,
                                NPIdentifier* identifiers)
{
    for (qint32 p = 0; p < nameCount; ++p)
        identifiers[p] = g_NPN_GetStringIdentifier(names[p]);
}

static NPIdentifier g_NPN_GetIntIdentifier(qint32 intid)
{
    QHash<qint32, NSPluginIdentifier*>::const_iterator i = intIdents.constFind(intid);
    if (i != intIdents.constEnd())
        return i.value();

    NSPluginIdentifier *ident = new NSPluginIdentifier();
    ident->isString = false;
    ident->num      = intid;
    intIdents[intid] = ident;
    return ident;
}

static bool g_NPN_IdentifierIsString(NPIdentifier identifier)
{
    return reinterpret_cast<NSPluginIdentifier*>(identifier)->isString;
}

static NPUTF8* g_NPN_UTF8FromIdentifier(NPIdentifier identifier)
{
    NSPluginIdentifier* ident = reinterpret_cast<NSPluginIdentifier*>(identifier);
    if (!ident->isString)
        return 0;

    // This deep copies as the API docs state the caller is responsible for freeing the string
    // we also include the trailing null in memory, but not length, like
    // QByteArray does.

    QByteArray utf8 = ident->str.toUtf8();
    int fullLength = utf8.size() + 1; //Includes 0...
    NPUTF8* buf = static_cast<NPUTF8*>(g_NPN_MemAlloc(fullLength));
    std::memcpy(buf, utf8.constData(), fullLength);
    return buf;
}

static int32_t g_NPN_IntFromIdentifier(NPIdentifier identifier)
{
    NSPluginIdentifier *ident = reinterpret_cast<NSPluginIdentifier*>(identifier);
    if (ident->isString)
        return 0;
    return ident->num;
}

// npruntime API --- objects
//-----------------------------------------------------------------------------

static NPObject* g_NPN_CreateObject(NPP npp, NPClass* aClass)
{
    NPObject* obj;
    if (aClass && aClass->allocate)
        obj = aClass->allocate(npp, aClass);
    else
        obj = (NPObject*)::malloc(sizeof(NPObject));

    obj->_class = aClass;
    obj->referenceCount = 1;
    return obj;
}

static NPObject* g_NPN_RetainObject(NPObject* npobj)
{
    if (npobj)
        ++npobj->referenceCount;

    return npobj;
}

static void g_NPN_ReleaseObject(NPObject* npobj)
{
    if (npobj) {
        --npobj->referenceCount;

        if (npobj->referenceCount == 0) {
            if (npobj->_class && npobj->_class->deallocate)
                npobj->_class->deallocate(npobj);
            else
                ::free(npobj);
        }
    }
}

static bool g_NPN_Invoke(NPP, NPObject* npobj, NPIdentifier name,
                const NPVariant* args, quint32 argCount, NPVariant* result)
{
    if (npobj && npobj->_class && npobj->_class->invoke)
        return npobj->_class->invoke(npobj, name, args, argCount, result);
    return false;
}

static bool g_NPN_InvokeDefault(NPP, NPObject* npobj, const NPVariant* args ,
                                quint32 argCount, NPVariant* result)
{
    if (npobj && npobj->_class && npobj->_class->invokeDefault)
        return npobj->_class->invokeDefault(npobj, args, argCount, result);
    return false;
}

static bool g_NPN_GetProperty(NPP /*npp*/, NPObject* npobj, NPIdentifier name, NPVariant* result)
{
    if (npobj && npobj->_class && npobj->_class->getProperty)
        return npobj->_class->getProperty(npobj, name, result);

    return false;
}

static bool g_NPN_SetProperty(NPP, NPObject* npobj, NPIdentifier name, const NPVariant* value)
{
    if (npobj && npobj->_class && npobj->_class->setProperty)
        return npobj->_class->setProperty(npobj, name, value);

    return false;
}

static bool g_NPN_HasProperty(NPP, NPObject* npobj, NPIdentifier name)
{
    if (npobj && npobj->_class && npobj->_class->hasProperty)
        return npobj->_class->hasProperty(npobj, name);
    return false;
}

static bool g_NPN_RemoveProperty(NPP, NPObject* npobj, NPIdentifier name)
{
    if (npobj && npobj->_class && npobj->_class->removeProperty)
        return npobj->_class->removeProperty(npobj, name);
    return false;
}

static bool g_NPN_HasMethod(NPP, NPObject* npobj, NPIdentifier name)
{
    if (npobj && npobj->_class && npobj->_class->hasMethod)
        return npobj->_class->hasMethod(npobj, name);
    return false;
}

static bool g_NPN_Enumerate(NPP, NPObject* npobj, NPIdentifier **identifiers,
                            quint32* identifierCount)
{
    if (npobj && npobj->_class && NP_CLASS_STRUCT_VERSION_HAS_ENUM(npobj->_class)
              && npobj->_class->enumerate)
        return npobj->_class->enumerate(npobj, identifiers, identifierCount);

    return false;
}

static bool g_NPN_Construct(NPP, NPObject* npobj, const NPVariant* args,
                            quint32 argCount, NPVariant* result)
{
    if (npobj && npobj->_class && NP_CLASS_STRUCT_VERSION_HAS_CTOR(npobj->_class)
              && npobj->_class->construct)
        return npobj->_class->construct(npobj, args, argCount, result);

    return false;
}

static void g_NPN_SetException(NPObject* /*npobj*/, const NPUTF8* /*message*/)
{
    kDebug(1431) << "[unimplemented]";
}

static bool g_NPN_Evaluate(NPP /*npp*/, NPObject* /*npobj*/, NPString* /*script*/,
                  NPVariant* /*result*/)
{
    kDebug(1431) << "[unimplemented]";
    return false;
}

// ### invalidate, NPN_evaluate, setException -- what should they do?


// now, a little adapter for representing native objects as NPRuntime ones. 
//-----------------------------------------------------------------------------

// ScriptExportEngine
//-----------------------------------------------------------------------------
void ScriptExportEngine::fillInScriptingFunctions(NPNetscapeFuncs* nsFuncs)
{
   nsFuncs->getstringidentifier  = g_NPN_GetStringIdentifier;
   nsFuncs->getstringidentifiers = g_NPN_GetStringIdentifiers;
   nsFuncs->getintidentifier     = g_NPN_GetIntIdentifier;
   nsFuncs->identifierisstring   = g_NPN_IdentifierIsString;
   nsFuncs->utf8fromidentifier   = g_NPN_UTF8FromIdentifier;
   nsFuncs->intfromidentifier    = g_NPN_IntFromIdentifier;
   nsFuncs->createobject         = g_NPN_CreateObject;
   nsFuncs->retainobject         = g_NPN_RetainObject;
   nsFuncs->releaseobject        = g_NPN_ReleaseObject;
   nsFuncs->invoke               = g_NPN_Invoke;
   nsFuncs->invokeDefault        = g_NPN_InvokeDefault;
   nsFuncs->evaluate             = g_NPN_Evaluate;
   nsFuncs->getproperty          = g_NPN_GetProperty;
   nsFuncs->setproperty          = g_NPN_SetProperty;
   nsFuncs->removeproperty       = g_NPN_RemoveProperty;
   nsFuncs->hasproperty          = g_NPN_HasProperty;
   nsFuncs->hasmethod            = g_NPN_HasMethod;
   nsFuncs->releasevariantvalue  = g_NPN_ReleaseVariantValue;
   nsFuncs->setexception         = g_NPN_SetException;
   nsFuncs->enumerate            = g_NPN_Enumerate;
   nsFuncs->construct            = g_NPN_Construct;
}

ScriptExportEngine* ScriptExportEngine::create(NSPluginInstance* inst) {
    NPObject* rootObj = 0;
    if (inst->NPGetValue(NPPVpluginScriptableNPObject, (void*)&rootObj) == NPERR_NO_ERROR) {
        kDebug(1431) << "Detected support for scripting, root = " <<rootObj << endl;
        if (rootObj)
            return new ScriptExportEngine(inst, rootObj);
    }
    return 0;
}

ScriptExportEngine::ScriptExportEngine(NSPluginInstance* inst, NPObject* root):
         _nextId(0), _pluginInstance(inst), _liveConnectRoot(root)
{
    kDebug(1431) << _liveConnectRoot->referenceCount;
    allocObjId(root); //Setup root as 0..
}

ScriptExportEngine::~ScriptExportEngine()
{
    kDebug(1431) << _liveConnectRoot->referenceCount;
    g_NPN_ReleaseObject(_liveConnectRoot);
}

NPObject* ScriptExportEngine::getScriptObject(unsigned long objid)
{
    QHash<unsigned long, NPObject*>::const_iterator i = _objectsForId.constFind(objid);
    if (i == _objectsForId.constEnd())
        return 0;
    else
        return i.value();
}

unsigned long ScriptExportEngine::allocObjId(NPObject* object)
{
    while(true) {
        if (!_objectsForId.contains(_nextId)) { // && !_functionForId.contains(_nextId)) {
            unsigned long freeID = _nextId;
            ++_nextId;

            _objectsForId[freeID] = object;
            _objectIds[object]    = freeID;

            return freeID;
        }
        ++_nextId;
    }
}

bool ScriptExportEngine::get(const unsigned long objid, const QString& field,
                             KParts::LiveConnectExtension::Type& typeOut,
                             unsigned long& retobjid, QString& value)
{
    return false;
}

bool ScriptExportEngine::put(const unsigned long objid, const QString& field,
                             const QString& value)
{
    return false;
}

bool ScriptExportEngine::call(const unsigned long objid, const QString& func, const QStringList& args,
                    KParts::LiveConnectExtension::Type& retType, unsigned long& retobjid, QString& value)
{
    return false;
}

void ScriptExportEngine::unregister(const unsigned long objid)
{
}

} // namespace kdeNsPluginViewer
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
