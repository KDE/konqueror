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

PluginHostXt::PluginHostXt(NSPluginInstance* plugin, QX11EmbedWidget* outside, 
                           int width, int height):
    _plugin(plugin), _outside(outside)
{
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
}

PluginHostXt::~PluginHostXt()
{
    XtRemoveEventHandler(_form, (KeyPressMask|KeyReleaseMask),
                            False, forwarder, (XtPointer)this);
    XtRemoveEventHandler(_toplevel, (KeyPressMask|KeyReleaseMask),
                            False, forwarder, (XtPointer)this);
    XtDestroyWidget(_form);
    XtDestroyWidget(_toplevel);
    _form     = 0;
    _toplevel = 0;
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

void PluginHostXt::setupWindow(int width, int height)
{
    // Embed the Xt widget into the Qt widget
    XReparentWindow(QX11Info::display(), XtWindow(_toplevel), _outside->winId(), 0, 0);
    XtMapWidget(_toplevel);
    NPWindow win;
    win.x = 0;
    win.y = 0;
    win.width  = width;
    win.height = height;
    win.type = NPWindowTypeWindow;
    
    // Well, the docu says sometimes, this is only used on the
    // MAC, but sometimes it says it's always. Who knows...
    win.clipRect.top  = 0;
    win.clipRect.left = 0;
    win.clipRect.bottom = height;
    win.clipRect.right  = width;
    
    win.window =  (void*) XtWindow(_form);
    QX11Info x11info;
    NPSetWindowCallbackStruct win_info;
    win_info.display  = QX11Info::display();
    win_info.visual   = (Visual*) x11info.visual();
    win_info.colormap = x11info.colormap();
    win_info.depth    = x11info.depth();
    win.ws_info = &win_info;
    
    NPError error = _plugin->NPSetWindow( &win );
    
    kDebug(1431) << "error = " << error;
}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
