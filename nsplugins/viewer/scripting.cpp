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

#include <cassert>
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
static QHash<int32_t, NSPluginIdentifier*> intIdents;
 /* unfortunately, since this not per-session we can't clean up int idents.
    ### on 64-bit, can use small-value optimization-like encoding for them
  */

//Internal -- not part of standard API..
static NPIdentifier g_NPN_GetQStringIdentifier(const QString& str)
{
    QHash<QString, NSPluginIdentifier*>::const_iterator i = stringIdents.constFind(str);
    if (i != stringIdents.constEnd()) {
        return i.value();
    }

    NSPluginIdentifier* ident = new NSPluginIdentifier();
    ident->isString = true;
    ident->str      = str;
    stringIdents[str] = ident;
    return ident;
}

// As above -- this one is in the other direction
static QString g_NPN_GetIdentifierQString(NPIdentifier n)
{
    NSPluginIdentifier* ident = reinterpret_cast<NSPluginIdentifier*>(n);
    if (!ident->isString)
        return QString::number(ident->num);
    else
        return ident->str;
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

// Internal -- not part of public API
static void g_NPN_SetVariantFromQString(NPVariant* v, const QString& s)
{
    // note: we also include a trailing null with utf8 data, just in case
    v->type = NPVariantType_String;
    v->value.stringValue.UTF8Length     = s.length();
    v->value.stringValue.UTF8Characters =
                static_cast<NPUTF8*>(g_NPN_MemAlloc(s.length() + 1));
     
    QByteArray utf8 = s.toUtf8();
    std::memcpy(const_cast<NPUTF8*>(v->value.stringValue.UTF8Characters),
                utf8.constData(), s.length() + 1);
}

// Internal, for debug purposes
static QString dumpVariant(const NPVariant& v)
{
    switch (v.type) {
        case NPVariantType_Void:
            return QLatin1String("void");
        case NPVariantType_Null:
            return QLatin1String("null");
        case NPVariantType_Bool:
            return QLatin1String(v.value.boolValue ? "true" : "false");
        case NPVariantType_Int32:
            return QLatin1String("i:") + QString::number(v.value.intValue);
        case NPVariantType_Double:
            return QLatin1String("d:") + QString::number(v.value.doubleValue);
        case NPVariantType_String:
            return QLatin1String("s:") +
                QString::fromUtf8(v.value.stringValue.UTF8Characters,
                                  v.value.stringValue.UTF8Length);
        case NPVariantType_Object:
            return QLatin1String("o");
        default:
            return QLatin1String("Unknown type");
     }
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
    if (npobj && npobj->_class && npobj->_class->invoke) {
        return npobj->_class->invoke(npobj, name, args, argCount, result);
    }
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


// now, a little adapter for bridging from NPRuntime type system
// into proper C++ subclasses.
//-----------------------------------------------------------------------------
class ScriptableObjectBase
{
public:
    virtual bool hasMethod(QString m) {
        kDebug(1431) << "[unimplemented]" << m;
        return false;
    }

    virtual bool hasProperty(QString p) {
        kDebug(1431) << "[unimplemented]" << p;
        return false;
    }

    virtual bool invoke(QString method, QStringList args, QString* /*out*/) {
        kDebug(1431) << "[unimplemented]" << method << args;
        return false;
    }

    virtual bool invokeDefault(QStringList args, QString* /*out*/) {
        kDebug(1431) << "[unimplemented]" << args;
        return false;
    }

    virtual bool construct(QStringList args, QString* /*out*/) {
        kDebug(1431) << "[unimplemented]" << args;
        return false;
    }

    virtual bool get(QString prop, QString* /*out*/) {
        kDebug(1431) << "[unimplemented]" << prop;
        return false;
    }

    virtual bool removeProperty(QString prop) {
        kDebug(1431) << "[unimplemented]" << prop;
        return false;
    }

    virtual bool setProperty(QString prop, QString val) {
        kDebug(1431) << "[unimplemented]" << prop << val;
        return false;
    }

    virtual QStringList enumeration() {
        kDebug(1431) << "[unimplemented]";
        return QStringList();
    }

    virtual ~ScriptableObjectBase() {}
};

struct NPObjectWrapper: public NPObject
{
    ScriptableObjectBase* impl;
};

template<typename ImplType>
NPObject* wrapAlloc(NPP /*npp*/, NPClass* /*aClass*/)
{
    ImplType*        imp  = new ImplType;
    NPObjectWrapper* wrap = new NPObjectWrapper;
    wrap->impl = imp;
    wrap->referenceCount = 1;
    return wrap;
}

template<typename ImplType>
void wrapDealloc(NPObject *npobj)
{
    NPObjectWrapper* wrap = static_cast<NPObjectWrapper*>(npobj);
    delete wrap->impl;
    delete wrap;
}

static void wrapInvalidate(NPObject* npobj)
{
    NPObjectWrapper* wrap = static_cast<NPObjectWrapper*>(npobj);
    delete wrap->impl;
    wrap->impl = 0;
}

static bool wrapHasMethod(NPObject* npobj, NPIdentifier name)
{
    NPObjectWrapper* wrap = static_cast<NPObjectWrapper*>(npobj);
    return wrap->impl->hasMethod(g_NPN_GetIdentifierQString(name));
}

static bool wrapInvoke(NPObject *npobj, NPIdentifier name,
                       const NPVariant *args, quint32 argCount,
                       NPVariant *result)
{
    NPObjectWrapper* wrap = static_cast<NPObjectWrapper*>(npobj);

    QStringList qa;
    for (unsigned i = 0; i < argCount; ++i)
        qa << dumpVariant(args[i]);

    QString ourRes;
    if (wrap->impl->invoke(g_NPN_GetIdentifierQString(name), qa, &ourRes)) {
        g_NPN_SetVariantFromQString(result, ourRes);
        return true;
    }

    return false;
}

static bool wrapInvokeDefault(NPObject *npobj, 
                       const NPVariant *args, quint32 argCount,
                       NPVariant *result)
{
    NPObjectWrapper* wrap = static_cast<NPObjectWrapper*>(npobj);

    QStringList qa;
    for (unsigned i = 0; i < argCount; ++i)
        qa << dumpVariant(args[i]);

    QString ourRes;
    if (wrap->impl->invokeDefault(qa, &ourRes)) {
        g_NPN_SetVariantFromQString(result, ourRes);
        return true;
    }

    return false;
}

static bool wrapConstruct(NPObject *npobj,
                       const NPVariant *args, quint32 argCount,
                       NPVariant *result)
{
    NPObjectWrapper* wrap = static_cast<NPObjectWrapper*>(npobj);

    QStringList qa;
    for (unsigned i = 0; i < argCount; ++i)
        qa << dumpVariant(args[i]);

    QString ourRes;
    if (wrap->impl->construct(qa, &ourRes)) {
        g_NPN_SetVariantFromQString(result, ourRes);
        return true;
    }

    return false;
}

static bool wrapHasProperty(NPObject* npobj, NPIdentifier name)
{
    return static_cast<NPObjectWrapper*>(npobj)->impl->hasProperty(g_NPN_GetIdentifierQString(name));
}

static bool wrapGetProperty(NPObject* npobj, NPIdentifier name, NPVariant* result)
{
    QString ourRes;
    if (static_cast<NPObjectWrapper*>(npobj)->impl->get(g_NPN_GetIdentifierQString(name), &ourRes)) {
        g_NPN_SetVariantFromQString(result, ourRes);
        return true;
    }
    return false;
}

static bool wrapSetProperty(NPObject* npobj, NPIdentifier name, const NPVariant* value)
{
    return static_cast<NPObjectWrapper*>(npobj)->impl->setProperty(
                g_NPN_GetIdentifierQString(name), dumpVariant(*value));
}

static bool wrapRemoveProperty(NPObject* npobj, NPIdentifier name)
{
    return static_cast<NPObjectWrapper*>(npobj)->impl->removeProperty(g_NPN_GetIdentifierQString(name));
}

static bool wrapEnumeration(NPObject* npobj, NPIdentifier** value, quint32* count)
{
    QStringList props = static_cast<NPObjectWrapper*>(npobj)->impl->enumeration();

    NPIdentifier* array = reinterpret_cast<NPIdentifier*>(g_NPN_MemAlloc(sizeof(NPIdentifier) * props.size()));
    for (int i = 0; i < props.size(); ++i)
        array[i] = g_NPN_GetQStringIdentifier(props[i]);

    *count = props.size();
    *value = array;
    return true;
}

template<typename ImplClass>
struct NPWrapperClass: public NPClass
{
    NPWrapperClass() {
        structVersion  = NP_CLASS_STRUCT_VERSION;
        allocate       = wrapAlloc<ImplClass>;
        deallocate     = wrapDealloc<ImplClass>;
        invalidate     = wrapInvalidate;
        hasMethod      = wrapHasMethod;
        invoke         = wrapInvoke;
        invokeDefault  = wrapInvokeDefault;
        hasProperty    = wrapHasProperty;
        getProperty    = wrapGetProperty;
        setProperty    = wrapSetProperty;
        removeProperty = wrapRemoveProperty;
        enumerate      = wrapEnumeration;
        construct      = wrapConstruct;
    }
};

static NPWrapperClass<ScriptableObjectBase> stubClass;

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

// This emulates as little we can get away with to get Flash sorta-scriptable
class FakeWindowObject: public ScriptableObjectBase
{
public:
    FakeWindowObject(): _plugin(0)
    {}

    FakeWindowObject(NSPluginInstance* plugin): _plugin(plugin)
    {}

    virtual bool get(QString prop, QString* out) {
        kDebug(1431) << prop;
        assert(_plugin);

        if (prop == QLatin1String("location") && !_plugin->pageURL().isEmpty()) {
            kDebug(1431) << " -> reporting page URL:" << _plugin->pageURL();
            *out = _plugin->pageURL();
            return true;
        }

        return false;
    }
    
private:
    NSPluginInstance* _plugin;
};

static NPWrapperClass<FakeWindowObject> windowClass;

void ScriptExportEngine::connectToPlugin() {
    NPObject* rootObj = 0;
    if (_pluginInstance->NPGetValue(NPPVpluginScriptableNPObject, (void*)&rootObj) == NPERR_NO_ERROR) {
        kDebug(1431) << "Detected support for scripting, root = " << rootObj << endl;

        _liveConnectRoot = rootObj;

        // Add it to it mappings as id = 0. We don't need to manual retain,
        // flash does.
        _objectForId[0] = rootObj;
        _objectIds[rootObj] = 0;
    }
}


static NPP dummy;

ScriptExportEngine::ScriptExportEngine(NSPluginInstance* inst):
         _nextId(1), _pluginInstance(inst), _liveConnectRoot(0)
{
    // Reserve a spot for root, even if we don't have it now.
    _objectForId[0] = 0;

    // Create our fake window object
    NPObjectWrapper* win = new NPObjectWrapper;
    win->_class = &windowClass;
    win->impl   = new FakeWindowObject(inst);
    win->referenceCount = 1;

    _window = win;    
    g_NPN_RetainObject(_window);

    // Stub for now
    _pluginElement = g_NPN_CreateObject(dummy, &stubClass);
    g_NPN_RetainObject(_pluginElement);
}

ScriptExportEngine::~ScriptExportEngine()
{
    // ### when should invalidation happen?

    // Release every object...    
    QHash<unsigned long, NPObject*>::iterator i = _objectForId.begin();
    while (i != _objectForId.end()) {
        g_NPN_ReleaseObject(i.value());
        ++i;
    }

    QHash<unsigned long, FuncRef>::iterator fi = _functionForId.begin();
    while (fi != _functionForId.end()) {
        g_NPN_ReleaseObject(fi.value().first);
        ++fi;
    }
}

NPObject* ScriptExportEngine::getScriptObject(unsigned long objid)
{
    QHash<unsigned long, NPObject*>::const_iterator i = _objectForId.constFind(objid);
    if (i == _objectForId.constEnd())
        return 0;
    else
        return i.value();
}

FuncRef* ScriptExportEngine::getScriptFunction(unsigned long objid)
{
    if (_functionForId.contains(objid))
        return &_functionForId[objid];
    else
        return 0;
}

unsigned long ScriptExportEngine::findFreeId()
{
    while(true) {
        if (!_objectForId.contains(_nextId) && !_functionForId.contains(_nextId)) {
            unsigned long freeID = _nextId;
            ++_nextId;
            return freeID;
        }
        ++_nextId;        
    }
}

unsigned long ScriptExportEngine::allocObjId(NPObject* object)
{
    unsigned long freeID = findFreeId();
    _objectForId[freeID] = object;
    _objectIds[object]   = freeID;
    return freeID;
}

unsigned long ScriptExportEngine::allocFuncId(FuncRef f)
{
    unsigned long freeID = findFreeId();
    _functionForId[freeID] = f;
    _functionIds  [f]      = freeID;
    return freeID;
}

unsigned long ScriptExportEngine::registerIfNeeded(NPObject* obj)
{
    QHash<NPObject*, unsigned long>::const_iterator i = _objectIds.constFind(obj);
    if (i != _objectIds.constEnd()) {
        return i.value();
    } else {
        g_NPN_RetainObject(obj);
        return allocObjId(obj);
    }
}

unsigned long ScriptExportEngine::registerFuncIfNeeded(FuncRef f)
{
    QHash<FuncRef, unsigned long>::const_iterator i = _functionIds.constFind(f);
    if (i != _functionIds.constEnd()) {
        return i.value();
    } else {
        g_NPN_RetainObject(f.first);
        return allocFuncId(f);
    }
}

void ScriptExportEngine::setupReturn(const NPVariant& result, KParts::LiveConnectExtension::Type& type,
                                     unsigned long& retobjid, QString& value)
{
    switch (result.type) {
        case NPVariantType_Void:
        case NPVariantType_Null: //### not quite accurate..
            type = KParts::LiveConnectExtension::TypeVoid;
            return;
        case NPVariantType_Bool:
            type = KParts::LiveConnectExtension::TypeBool;
            value = result.value.boolValue ? "true" : "false";
            return;
        case NPVariantType_Int32:
            type  = KParts::LiveConnectExtension::TypeNumber;
            value = QString::number(result.value.intValue);
            return;
        case NPVariantType_Double:
            type  = KParts::LiveConnectExtension::TypeNumber;
            value = QString::number(result.value.doubleValue);
            return;
        case NPVariantType_String:
            type  = KParts::LiveConnectExtension::TypeString;
            value = QString::fromUtf8(result.value.stringValue.UTF8Characters,
                                      result.value.stringValue.UTF8Length);
            return;
        case NPVariantType_Object:
            type = KParts::LiveConnectExtension::TypeObject;
            retobjid = registerIfNeeded(result.value.objectValue);
     }
}


bool ScriptExportEngine::get(const unsigned long objid, const QString& field,
                             KParts::LiveConnectExtension::Type& typeOut,
                             unsigned long& retobjid, QString& value)
{
    kDebug(1431) << objid << field;

    NPObject* obj = getScriptObject(objid);
    if (!obj) {
        kDebug(1431) << "huh? base object not found";
        return false;
    }

    NPIdentifier fieldIdent = g_NPN_GetQStringIdentifier(field);


    //First, see if this has a method...
    if (g_NPN_HasMethod(_pluginInstance->_npp, obj, fieldIdent)) {
        kDebug(1431) << " --> found function";

        //Return a wrapper object representing the function reference
        typeOut = KParts::LiveConnectExtension::TypeFunction;

        FuncRef f = qMakePair(obj, field);
        retobjid = registerFuncIfNeeded(f);
        kDebug(1431) << " --> returning with id:" << retobjid;
        return true;
    }

    //Now see if it is a field..
    if (g_NPN_HasProperty(_pluginInstance->_npp, obj, fieldIdent)) {
        kDebug(1431) << "--> found field";
        NPVariant result;
        if (g_NPN_GetProperty(_pluginInstance->_npp, obj, fieldIdent, &result)) {
            kDebug(1431) << "--> get OK";
            //Set our return value from the variant..
            setupReturn(result, typeOut, retobjid, value);
            g_NPN_ReleaseVariantValue(&result);
            return true;
        }
    }

    //Nothing seems to be there..
    return false;    
}

bool ScriptExportEngine::put(const unsigned long objid, const QString& field,
                             const QString& value)
{
    kDebug(1431) << objid << field << value;
    
    //Ugh. This is pretty bad, since the extension doesn't support types on put...
    //just hope the string works..
    NPObject* obj = getScriptObject(objid);
    if (!obj) {
        kDebug(1431) << "huh? no object?";
        return false;
    }

    NPIdentifier fieldIdent = g_NPN_GetQStringIdentifier(field);

    NPVariant    npValue;
    g_NPN_SetVariantFromQString(&npValue, value);
    bool result =  g_NPN_SetProperty(_pluginInstance->_npp, obj, fieldIdent, &npValue);
    g_NPN_ReleaseVariantValue(&npValue);

    kDebug(1431) << " --> result:" << result;

    return result;
}

bool ScriptExportEngine::call(const unsigned long objid, const QString& func, const QStringList& args,
                    KParts::LiveConnectExtension::Type& retType, unsigned long& retobjid, QString& value)
{
    kDebug(1431) << objid << func << args;

    // As above, we pass everything as strings --- pretty bad.
    // Also, no InvokeDefault support for now, and we assume we
    // are passed a FuncRef.
    FuncRef* fi = getScriptFunction(objid);
    if (!fi) {
        kDebug(1431) << "huh? not a funcref";
        return false;
    }

    NPIdentifier methodIdent = g_NPN_GetQStringIdentifier(func);

    //Convert arguments..
    NPVariant* npArgs = new NPVariant[args.size()];
    for (int c = 0; c < args.size(); ++c) {
        g_NPN_SetVariantFromQString(&npArgs[c], args[c]);
        kDebug(1431) << QString::fromUtf8(npArgs[c].value.stringValue.UTF8Characters,
                                      npArgs[c].value.stringValue.UTF8Length);
    }

    //Call...
    NPVariant result;
    result.type = NPVariantType_Void;
    bool ok = g_NPN_Invoke(_pluginInstance->_npp, fi->first, methodIdent, npArgs, args.size(), &result);
    kDebug(1431) << " --> invoke result:" << ok;
    kDebug(1431) << " --> res type:" << result.type;
    if (ok) {
        setupReturn(result, retType, retobjid, value);
        g_NPN_ReleaseVariantValue(&result);
    }
    
    // Cleanup..
    for (int c = 0; c < args.size(); ++c)
        g_NPN_ReleaseVariantValue(&npArgs[c]);
    delete[] npArgs;

    return ok;
}

void ScriptExportEngine::unregister(const unsigned long objid)
{
    //See if this is an object identifier..
    QHash<unsigned long, NPObject*>::const_iterator i = _objectForId.constFind(objid);
    if (i != _objectForId.constEnd()) {
        NPObject* obj = i.value();
        _objectForId.remove(objid);
        _objectIds.remove(obj);
        g_NPN_ReleaseObject(obj);
        return;
    }

    // Or perhaps a function one?
    QHash<unsigned long, FuncRef>::const_iterator fi = _functionForId.constFind(objid);
    if (fi != _functionForId.constEnd()) {
        FuncRef f = fi.value();
        _functionForId.remove(objid);
        _functionIds.remove(f);
        g_NPN_ReleaseObject(f.first);
        return;
    }
}

NPObject* ScriptExportEngine::acquireWindow()
{
    g_NPN_RetainObject(_window);
    return _window;
}

NPObject* ScriptExportEngine::acquirePluginElement()
{
    g_NPN_RetainObject(_pluginElement);
    return _pluginElement;
}


} // namespace kdeNsPluginViewer
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
