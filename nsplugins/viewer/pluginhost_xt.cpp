/*

  Xt plugin embedding support

  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
                     Stefan Schimanski <1Stein@gmx.de>
                2003-2005 George Staikos <staikos@kde.org>
                2007 Maksim orlovich     <maksim@kde.org>

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

#include "pluginhost_xt.h"
#include <QX11Info>
#include <QVBoxLayout>
#include <QLabel>

// BEGIN Workaround for QX11EmbedWidget silliness --- it maps widgets by default
// Most of the code is from Qt 4.3.3, Copyright (C) 1992-2007 Trolltech ASA
static unsigned int XEMBED_VERSION = 0;
static Atom _XEMBED_INFO = None;
static void initXEmbedAtoms(Display *d)
{
    if (_XEMBED_INFO == None)
        _XEMBED_INFO = XInternAtom(d, "_XEMBED_INFO", false);
}

static void doNotAskForXEmbedMapping(QX11EmbedWidget* widget)
{
    initXEmbedAtoms(widget->x11Info().display());
    unsigned int data[] = {XEMBED_VERSION, 0 /* e.g. not XEMBED_MAPPED*/};
    XChangeProperty(widget->x11Info().display(), widget->winId(), _XEMBED_INFO,
                    _XEMBED_INFO, 32, PropModeReplace,
                    (unsigned char*) data, 2);
}

// END  Workaround for QX11EmbedWidget silliness.


PluginHostXt::PluginHostXt(NSPluginInstance* plugin):
    _plugin(plugin), _outside(0), _toplevel(0), _form(0)
{}


void PluginHostXt::setupWindow(int winId, int width, int height)
{
    _outside = new QX11EmbedWidget();
    doNotAskForXEmbedMapping(_outside);
    _outside->embedInto(winId);

    Arg args[7];
    Cardinal nargs = 0;
    XtSetArg(args[nargs], XtNwidth, width); nargs++;
    XtSetArg(args[nargs], XtNheight, height); nargs++;
    XtSetArg(args[nargs], XtNborderWidth, 0); nargs++;

    String n, c;
    XtGetApplicationNameAndClass(QX11Info::display(), &n, &c);
    _toplevel = XtAppCreateShell("drawingArea", c, applicationShellWidgetClass,
                                 QX11Info::display(), args, nargs);

    //if (embed)
        XtSetMappedWhenManaged(_toplevel, False);
    XtRealizeWidget(_toplevel);

    // Create form window that is searched for by flash plugin
    _form = XtVaCreateWidget("form", compositeWidgetClass, _toplevel, NULL);
    XtSetArg(args[nargs], XtNvisual, QX11Info::appVisual()); nargs++;
    XtSetArg(args[nargs], XtNdepth, QX11Info::appDepth()); nargs++;
    XtSetArg(args[nargs], XtNcolormap, QX11Info::appColormap()); nargs++;
    XtSetValues(_form, args, nargs);
    XSync(QX11Info::display(), false);

    // From mozilla - not sure if it's needed yet, nor what to use for embedder
#if 0
    /* this little trick seems to finish initializing the widget */
#if XlibSpecificationRelease >= 6
    XtRegisterDrawable(QX11Info::display(), embedderid, _toplevel);
#else
    _XtRegisterWindow(embedderid, _toplevel);
#endif
#endif
    XtRealizeWidget(_form);
    XtManageChild(_form);

    // Register forwarder
    XtAddEventHandler(_toplevel, (KeyPressMask|KeyReleaseMask),
                      False, forwarder, (XtPointer)this );
    XtAddEventHandler(_form, (KeyPressMask|KeyReleaseMask),
                      False, forwarder, (XtPointer)this );

    // Embed the Xt widget into the Qt widget
    XReparentWindow(QX11Info::display(), XtWindow(_toplevel), _outside->winId(), 0, 0);
    XtMapWidget(_toplevel);
    setupPluginWindow(_plugin, (void*) XtWindow(_form), width, height);
}

PluginHostXt::~PluginHostXt()
{
    if (_form) {
        XtRemoveEventHandler(_form, (KeyPressMask|KeyReleaseMask),
                                False, forwarder, (XtPointer)this);
        XtRemoveEventHandler(_toplevel, (KeyPressMask|KeyReleaseMask),
                                False, forwarder, (XtPointer)this);
        XtDestroyWidget(_form);
        XtDestroyWidget(_toplevel);
        _form     = 0;
        _toplevel = 0;
    }
    delete _outside;
}

void PluginHostXt::forwarder(Widget w, XtPointer cl_data, XEvent * event, Boolean * cont)
{
    Q_UNUSED(w);
    PluginHostXt *inst = (PluginHostXt*)cl_data;
    *cont = True;
    if (inst->_form == 0 || event->xkey.window == XtWindow(inst->_form))
        return;
    *cont = False;
    event->xkey.window = XtWindow(inst->_form);
    event->xkey.subwindow = None;
    XtDispatchEvent(event);
}


static void resizeWidgets(Window w, int width, int height) {
   Window rroot, parent, *children;
   unsigned int nchildren = 0;

   if (XQueryTree(QX11Info::display(), w, &rroot, &parent, &children, &nchildren)) {
      for (unsigned int i = 0; i < nchildren; i++) {
         XResizeWindow(QX11Info::display(), children[i], width, height);
      }
      XFree(children);
   }
}

 #if 0

void PluginHostXt::resizePlugin(int w, int h)
{
    kDebug(1431) << w << h;
    XResizeWindow(QX11Info::display(), XtWindow(_form), w, h);
    XResizeWindow(QX11Info::display(), XtWindow(_toplevel), w, h);

    Arg args[7];
    Cardinal nargs = 0;
    XtSetArg(args[nargs], XtNwidth, w); nargs++;
    XtSetArg(args[nargs], XtNheight, h); nargs++;
    XtSetArg(args[nargs], XtNvisual, QX11Info::appVisual()); nargs++;
    XtSetArg(args[nargs], XtNdepth, QX11Info::appDepth()); nargs++;
    XtSetArg(args[nargs], XtNcolormap, QX11Info::appColormap()); nargs++;
    XtSetArg(args[nargs], XtNborderWidth, 0); nargs++;

    XtSetValues(_form, args, nargs);

    resizeWidgets(XtWindow(_form), w, h);
}

#endif

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
