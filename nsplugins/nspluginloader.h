/*

  This is an encapsulation of the  Netscape plugin API.


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


#ifndef NS_PLUGINLOADER_H
#define NS_PLUGINLOADER_H

#include <QHash>
#include <QObject>
#include <QWidget>
#include <QtGui/QX11EmbedContainer>

#include <kdemacros.h>
#include <kprocess.h>

#define EMBEDCLASS QX11EmbedContainer

class OrgKdeNspluginsViewerInterface;
class QPushButton;
class QGridLayout;
class OrgKdeNspluginsInstanceInterface;

class NSPluginInstance : public EMBEDCLASS
{
  Q_OBJECT

public:
    NSPluginInstance(QWidget *parent, const QString& app, const QString& id);
    ~NSPluginInstance();

    void javascriptResult(int id, const QString &result);

    void pluginResized(int w, int h);
private Q_SLOTS:
    void doLoadPlugin(int w, int h);
protected:
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);
    void windowChanged(WId w);
    virtual void focusInEvent( QFocusEvent* event );
    virtual void focusOutEvent( QFocusEvent* event );
private:
    class NSPluginLoader *_loader;
    OrgKdeNspluginsInstanceInterface *_instanceInterface;
    bool inited;
    bool haveSize;
    void embedIfNeeded(int w, int h);
    void resizePlugin(int w, int h );

    QPushButton *_button;
    QGridLayout *_layout;
};

// class exported for the test program
class KDE_EXPORT NSPluginLoader : public QObject
{
  Q_OBJECT

public:
  NSPluginLoader();
  ~NSPluginLoader();

  NSPluginInstance *newInstance(QWidget *parent,
				const QString& url, const QString& mimeType, bool embed,
				const QStringList& argn, const QStringList& argv,
                                const QString& ownDBusId, const QString& callbackId, bool reload );

  static NSPluginLoader *instance();
  void release();

protected:
  void scanPlugins();

  QString lookup(const QString &mimeType);
  QString lookupMimeType(const QString &url);

  bool loadViewer();
  void unloadViewer();

protected Q_SLOTS:
  void processTerminated();

private:
  QStringList _searchPaths;
  QMultiHash<QString, QString> _mapping;
  QHash<QString, QString> _filetype;

  KProcess _process;
  QString _viewerDBusId;
  OrgKdeNspluginsViewerInterface *_viewer;

  static NSPluginLoader *s_instance;
  static int s_refCount;
};


#endif
