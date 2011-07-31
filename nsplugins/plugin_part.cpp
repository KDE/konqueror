/*
  Netscape Plugin Loader KPart

  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
                     Stefan Schimanski <1Stein@gmx.de>
  Copyright (c) 2002-2005 George Staikos <staikos@kde.org>

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

#include "plugin_part.h"
#include "nspluginloader.h"
#include "callbackadaptor.h"

#include <kaboutdata.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kparts/browserinterface.h>
#include <kparts/browserextension.h>

#include "nsplugins_instance_interface.h"

#include <QLabel>


class PluginBrowserExtension : public KParts::BrowserExtension
{
    friend class PluginPart;
public:
    PluginBrowserExtension( KParts::ReadOnlyPart *parent )
        : KParts::BrowserExtension( parent ) {}
    ~PluginBrowserExtension() {}

    // ATTENTION: you -CANNOT- add data members here
};


PluginLiveConnectExtension::PluginLiveConnectExtension(PluginPart* part)
: KParts::LiveConnectExtension(part), _part(part), _retval(0L) {
}

PluginLiveConnectExtension::~PluginLiveConnectExtension() {
}
static bool demarshalReturn(const NSLiveConnectResult& result, KParts::LiveConnectExtension::Type &type,
                            unsigned long& retobj, QString& value)
{
    if (result.success) {
        type   = (KParts::LiveConnectExtension::Type)result.type;
        retobj = result.objid;
        value  = result.value;
        return true;
    }
    return false;
}


bool PluginLiveConnectExtension::get(const unsigned long obj, const QString& f,
                                     Type& typeOut, unsigned long& objOut, QString& valOut) {
    kDebug(1432) << "PLUGIN:LiveConnect::get " << obj << f;

    NSPluginInstance* instance = _part->instance();
    if (instance) {
        NSLiveConnectResult result;
        result = instance->peer()->lcGet(obj, f);
        return demarshalReturn(result, typeOut, objOut, valOut);
    }
    return false;
}

bool PluginLiveConnectExtension::call(const unsigned long obj, const QString& f, const QStringList &args,
                                     Type& typeOut, unsigned long& objOut, QString& valOut) {
    kDebug(1432) << "PLUGIN:LiveConnect::call " << obj << f << args;

    NSPluginInstance* instance = _part->instance();
    if (instance) {
        NSLiveConnectResult result;
        result = instance->peer()->lcCall(obj, f, args);
        return demarshalReturn(result, typeOut, objOut, valOut);
    }

    return false;
}

bool PluginLiveConnectExtension::put( const unsigned long objId, const QString &field, const QString &value) {
    kDebug(1432) << "PLUGIN:LiveConnect::put " << objId << field << value;
    if (objId == 0) {
        if (_retval && field == "__nsplugin") {
            *_retval = value;
            return true;
        } else if (field.toLower() == "src") {
            _part->changeSrc(value);
            return true;
        }
    }

    NSPluginInstance *instance = _part->instance();
    if (instance)
        return instance->peer()->lcPut(objId, field, value);
    
    return false;
}

void PluginLiveConnectExtension::unregister( const unsigned long objid ) {
    NSPluginInstance *instance = _part->instance();
    if (instance)
        instance->peer()->lcUnregister(objid);
}


QString PluginLiveConnectExtension::evalJavaScript( const QString & script )
{
    kDebug(1432) << "PLUGIN:LiveConnect::evalJavaScript " << script;
    ArgList args;
    QString jscode;
    jscode.sprintf("this.__nsplugin=eval(\"%s\")", qPrintable( QString(script).replace('\\', "\\\\").replace('"', "\\\"")));
    //kDebug(1432) << "String is [" << jscode << "]";
    args.push_back(qMakePair(KParts::LiveConnectExtension::TypeString, jscode));
    QString nsplugin("Undefined");
    _retval = &nsplugin;
    emit partEvent(0, "eval", args);
    _retval = 0L;
    return nsplugin;
}

KComponentData *PluginFactory::s_instance = 0;

PluginFactory::PluginFactory()
  : KPluginFactory("plugin", "nsplugin")
{
    kDebug(1432) << "PluginFactory::PluginFactory";
    setComponentData(componentData());

    registerPlugin<PluginPart>();

    // preload plugin loader
    _loader = NSPluginLoader::instance();
}


PluginFactory::~PluginFactory()
{
   kDebug(1432) << "PluginFactory::~PluginFactory";

   _loader->release();

   delete s_instance;
   s_instance = 0;
}

const KComponentData &PluginFactory::componentData()
{
    if (!s_instance) {
        KAboutData about("nsplugin", 0, ki18n("Netscape Plugin"), KDE_VERSION_STRING);
        s_instance = new KComponentData(about);
    }
    return *s_instance;
}

K_EXPORT_PLUGIN(PluginFactory)

/**************************************************************************/

static int s_callBackObjectCounter;

// KDE5: use static public KPluginFactory::variantListToStringList instead.
static QStringList variantListToStringList(const QVariantList &list)
{
    QStringList stringlist;
    Q_FOREACH(const QVariant& var, list)
        stringlist << var.toString();
    return stringlist;
}

PluginPart::PluginPart(QWidget *parentWidget, QObject *parent, const QVariantList &args)
    : KParts::ReadOnlyPart(parent), _widget(0), _args(variantListToStringList(args)),
      _destructed(0L)
{
    callbackPath = QString::fromLatin1("/Callback") + QString::number(s_callBackObjectCounter);
    ++s_callBackObjectCounter;
    (void) new CallBackAdaptor( this );
    QDBusConnection::sessionBus().registerObject( callbackPath, this );

    setComponentData(PluginFactory::componentData());
    kDebug(1432) << "PluginPart::PluginPart";

    // we have to keep the class name of KParts::PluginBrowserExtension
    // to let khtml find it
    _extension = static_cast<PluginBrowserExtension*>(new KParts::BrowserExtension(this));
    _liveconnect = new PluginLiveConnectExtension(this);

    // Only create this if we have no parent since the parent part is
    // responsible for "Save As" then
    if (!parent || !parent->inherits("Part")) {
        KAction *action = actionCollection()->addAction("saveDocument");
        action->setText(i18n("&Save As..."));
        connect(action, SIGNAL(triggered(bool)), SLOT(saveAs()));
        action->setShortcut(Qt::CTRL+Qt::Key_S);
        setXMLFile("nspluginpart.rc");
    }

    // create
    _loader = NSPluginLoader::instance();

    // create a canvas to insert our widget
    _canvas = new PluginCanvasWidget( parentWidget );
    //_canvas->setFocusPolicy( QWidget::ClickFocus );
    _canvas->setFocusPolicy( Qt::WheelFocus );
    setWidget(_canvas);
    _canvas->show();
    QObject::connect( _canvas, SIGNAL(resized(int,int)),
                      this, SLOT(pluginResized(int,int)) );
}


PluginPart::~PluginPart()
{
    kDebug(1432) << "PluginPart::~PluginPart";

    _loader->release();
    if (_destructed)
        *_destructed = true;
}


bool PluginPart::openUrl(const KUrl &url)
{
    closeUrl();
    kDebug(1432) << "-> PluginPart::openUrl";

    setUrl(url);
    QString surl = url.url();
    QString smime = arguments().mimeType();
    bool reload = arguments().reload();
    bool embed = false;

    // handle arguments
    QStringList argn, argv;

    QStringList::const_iterator it = _args.constBegin();
    for ( ; it != _args.constEnd(); ) {

        int equalPos = (*it).indexOf("=");
        if (equalPos>0) {

            QString name = (*it).left(equalPos).toUpper();
            QString value = (*it).mid(equalPos+1);
            if (value[0] == '"' && value[value.length()-1] == '"')
                value = value.mid(1, value.length()-2);

            kDebug(1432) << "name=" << name << " value=" << value;

            if (!name.isEmpty()) {
                // hack to pass view mode from khtml
                if ( name=="__KHTML__PLUGINEMBED" ) {
                    embed = true;
                    kDebug(1432) << "__KHTML__PLUGINEMBED found";
                } else {
                    argn << name;
                    argv << value;
                }
            }
        }

        it++;
    }

    if (surl.isEmpty()) {
        kDebug(1432) << "<- PluginPart::openUrl - false (no url passed to nsplugin)";
        return false;
    }

    // status messages
    emit setWindowCaption( url.prettyUrl() );
    emit setStatusBarText( i18n("Loading Netscape plugin for %1", url.prettyUrl()) );

    // create plugin widget
    NSPluginInstance *inst = _loader->newInstance( _canvas, surl, smime, embed,
                                                   argn, argv,
                                                   QDBusConnection::sessionBus().baseService(),
                                                   callbackPath, reload);

    if ( inst ) {
        _widget = inst;
        _nspWidget = inst;
    } else {
        QLabel *label = new QLabel( i18n("Unable to load Netscape plugin for %1", url.url()), _canvas );
        label->setAlignment( Qt::AlignCenter );
        label->setWordWrap( true );
        _widget = label;
    }

    _widget->resize(_canvas->width(), _canvas->height());
    _widget->show();

    kDebug(1432) << "<- PluginPart::openUrl = " << (inst!=0);
    return inst != 0L;
}


bool PluginPart::closeUrl()
{
    kDebug(1432) << "PluginPart::closeUrl";
    delete _widget;
    _widget = 0;
    return true;
}


void PluginPart::reloadPage()
{
    kDebug(1432) << "PluginPart::reloadPage()";
    _extension->browserInterface()->callMethod("goHistory", 0);
}

void PluginPart::postURL(const QString& url, const QString& target, const QByteArray& data, const QString& mime)
{
    kDebug(1432) << "PluginPart::postURL( url=" << url
                  << ", target=" << target << endl;

    KUrl new_url(this->url(), url);
    KParts::OpenUrlArguments arguments;
    KParts::BrowserArguments browserArguments;
    browserArguments.setDoPost(true);
    browserArguments.frameName = target;
    browserArguments.postData = data;
    browserArguments.setContentType(mime);

    emit _extension->openUrlRequest(new_url, arguments, browserArguments);
}

void PluginPart::requestURL(const QString& url, const QString& target)
{
    kDebug(1432) << "PluginPart::requestURL( url=" << url
                  << ", target=" << target << endl;

    KUrl new_url(this->url(), url);
    KParts::OpenUrlArguments arguments;
    KParts::BrowserArguments browserArguments;
    browserArguments.frameName = target;
    browserArguments.setDoPost(false);

    emit _extension->openUrlRequest(new_url, arguments, browserArguments);
}

NSPluginInstance* PluginPart::instance()
{
    if (!_widget) return 0;
    return dynamic_cast<NSPluginInstance*>(_widget.operator->());
}

void PluginPart::evalJavaScript(int id, const QString & script)
{
    kDebug(1432) <<"evalJavascript: before widget check";
    if (_widget) {
        bool destructed = false;
        _destructed = &destructed;
		kDebug(1432) <<"evalJavascript: there is a widget:";
        QString rc = _liveconnect->evalJavaScript(script);
        if (destructed)
            return;
        _destructed = 0L;
        kDebug(1432) << "Liveconnect: script [" << script << "] evaluated to [" << rc << "]";
        NSPluginInstance *ni = instance();
        if (ni)
            ni->javascriptResult(id, rc);
    }
}

void PluginPart::statusMessage(const QString &msg)
{
    kDebug(1422) << "PluginPart::statusMessage " << msg;
    emit setStatusBarText(msg);
}


void PluginPart::pluginResized(int w, int h)
{
    if (_widget) {
        _widget->resize(w, h);
    }
}


void PluginPart::changeSrc(const QString& url) {
    closeUrl();
    openUrl(KUrl( url ));
}


void PluginPart::saveAs() {
    KUrl savefile = KFileDialog::getSaveUrl(QString(), QString(), _widget);
    KIO::NetAccess::file_copy(url(), savefile, _widget);
}


void PluginCanvasWidget::resizeEvent(QResizeEvent *ev)
{
    QWidget::resizeEvent(ev);
    emit resized(width(), height());
}


#include "plugin_part.moc"
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
